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

#pragma once

#include <bg2e/base/CpuProfile.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace bg2e::ui::detail {
// Private telemetry presentation model. No ImGui or native resource dependencies.
struct PerformanceKey {
    uint64_t epoch = 0, thread = 0;
    std::string label, unit;

    bool matches(const base::CpuProfileSample& sample) const
    {
        return epoch == sample.epoch && thread == sample.thread &&
            label == sample.label && unit == sample.unit;
    }

    void assign(const base::CpuProfileSample& sample)
    {
        epoch = sample.epoch;
        thread = sample.thread;
        label = sample.label;
        unit = sample.unit;
    }
};

struct PerformanceSeries {
    static constexpr size_t capacity = 600;
    std::array<double, capacity> sums{};
    std::array<uint32_t, capacity> counts{};
    bool sumEvents = false;

    void clear()
    {
        sums.fill(0);
        counts.fill(0);
    }

    float value(size_t index) const
    {
        return counts[index] ? float(sumEvents ? sums[index] : sums[index] / counts[index]) :
            std::numeric_limits<float>::quiet_NaN();
    }

    size_t samples() const
    {
        size_t result = 0;
        for (auto count : counts)
        {
            result += count;
        }
        return result;
    }
};

struct PerformanceModel {
    static constexpr size_t rowLimit = 256;
    static constexpr size_t streamLimit = 64;
    struct Interval {
        uint64_t begin = 0, end = 0;
    };
    struct Frame {
        const base::CpuProfileSample* marker = nullptr;
        const base::CpuProfileSample* duration = nullptr;
    };
    struct Stream {
        uint64_t epoch = 0, thread = 0;
    };
    struct GpuStream {
        uint64_t epoch = 0, device = 0, queue = 0;
    };
    base::CpuProfileSnapshot snapshot;
    GpuStream selectedGpu;
    std::vector<GpuStream> gpuStreams;
    std::vector<size_t> gpuRows;
    int gpuWorkload = -1;
    std::string gpuStage;
    std::vector<std::string> gpuStages;
    size_t omittedGpuStages = 0;
    size_t omittedGpuStreams = 0, gpuAvailable = 0, gpuUnavailable = 0;
    PerformanceKey stage, counter;
    Stream selectedStream;
    std::vector<size_t> stages, counters;
    std::vector<Frame> frames;
    std::vector<Stream> streams;
    std::array<Interval, 16> dialogs{};
    size_t dialogCount = 0, dialogCursor = 0;
    size_t omittedStages = 0, omittedCounters = 0, omittedStreams = 0;
    size_t unassociated = 0, modalSamples = 0;
    size_t presentations = 0, sceneFrames = 0, incomplete = 0;
    uint64_t begin = 0, end = 0, modalCutoff = 0, preferredEpoch = 0;
    PerformanceSeries presentationMs, presentationFps, sceneMs, sceneFps;
    PerformanceSeries frameMs, stageInclusive, stageExclusive, counterValues, gpuMs;

    PerformanceModel()
    {
        stages.reserve(65536);
        counters.reserve(65536);
        frames.reserve(65536);
        streams.reserve(streamLimit);
        gpuStreams.reserve(streamLimit);
        gpuRows.reserve(512);
        counterValues.sumEvents = true;
    }

    bool overlapsDialog(uint64_t first, uint64_t last) const
    {
        if (modalCutoff && first <= modalCutoff)
        {
            return true;
        }
        for (size_t i = 0; i < dialogCount; ++i)
        {
            if (first <= dialogs[i].end && last >= dialogs[i].begin)
            {
                return true;
            }
        }
        return false;
    }

    void dialog(uint64_t first, uint64_t last)
    {
        if (dialogCount == dialogs.size())
        {
            modalCutoff = std::max(modalCutoff, dialogs[dialogCursor].end);
        }
        dialogs[dialogCursor] = {first, last};
        dialogCursor = (dialogCursor + 1) % dialogs.size();
        dialogCount = std::min(dialogCount + 1, dialogs.size());
    }

    void add(PerformanceSeries& series, uint64_t timestamp, double value)
    {
        if (end <= begin || timestamp < begin || timestamp > end || !std::isfinite(value))
        {
            return;
        }
        const auto index = std::min(PerformanceSeries::capacity - 1,
            size_t(double(timestamp - begin) / double(end - begin) * PerformanceSeries::capacity));
        series.sums[index] += value;
        ++series.counts[index];
    }

