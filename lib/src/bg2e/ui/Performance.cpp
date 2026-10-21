/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/ui/Performance.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/base/CpuProfile.hpp>

#include "detail/PerformanceData.hpp"

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <string>

namespace bg2e::ui {
namespace {
uint64_t timestamp()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool waitStage(const std::string& label)
{
    return label.find("wait") != std::string::npos || label.find("Wait") != std::string::npos ||
        label.find("nextDrawable") != std::string::npos;
}

const char* gpuStatus(base::GpuProfileStatus status)
{
    switch (status)
    {
    case base::GpuProfileStatus::Available: return "Available";
    case base::GpuProfileStatus::Unsupported: return "Unsupported";
    case base::GpuProfileStatus::Dropped: return "Dropped";
    case base::GpuProfileStatus::Failed: return "Failed";
    case base::GpuProfileStatus::Aborted: return "Aborted";
    case base::GpuProfileStatus::Unavailable: return "Unavailable";
    default: return "Pending";
    }
}

const char* gpuWorkload(base::GpuProfileWorkload workload)
{
    switch (workload)
    {
    case base::GpuProfileWorkload::Scene: return "Scene";
    case base::GpuProfileWorkload::Ui: return "UI";
    case base::GpuProfileWorkload::SceneAndUi: return "Scene + UI";
    case base::GpuProfileWorkload::Auxiliary: return "Auxiliary";
    default: return "Unassociated";
    }
}

std::string gpuTitle(const detail::PerformanceModel::GpuStream& stream)
{
    return "Device " + std::to_string(stream.device) + " / queue " + std::to_string(stream.queue) +
        " / epoch " + std::to_string(stream.epoch);
}

std::string keyTitle(const detail::PerformanceKey& key)
{
    return key.label + " (" + key.unit + ") [thread " + std::to_string(key.thread) +
        ", epoch " + std::to_string(key.epoch) + "]";
}

// Fixed time bins preserve real timestamp spacing and leave absent bins empty.
void chart(const char* label, const detail::PerformanceSeries& series, const char* unit, double seconds)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const auto top = ImGui::GetCursorScreenPos();
    const ImVec2 size(std::max(80.0f, ImGui::GetContentRegionAvail().x), 72.0f);
    ImGui::InvisibleButton("plot", size);
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 bottom(top.x + size.x, top.y + size.y);
    draw->AddRectFilled(top, bottom, ImGui::GetColorU32(ImGuiCol_FrameBg));
    float maximum = 0;
    size_t valid = 0;
    for (size_t i = 0; i < detail::PerformanceSeries::capacity; ++i)
    {
        const float value = series.value(i);
        if (std::isfinite(value))
        {
            maximum = std::max(maximum, value);
            ++valid;
        }
    }
    if (!valid)
    {
        draw->AddText(ImVec2(top.x + 8, top.y + 8), ImGui::GetColorU32(ImGuiCol_TextDisabled),
            "Unavailable / no valid samples");
    }
    else
    {
        const float scale = maximum > 0 ? maximum * 1.1f : 1.0f;
        bool previousValid = false;
        ImVec2 previous;
        draw->PushClipRect(top, bottom, true);
        for (size_t i = 0; i < detail::PerformanceSeries::capacity; ++i)
        {
            const float value = series.value(i);
            if (!std::isfinite(value))
            {
                previousValid = false;
                continue;
            }
            const ImVec2 point(top.x + (float(i) + 0.5f) / detail::PerformanceSeries::capacity * size.x,
                bottom.y - 3 - value / scale * (size.y - 6));
            const auto color = ImGui::GetColorU32(ImGuiCol_PlotLines);
            draw->AddCircleFilled(point, 1.4f, color);
            if (previousValid)
            {
                draw->AddLine(previous, point, color);
            }
            previous = point;
            previousValid = true;
        }
        draw->PopClipRect();
        if (ImGui::IsItemHovered())
        {
            const auto index = std::min(detail::PerformanceSeries::capacity - 1,
                size_t(std::max(0.0f, ImGui::GetIO().MousePos.x - top.x) / size.x *
                    detail::PerformanceSeries::capacity));
            const float value = series.value(index);
            if (std::isfinite(value))
            {
                ImGui::SetTooltip("%.3f %s | %u events | %.3f s before window end",
                    double(value), unit, series.counts[index],
                    seconds * (1.0 - (double(index) + 0.5) / detail::PerformanceSeries::capacity));
            }
            else
            {
                ImGui::SetTooltip("No sample in this time bin");
            }
        }
    }
    ImGui::TextDisabled("%.3g %s peak | %zu samples | %.2f s", double(maximum), unit, series.samples(), seconds);
    ImGui::PopID();
}
}

struct Performance::Impl {
    std::unique_ptr<base::CpuProfiler::Collection> collection;
    detail::PerformanceModel model;
    uint64_t cutoff = 0, nextRefresh = 0, modalNs = 0;
    double refreshMs = 0, drawMs = 0;
    bool paused = false, busy = false, snapshotFailed = false;
    std::filesystem::path pendingPath;
    std::string error;

    void refresh()
    {
        const auto start = timestamp();
        if (paused || start < nextRefresh)
        {
            return;
        }
        try
        {
            {
                base::CpuProfileScope profile("ui.performance.snapshot");
                model.snapshot = base::CpuProfiler::snapshot(cutoff);
            }
            {
                base::CpuProfileScope profile("ui.performance.series");
                model.rows();
                model.rebuild(timestamp());
            }
            snapshotFailed = false;
            nextRefresh = timestamp() + 250'000'000;
        }
        catch (...)
        {
            // An unavailable snapshot must not abort application rendering.
            model.snapshot.samples.clear();
            model.snapshot.gpuSamples.clear();
            model.snapshot.totals.clear();
            model.snapshot.begin = model.snapshot.end = 0;
            model.rows();
            model.rebuild(timestamp());
            snapshotFailed = true;
            nextRefresh = timestamp() + 1'000'000'000;
        }
        refreshMs = double(timestamp() - start) / 1e6;
    }

    void start(const std::filesystem::path& path, bool overwrite)
    {
        if (!base::CpuProfiler::start(path, overwrite))
        {
            error = base::CpuProfiler::error();
        }
        else
        {
            error.clear();
        }
    }