    void rows()
    {
        stages.clear();
        counters.clear();
        for (size_t i = 0; i < snapshot.totals.size(); ++i)
        {
            const auto kind = snapshot.totals[i].kind;
            if (kind == base::CpuProfileSample::Kind::Scope)
            {
                stages.push_back(i);
            }
            else if (kind == base::CpuProfileSample::Kind::Counter)
            {
                counters.push_back(i);
            }
        }
        const auto limit = [&](auto& indices, bool scopes)
        {
            const auto kept = std::min(rowLimit, indices.size());
            std::partial_sort(indices.begin(), indices.begin() + kept, indices.end(), [&](size_t a, size_t b)
            {
                const auto& left = snapshot.totals[a];
                const auto& right = snapshot.totals[b];
                if ((left.epoch == preferredEpoch) != (right.epoch == preferredEpoch))
                {
                    return left.epoch == preferredEpoch;
                }
                return scopes ? left.exclusiveMs > right.exclusiveMs : left.value > right.value;
            });
            const auto omitted = indices.size() - kept;
            indices.resize(kept);
            return omitted;
        };
        omittedStages = limit(stages, true);
        omittedCounters = limit(counters, false);
        const auto choose = [&](auto& key, const auto& indices)
        {
            if (!key.thread)
            {
                for (auto index : indices)
                {
                    const auto& sample = snapshot.totals[index];
                    if (!preferredEpoch || sample.epoch == preferredEpoch)
                    {
                        key.assign(sample);
                        break;
                    }
                }
            }
        };
        choose(stage, stages);
        choose(counter, counters);
    }

    void rebuild(uint64_t now)
    {
        uint64_t oldest = snapshot.begin;
        for (const auto& sample : snapshot.gpuSamples)
        {
            oldest = oldest ? std::min(oldest, sample.requested) : sample.requested;
        }
        begin = std::max(oldest, now > 30'000'000'000 ? now - 30'000'000'000 : 0);
        end = now;
        presentationMs.clear();
        presentationFps.clear();
        sceneMs.clear();
        sceneFps.clear();
        frameMs.clear();
        stageInclusive.clear();
        stageExclusive.clear();
        counterValues.clear();
        gpuMs.clear();
        frames.clear();
        streams.clear();
        omittedStreams = unassociated = modalSamples = 0;
        presentations = sceneFrames = incomplete = 0;
        for (const auto& sample : snapshot.samples)
        {
            if (sample.kind == base::CpuProfileSample::Kind::Frame)
            {
                frames.push_back({&sample, nullptr});
            }
        }
        gpuStreams.clear();
        gpuRows.clear();
        omittedGpuStreams = gpuAvailable = gpuUnavailable = 0;
        for (size_t i = 0; i < snapshot.gpuSamples.size(); ++i)
        {
            gpuRows.push_back(i);
        }
        std::sort(gpuRows.begin(), gpuRows.end(), [&](size_t a, size_t b)
        {
            return snapshot.gpuSamples[a].resolved > snapshot.gpuSamples[b].resolved;
        });
        for (auto index : gpuRows)
        {
            const auto& sample = snapshot.gpuSamples[index];
            const bool exists = std::any_of(gpuStreams.begin(), gpuStreams.end(), [&](const auto& stream)
            {
                return stream.epoch == sample.epoch && stream.device == sample.device && stream.queue == sample.queue;
            });
            if (!exists && gpuStreams.size() < streamLimit)
            {
                gpuStreams.push_back({sample.epoch, sample.device, sample.queue});
            }
            else if (!exists)
            {
                ++omittedGpuStreams;
            }
        }
        if (!selectedGpu.epoch)
        {
            for (const auto& stream : gpuStreams)
            {
                if (!preferredEpoch || stream.epoch == preferredEpoch)
                {
                    selectedGpu = stream;
                    break;
                }
            }
        }
        gpuStages.clear();
        omittedGpuStages = 0;
        for (const auto& sample : snapshot.gpuSamples)
        {
            for (uint32_t i = 0; i < sample.intervalCount; ++i)
            {
                const std::string label(sample.intervals[i].label.data());
                if (std::find(gpuStages.begin(), gpuStages.end(), label) == gpuStages.end())
                {
                    if (gpuStages.size() < 128)
                    {
                        gpuStages.push_back(label);
                    }
                    else
                    {
                        ++omittedGpuStages;
                    }
                }
            }
        }
        for (const auto& sample : snapshot.gpuSamples)
        {
            if (sample.epoch != selectedGpu.epoch || sample.device != selectedGpu.device || sample.queue != selectedGpu.queue ||
                (gpuWorkload >= 0 && int(sample.workload) != gpuWorkload))
            {
                continue;
            }
            if (sample.status != base::GpuProfileStatus::Available)
            {
                ++gpuUnavailable;
                continue;
            }
            ++gpuAvailable;
            if (!overlapsDialog(sample.requested, sample.requested))
            {
                if (gpuStage.empty())
                {
                    add(gpuMs, sample.requested, sample.durationMs);
                }
                else
                {
                    for (uint32_t i = 0; i < sample.intervalCount; ++i)
                    {
                        const auto& interval = sample.intervals[i];
                        if (gpuStage == interval.label.data() && interval.status == base::GpuProfileStatus::Available)
                        {
                            add(gpuMs, sample.requested, interval.durationMs);
                        }
                    }
                }
            }
        }
        if (gpuRows.size() > 128)
        {
            gpuRows.resize(128);
        }
        std::sort(frames.begin(), frames.end(), [](const auto& a, const auto& b)
        {
            return a.marker->presentationFrame < b.marker->presentationFrame;
        });
        for (const auto& sample : snapshot.samples)
        {
            if (sample.kind == base::CpuProfileSample::Kind::Frame)
            {
                continue;
            }
            if (!sample.presentationFrame)
            {
                ++unassociated;
            }
            if (overlapsDialog(sample.begin, sample.end))
            {
                ++modalSamples;
                continue;
            }
            if (sample.kind == base::CpuProfileSample::Kind::Scope)
            {
                if (sample.id == sample.presentationFrame)
                {
                    const auto frame = std::lower_bound(frames.begin(), frames.end(), sample.id,
                        [](const auto& item, uint64_t id)
                        {
                            return item.marker->presentationFrame < id;
                        });
                    if (frame != frames.end() && frame->marker->presentationFrame == sample.id)
                    {
                        frame->duration = &sample;
                    }
                }
                if (stage.matches(sample) && sample.complete)
                {
                    add(stageInclusive, sample.end, sample.inclusiveMs);
                    add(stageExclusive, sample.end, sample.exclusiveMs);
                }
            }
            else if (counter.matches(sample))
            {
                add(counterValues, sample.end, double(sample.value));
            }
        }
        // Latest streams get the bounded selector slots first.
        for (auto it = frames.rbegin(); it != frames.rend(); ++it)
        {
            const Stream stream{it->marker->epoch, it->marker->thread};
            const bool exists = std::any_of(streams.begin(), streams.end(), [&](const auto& item)
            {
                return item.epoch == stream.epoch && item.thread == stream.thread;
            });
            if (!exists && streams.size() < streamLimit)
            {
                streams.push_back(stream);
            }
            else if (!exists)
            {
                ++omittedStreams;
            }
        }
        if (!selectedStream.thread)
        {
            for (const auto& stream : streams)
            {
                if (!preferredEpoch || stream.epoch == preferredEpoch)
                {
                    selectedStream = stream;
                    break;
                }
            }
        }
        std::sort(frames.begin(), frames.end(), [](const auto& a, const auto& b)
        {
            return a.marker->end < b.marker->end;
        });
        const base::CpuProfileSample* previous = nullptr;
        const base::CpuProfileSample* previousScene = nullptr;
        for (const auto& frame : frames)
        {
            const auto& marker = *frame.marker;
            if (marker.thread != selectedStream.thread || marker.epoch != selectedStream.epoch)
            {
                continue;
            }
            if (!marker.complete)
            {
                ++incomplete;
                previous = previousScene = nullptr;
                continue;
            }
            ++presentations;
            if (frame.duration && frame.duration->complete)
            {
                add(frameMs, marker.end, frame.duration->inclusiveMs);
            }
            if (previous && marker.presentationNumber == previous->presentationNumber + 1 &&
                marker.end > previous->end && !overlapsDialog(previous->end, marker.end))
            {
                const auto ms = double(marker.end - previous->end) / 1e6;
                add(presentationMs, marker.end, ms);
                add(presentationFps, marker.end, 1000.0 / ms);
            }
            previous = &marker;
            if (marker.sceneRendered)
            {
                ++sceneFrames;
                if (previousScene && marker.sceneFrame == previousScene->sceneFrame + 1 &&
                    marker.end > previousScene->end && !overlapsDialog(previousScene->end, marker.end))
                {
                    const auto ms = double(marker.end - previousScene->end) / 1e6;
                    add(sceneMs, marker.end, ms);
                    add(sceneFps, marker.end, 1000.0 / ms);
                }
                previousScene = &marker;
            }
        }
    }
};
}