    void recordingControls()
    {
        auto status = base::CpuProfiler::recordingStatus();
        ImGui::BeginDisabled(busy);
        if (ImGui::Button(status.active ? "Stop Recording" : "Start Recording"))
        {
            struct BusyGuard {
                bool& value;

                explicit BusyGuard(bool& flag) : value(flag)
                {
                    value = true;
                }

                ~BusyGuard()
                {
                    value = false;
                }
            } guard(busy);
            try
            {
                if (status.active)
                {
                    base::CpuProfiler::stop();
                    error.clear();
                }
                else
                {
                    std::filesystem::path path;
                    const auto dialogStart = timestamp();
                    try
                    {
                        path = app::FileDialog::getSaveFilePath({{"Performance capture", "jsonl"}});
                    }
                    catch (...)
                    {
                        const auto dialogEnd = timestamp();
                        model.dialog(dialogStart, dialogEnd);
                        modalNs += dialogEnd - dialogStart;
                        throw;
                    }
                    const auto dialogEnd = timestamp();
                    model.dialog(dialogStart, dialogEnd);
                    modalNs += dialogEnd - dialogStart;
                    if (!path.empty())
                    {
                        if (path.extension().empty())
                        {
                            path += ".jsonl";
                        }
                        if (std::filesystem::exists(path))
                        {
                            pendingPath = path;
                            ImGui::OpenPopup("Replace capture?");
                        }
                        else
                        {
                            start(path, false);
                        }
                    }
                }
            }
            catch (const std::exception& exception)
            {
                error = exception.what();
            }
            catch (...)
            {
                error = "Cannot change recording state";
            }
        }
        ImGui::EndDisabled();
        if (ImGui::BeginPopupModal("Replace capture?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Replace the existing capture? Its previous contents will be lost.");
            ImGui::TextUnformatted(pendingPath.string().c_str());
            ImGui::BeginDisabled(busy || base::CpuProfiler::recording());
            if (ImGui::Button("Replace"))
            {
                busy = true;
                try
                {
                    start(pendingPath, true);
                }
                catch (const std::exception& exception)
                {
                    error = exception.what();
                }
                busy = false;
                pendingPath.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                pendingPath.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        // Re-read after synchronous actions or an asynchronous writer failure.
        status = base::CpuProfiler::recordingStatus();
        if (!status.path.empty())
        {
            ImGui::TextWrapped("%s: %s", status.active ? "Recording" : "Last capture", status.path.string().c_str());
        }
        if (status.begin)
        {
            const auto end = status.active ? timestamp() : status.end;
            ImGui::Text("Session %llu | %.2f s", static_cast<unsigned long long>(status.session),
                end >= status.begin ? double(end - status.begin) / 1e9 : 0.0);
        }
        if (!error.empty())
        {
            ImGui::TextWrapped("%s", error.c_str());
        }
        if (!status.error.empty() && status.error != error)
        {
            ImGui::TextWrapped("%s", status.error.c_str());
        }
    }

    bool select(const char* label, detail::PerformanceKey& key, const std::vector<size_t>& rows)
    {
        bool changed = false;
        if (ImGui::BeginCombo(label, key.thread ? keyTitle(key).c_str() : "Unavailable"))
        {
            for (auto index : rows)
            {
                const auto& sample = model.snapshot.totals[index];
                detail::PerformanceKey item;
                item.assign(sample);
                const auto title = keyTitle(item);
                ImGui::PushID(static_cast<int>(index));
                if (ImGui::Selectable(title.c_str(), key.matches(sample)))
                {
                    key.assign(sample);
                    changed = true;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    void gpuContents(double seconds)
    {
        if (!ImGui::CollapsingHeader("GPU submissions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            return;
        }
        bool enabled = base::CpuProfiler::gpuTimingEnabled();
        if (ImGui::Checkbox("GPU submission timing", &enabled))
        {
            base::CpuProfiler::setGpuTimingEnabled(enabled);
        }
        if (!enabled)
        {
            ImGui::TextWrapped("GPU timing disabled for new submissions. CPU collection/recording continue; previous GPU results remain visible.");
        }
        bool passes = base::CpuProfiler::gpuPassTimingEnabled();
        if (ImGui::Checkbox("GPU pass intervals", &passes))
        {
            base::CpuProfiler::setGpuPassTimingEnabled(passes);
        }
        bool detailed = base::CpuProfiler::gpuDetailedTimingEnabled();
        if (ImGui::Checkbox("Detailed transparent GPU intervals", &detailed))
        {
            base::CpuProfiler::setGpuDetailedTimingEnabled(detailed);
        }
        if (ImGui::BeginCombo("GPU stage", model.gpuStage.empty() ? "Total command" : model.gpuStage.c_str()))
        {
            if (ImGui::Selectable("Total command", model.gpuStage.empty()))
            {
                model.gpuStage.clear();
                model.rebuild(model.end);
            }
            std::string next;
            for (const auto& label : model.gpuStages)
            {
                if (ImGui::Selectable(label.c_str(), label == model.gpuStage))
                {
                    next = label;
                }
            }
            if (!next.empty())
            {
                model.gpuStage = next;
                model.rebuild(model.end);
            }
            ImGui::EndCombo();
        }
        ImGui::TextDisabled("%zu interval records outside the 128-label selector", model.omittedGpuStages);
        const auto& snapshot = model.snapshot;
        ImGui::TextWrapped("GPU execution envelopes per submission/queue; never summed across submissions or queues. "
            "X axis is the original CPU request time, not a calibrated GPU timeline. "
            "Draw requests cover scene/UI work; auxiliary submissions are unmeasured unless explicitly requested.");
        ImGui::Text("Pending publication %llu | drops %llu | incomplete/failed %llu | unsupported %llu",
            static_cast<unsigned long long>(snapshot.gpuPending), static_cast<unsigned long long>(snapshot.gpuDropped),
            static_cast<unsigned long long>(snapshot.gpuFailed), static_cast<unsigned long long>(snapshot.gpuUnsupported));
        ImGui::TextDisabled("GPU history evictions %llu | expired/unknown late results %llu | %zu stream records outside selector",
            static_cast<unsigned long long>(snapshot.gpuEvicted), static_cast<unsigned long long>(snapshot.gpuLate), model.omittedGpuStreams);
        if (snapshot.gpuQueues.empty())
        {
            ImGui::TextDisabled("GPU capability unavailable: no draw timing consumer has published a queue yet.");
        }
        for (const auto& queue : snapshot.gpuQueues)
        {
            ImGui::TextWrapped("%s device %llu / queue %llu (last observed): %s | backend pending %llu, dropped %llu, failed %llu",
                queue.backend == base::GpuProfileBackend::Vulkan ? "Vulkan" :
                    queue.backend == base::GpuProfileBackend::Metal ? "Metal" : "Unknown backend",
                static_cast<unsigned long long>(queue.device), static_cast<unsigned long long>(queue.queue),
                queue.supported ? "submission timing supported; each result is validated" : "submission timing unsupported",
                static_cast<unsigned long long>(queue.pending), static_cast<unsigned long long>(queue.dropped),
                static_cast<unsigned long long>(queue.failed));
        }
        if (ImGui::BeginCombo("GPU queue", model.selectedGpu.epoch ? gpuTitle(model.selectedGpu).c_str() : "Unavailable"))
        {
            for (const auto& stream : model.gpuStreams)
            {
                const bool selected = stream.epoch == model.selectedGpu.epoch && stream.device == model.selectedGpu.device &&
                    stream.queue == model.selectedGpu.queue;
                if (ImGui::Selectable(gpuTitle(stream).c_str(), selected))
                {
                    model.selectedGpu = stream;
                    model.rebuild(model.end);
                    break;
                }
            }
            ImGui::EndCombo();
        }
        const char* workloads[] = {"All submissions", "Unassociated", "Scene", "UI", "Scene + UI", "Auxiliary"};
        int workload = model.gpuWorkload + 1;
        if (ImGui::Combo("GPU workload", &workload, workloads, 6))
        {
            model.gpuWorkload = workload - 1;
            model.rebuild(model.end);
        }
        chart("Selected GPU duration (mean per bin, never a sum)", model.gpuMs, "ms", seconds);
        ImGui::Text("Selected queue: %zu valid, %zu unavailable results", model.gpuAvailable, model.gpuUnavailable);
        ImGui::TextDisabled("Recent completed results: all queues/epochs, at most 128 rows.");
        if (ImGui::BeginTable("gpu_submissions", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 160)))
        {
            ImGui::TableSetupColumn("Epoch / thread");
            ImGui::TableSetupColumn("Device / queue");
            ImGui::TableSetupColumn("Command / submission");
            ImGui::TableSetupColumn("Frame token / scene");
            ImGui::TableSetupColumn("Workload");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("GPU ms");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();
            for (auto index : model.gpuRows)
            {
                const auto& sample = snapshot.gpuSamples[index];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%llu / %llu", static_cast<unsigned long long>(sample.epoch), static_cast<unsigned long long>(sample.thread));
                ImGui::TableNextColumn();
                ImGui::Text("%llu / %llu", static_cast<unsigned long long>(sample.device), static_cast<unsigned long long>(sample.queue));
                ImGui::TableNextColumn();
                ImGui::Text("%llu / %llu", static_cast<unsigned long long>(sample.command), static_cast<unsigned long long>(sample.submission));
                ImGui::TableNextColumn();
                ImGui::Text("%llu / %llu", static_cast<unsigned long long>(sample.presentationFrame), static_cast<unsigned long long>(sample.sceneFrame));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(gpuWorkload(sample.workload));
                ImGui::TableNextColumn();
                bool partial = sample.intervalsDropped != 0;
                for (uint32_t i = 0; i < sample.intervalCount; ++i)
                {
                    partial |= sample.intervals[i].status != base::GpuProfileStatus::Available;
                }
                ImGui::Text("%s%s", gpuStatus(sample.status), partial ? " / partial passes" : "");
                ImGui::TableNextColumn();
                if (sample.status == base::GpuProfileStatus::Available)
                {
                    ImGui::Text("%.3f", sample.durationMs);
                }
                else
                {
                    ImGui::TextUnformatted("Unavailable");
                }
            }
            ImGui::EndTable();
        }
        ImGui::TextWrapped("Pass intervals may nest or overlap; never add them. Vulkan: top/bottom pipeline envelope. "
            "Metal: natural render/compute encoder envelope, including gaps. Transfers or interior endpoints are unsupported. "
            "Detailed sampling is bounded to 32 intervals and 128 Metal encoders per command.");
        if (ImGui::BeginTable("gpu_passes", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 200)))
        {
            for (const auto* column : {"Command / ID / parent", "Pass", "Context", "Granularity", "State", "GPU ms"})
            {
                ImGui::TableSetupColumn(column);
            }
            ImGui::TableHeadersRow();
            size_t rows = 0;
            uint64_t dropped = 0;
            for (auto index : model.gpuRows)
            {
                const auto& sample = snapshot.gpuSamples[index];
                dropped += sample.intervalsDropped;
                for (uint32_t i = 0; i < sample.intervalCount && rows < 256; ++i)
                {
                    const auto& interval = sample.intervals[i];
                    if (!model.gpuStage.empty() && model.gpuStage != interval.label.data())
                    {
                        continue;
                    }
                    ++rows;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu / %u / %u", static_cast<unsigned long long>(sample.command), interval.id, interval.parent);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(interval.label.data());
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", static_cast<unsigned long long>(interval.context));
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(interval.granularity == base::GpuProfileGranularity::PipelineBoundary ? "Pipeline boundary" :
                        interval.granularity == base::GpuProfileGranularity::EncoderEnvelope ? "Encoder envelope" : "Unsupported");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(gpuStatus(interval.status));
                    ImGui::TableNextColumn();
                    if (interval.status == base::GpuProfileStatus::Available)
                    {
                        ImGui::Text("%.3f", interval.durationMs);
                    }
                    else
                    {
                        ImGui::TextUnformatted("Unavailable");
                    }
                }
            }
            ImGui::EndTable();
            ImGui::TextDisabled("At most 256 interval rows; %llu omitted intervals in recent submission rows.",
                static_cast<unsigned long long>(dropped));
        }
    }

    void contents()
    {
        recordingControls();
        ImGui::Separator();
        if (ImGui::Checkbox("Pause display", &paused) && !paused)
        {
            nextRefresh = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear charts"))
        {
            cutoff = timestamp();
            nextRefresh = 0;
            model.snapshot.samples.clear();
            model.snapshot.gpuSamples.clear();
            model.snapshot.totals.clear();
            model.snapshot.begin = model.snapshot.end = 0;
            model.snapshot.presentationFrames = model.snapshot.sceneFrames = model.snapshot.incompleteFrames = 0;
            model.rows();
            model.rebuild(cutoff);
        }
        ImGui::TextDisabled("Display controls affect this window only. Collection and recording continue.");
        refresh();
        if (snapshotFailed)
        {
            ImGui::TextWrapped("Performance snapshot unavailable; collection/recording are unchanged. Retrying in one second.");
        }
        const auto& snapshot = model.snapshot;
        const double seconds = model.end > model.begin ? double(model.end - model.begin) / 1e9 : 0;
        const double retained = snapshot.end >= snapshot.begin ? double(snapshot.end - snapshot.begin) / 1e9 : 0;
        ImGui::Text("Retained event span %.2f s | %zu events | %zu stage rows | %zu counter rows", retained,
            snapshot.samples.size(), model.stages.size(), model.counters.size());
        ImGui::Text("Evicted %llu | collection drops %llu | writer drops %llu | partial boundaries %llu",
            static_cast<unsigned long long>(snapshot.evicted), static_cast<unsigned long long>(snapshot.dropped),
            static_cast<unsigned long long>(snapshot.writerDropped), static_cast<unsigned long long>(snapshot.partial));
        ImGui::TextDisabled("Last refresh %.3f ms | previous window draw %.3f ms (native dialog excluded)", refreshMs, drawMs);
        ImGui::TextWrapped("CPU wall time includes waits and driver work. Wait labels are identified by name. "
            "Scopes and threads are never added to estimate frame time. GPU values are per submission, not frame totals.");
        ImGui::TextDisabled("Chart range %.3f..%.3f s (monotonic); cadence uses CPU frame completion, not display timestamps.",
            double(model.begin) / 1e9, double(model.end) / 1e9);
        ImGui::TextDisabled("Chart bins: %.3f s; gaps mean unavailable. Cadence/stage values are bin means; counters are bin sums.",
            seconds / detail::PerformanceSeries::capacity);
        ImGui::TextDisabled("%zu unassociated events | %zu modal-overlapping events omitted from charts",
            model.unassociated, model.modalSamples);
        if (model.omittedStages || model.omittedCounters || model.omittedStreams)
        {
            ImGui::TextWrapped("Selector limits: %zu stage rows, %zu counter rows and %zu frame records omitted.",
                model.omittedStages, model.omittedCounters, model.omittedStreams);
        }
        const auto streamTitle = "Thread " + std::to_string(model.selectedStream.thread) +
            ", epoch " + std::to_string(model.selectedStream.epoch);
        if (ImGui::BeginCombo("Frame stream", model.selectedStream.thread ? streamTitle.c_str() : "Unavailable"))
        {
            for (const auto& stream : model.streams)
            {
                const auto title = "Thread " + std::to_string(stream.thread) + ", epoch " + std::to_string(stream.epoch);
                const bool selected = stream.thread == model.selectedStream.thread && stream.epoch == model.selectedStream.epoch;
                if (ImGui::Selectable(title.c_str(), selected))
                {
                    model.selectedStream = stream;
                    model.rebuild(model.end);
                    break;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Text("Frame stream: %zu presentations | %zu scene frames | %zu incomplete attempts",
            model.presentations, model.sceneFrames, model.incomplete);
        if (ImGui::CollapsingHeader("Presentation and scene cadence", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("cadence", 2, ImGuiTableFlags_SizingStretchSame))
            {
                ImGui::TableNextColumn();
                chart("Presentation interval", model.presentationMs, "ms", seconds);
                ImGui::TableNextColumn();
                chart("Presentation FPS", model.presentationFps, "FPS", seconds);
                ImGui::TableNextColumn();
                chart("Scene interval", model.sceneMs, "ms", seconds);
                ImGui::TableNextColumn();
                chart("Scene FPS", model.sceneFps, "FPS", seconds);
                ImGui::EndTable();
            }
        }
        chart("CPU frame wall time (includes waits)", model.frameMs, "ms", seconds);
        gpuContents(seconds);
        if (ImGui::CollapsingHeader("CPU stages", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (select("Stage", model.stage, model.stages))
            {
                model.rebuild(model.end);
            }
            ImGui::TextDisabled("%s", waitStage(model.stage.label) ? "Selected stage: wait/blocking" : "Selected stage: CPU/driver scope");
            chart("Stage inclusive per call", model.stageInclusive, "ms", seconds);
            chart("Stage exclusive per call", model.stageExclusive, "ms", seconds);
            ImGui::TextWrapped("Table totals cover retained events, including raw modal/partial scopes. "
                "Maximum is inclusive time per call. Rows prioritize the current epoch, then exclusive total; threads/epochs remain separate.");
            if (ImGui::BeginTable("stages", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 200)))
            {
                ImGui::TableSetupColumn("Stage");
                ImGui::TableSetupColumn("Thread / epoch");
                ImGui::TableSetupColumn("Kind");
                ImGui::TableSetupColumn("Exclusive ms");
                ImGui::TableSetupColumn("Inclusive ms");
                ImGui::TableSetupColumn("Maximum ms");
                ImGui::TableSetupColumn("Calls");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();
                for (auto index : model.stages)
                {
                    const auto& sample = snapshot.totals[index];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(static_cast<int>(index));
                    if (ImGui::Selectable(sample.label.c_str(), model.stage.matches(sample)))
                    {
                        model.stage.assign(sample);
                        model.rebuild(model.end);
                    }
                    ImGui::PopID();
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu / %llu", static_cast<unsigned long long>(sample.thread), static_cast<unsigned long long>(sample.epoch));
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(waitStage(sample.label) ? "Wait" : "CPU/driver");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", sample.exclusiveMs);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", sample.inclusiveMs);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", sample.maximumMs);
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", static_cast<unsigned long long>(sample.calls));
                }
                ImGui::EndTable();
            }
        }
        if (ImGui::CollapsingHeader("Counters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (select("Counter", model.counter, model.counters))
            {
                model.rebuild(model.end);
            }
            chart("Counter events summed per time bin", model.counterValues, model.counter.unit.c_str(), seconds);
            bool found = false;
            for (auto index : model.counters)
            {
                const auto& sample = snapshot.totals[index];
                if (model.counter.matches(sample))
                {
                    ImGui::Text("Total %llu %s | maximum/event %llu | %llu events",
                        static_cast<unsigned long long>(sample.value), sample.unit.c_str(),
                        static_cast<unsigned long long>(sample.maximumValue), static_cast<unsigned long long>(sample.calls));
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ImGui::TextDisabled("Selected counter unavailable in retained rows.");
            }
        }
    }
};

Performance::Performance() : _impl(std::make_unique<Impl>())
{
    setTitle("Performance##" + std::to_string(++s_windowIndex));
    setSize(940, 840);
    options.minWidth = 420;
    options.minHeight = 320;
    options.noMenu = true;
    Window::close();
}

Performance::~Performance() = default;

void Performance::open()
{
    if (!_impl->collection)
    {
        _impl->collection = std::make_unique<base::CpuProfiler::Collection>();
    }
    const auto epoch = base::CpuProfiler::session();
    if (_impl->model.preferredEpoch != epoch)
    {
        _impl->model.stage = {};
        _impl->model.counter = {};
        _impl->model.selectedStream = {};
        _impl->model.selectedGpu = {};
        _impl->model.preferredEpoch = epoch;
    }
    _impl->nextRefresh = 0;
    Window::open();
}

void Performance::close()
{
    Window::close();
    _impl->collection.reset();
}

void Performance::draw()
{
    if (!isOpen())
    {
        _impl->collection.reset();
        return;
    }
    if (!_impl->collection)
    {
        open();
    }
    const auto start = timestamp();
    _impl->modalNs = 0;
    {
        base::CpuProfileScope profile("ui.performance");
        Window::draw([this]
        {
            ImGui::PushID(_impl.get());
            _impl->contents();
            ImGui::PopID();
        });
    }
    _impl->drawMs = double(timestamp() - start - _impl->modalNs) / 1e6;
    if (!isOpen())
    {
        _impl->collection.reset();
    }
}
}
