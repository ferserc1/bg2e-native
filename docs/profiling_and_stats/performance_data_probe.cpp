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

// Pure CPU presentation-model probe, outside CMake source discovery.
#include <bg2e/ui/detail/PerformanceData.hpp>

#include <cassert>
#include <cmath>

using bg2e::base::CpuProfileSample;
using bg2e::ui::detail::PerformanceModel;
using bg2e::ui::detail::PerformanceSeries;

void frame(PerformanceModel& model, uint64_t token, uint64_t thread, uint64_t number,
    uint64_t scene, uint64_t end, bool sceneRendered, double ms, bool complete = true)
{
    CpuProfileSample duration;
    duration.kind = CpuProfileSample::Kind::Scope;
    duration.epoch = 1;
    duration.thread = thread;
    duration.id = duration.presentationFrame = token;
    duration.begin = end - uint64_t(ms * 1e6);
    duration.end = end;
    duration.inclusiveMs = duration.maximumMs = ms;
    duration.exclusiveMs = ms / 2;
    duration.complete = complete;
    duration.label = "frame.total";
    duration.unit = "ms";
    model.snapshot.samples.push_back(duration);
    auto marker = duration;
    marker.kind = CpuProfileSample::Kind::Frame;
    marker.presentationNumber = number;
    marker.sceneFrame = scene;
    marker.sceneRendered = sceneRendered;
    model.snapshot.samples.push_back(marker);
}

int main()
{
    PerformanceModel model;
    model.snapshot.begin = 800'000'000;
    frame(model, 101, 1, 1, 1, 1'000'000'000, true, 2);
    frame(model, 102, 1, 2, 1, 1'016'000'000, false, 4);
    frame(model, 103, 1, 3, 2, 1'032'000'000, true, 6);
    frame(model, 105, 1, 5, 3, 1'064'000'000, true, 8); // Missing presentation 4.
    frame(model, 106, 1, 0, 0, 1'080'000'000, false, 10, false);
    frame(model, 107, 1, 6, 3, 1'096'000'000, false, 12);
    frame(model, 201, 2, 50, 10, 1'020'000'000, true, 200); // Concurrent stream.
    CpuProfileSample counter;
    counter.kind = CpuProfileSample::Kind::Counter;
    counter.epoch = counter.thread = 1;
    counter.label = "probe.counter";
    counter.unit = "bytes";
    counter.begin = counter.end = 1'090'000'000;
    counter.value = 2;
    model.snapshot.samples.push_back(counter);
    counter.value = 3;
    ++counter.begin;
    ++counter.end;
    model.snapshot.samples.push_back(counter);
    counter.value = 0;
    counter.begin = counter.end = 1'095'000'000;
    model.snapshot.samples.push_back(counter);
    model.counter.assign(counter);
    model.selectedStream = {1, 1};
    model.rebuild(1'100'000'000);
    assert(model.presentations == 5 && model.sceneFrames == 3 && model.incomplete == 1);
    assert(model.presentationMs.samples() == 2 && model.presentationFps.samples() == 2);
    assert(model.sceneMs.samples() == 2 && model.sceneFps.samples() == 2);
    assert(model.frameMs.samples() == 5); // Never includes the 200 ms other thread.
    assert(model.counterValues.samples() == 3 && model.unassociated == 3);
    bool foundFive = false, foundZero = false, foundGap = false;
    for (size_t i = 0; i < PerformanceSeries::capacity; ++i)
    {
        const auto value = model.counterValues.value(i);
        foundFive |= value == 5;
        foundZero |= value == 0;
        foundGap |= std::isnan(value);
        const auto frameValue = model.frameMs.value(i);
        assert(std::isnan(frameValue) || frameValue <= 12);
        const auto sceneInterval = model.sceneMs.value(i);
        assert(std::isnan(sceneInterval) || sceneInterval == 32); // UI-only frame is not a scene frame.
    }
    assert(foundFive && foundZero && foundGap);
    model.dialog(999'000'000, 1'017'000'000);
    model.rebuild(1'100'000'000);
    assert(model.frameMs.samples() == 3);
    assert(model.presentationMs.samples() == 0 && model.sceneMs.samples() == 1);
    model.selectedStream = {1, 2};
    model.rebuild(1'100'000'000);
    assert(model.presentations == 1 && model.frameMs.samples() == 0); // Long root overlaps modal time.
    assert(model.presentationMs.samples() == 0); // A first marker cannot imply an interval.
    for (uint64_t i = 0; i < 300; ++i)
    {
        CpuProfileSample stage;
        stage.epoch = stage.thread = 1;
        stage.label = "stage." + std::to_string(i);
        stage.unit = "ms";
        stage.exclusiveMs = double(i);
        model.snapshot.totals.push_back(stage);
    }
    model.rows();
    assert(model.stages.size() == 256 && model.omittedStages == 44);
    for (uint64_t i = 3; i < 73; ++i)
    {
        frame(model, 300 + i, i, 1, 1, 1'050'000'000 + i, true, 1);
    }
    model.rebuild(1'100'000'000);
    assert(model.streams.size() == 64 && model.omittedStreams > 0);
    model.preferredEpoch = 2;
    model.selectedStream = {};
    model.stage = {};
    model.counter = {};
    model.rows();
    model.rebuild(1'100'000'000);
    assert(model.selectedStream.thread == 0 && model.stage.thread == 0 && model.counter.thread == 0);
    assert(model.frameMs.samples() == 0); // Old history must not seize a new epoch's default selection.
    for (uint64_t i = 0; i < 17; ++i)
    {
        model.dialog(1'000'000'000 + i, 1'000'000'001 + i);
    }
    assert(model.dialogCount == 16 && model.modalCutoff > 0);
    assert(model.overlapsDialog(1, 2)); // Lost modal history suppresses the ambiguous prefix.
    PerformanceModel gpu;
    gpu.snapshot.begin = 900'000'000;
    bg2e::base::GpuProfileSample submission;
    submission.epoch = submission.device = submission.queue = 1;
    submission.requested = 1'000'000'000;
    submission.resolved = 1'095'000'000;
    submission.status = bg2e::base::GpuProfileStatus::Available;
    submission.workload = bg2e::base::GpuProfileWorkload::Scene;
    submission.durationMs = 4;
    gpu.snapshot.gpuSamples.push_back(submission);
    submission.durationMs = 6;
    submission.resolved = 1'094'000'000;
    gpu.snapshot.gpuSamples.push_back(submission);
    submission.queue = 2;
    submission.durationMs = 100;
    gpu.snapshot.gpuSamples.push_back(submission);
    submission.queue = 1;
    submission.status = bg2e::base::GpuProfileStatus::Unsupported;
    submission.durationMs = 0;
    gpu.snapshot.gpuSamples.push_back(submission);
    gpu.selectedGpu = {1, 1, 1};
    gpu.rebuild(1'100'000'000);
    assert(gpu.gpuMs.samples() == 2 && gpu.gpuAvailable == 2 && gpu.gpuUnavailable == 1);
    assert(gpu.gpuMs.value(300) == 5); // Per-submission mean, not a 10 ms frame sum.
    assert(std::isnan(gpu.gpuMs.value(585))); // Late result is not plotted at resolution time.
    for (auto& sample : gpu.snapshot.gpuSamples)
    {
        sample.intervalCount = 2;
        auto& parent = sample.intervals[0];
        parent.id = 1;
        parent.label[0] = 'P';
        parent.status = bg2e::base::GpuProfileStatus::Available;
        parent.durationMs = 20;
        auto& child = sample.intervals[1];
        child.id = 2;
        child.parent = 1;
        child.label[0] = 'C';
        child.status = bg2e::base::GpuProfileStatus::Available;
        child.durationMs = sample.durationMs / 2;
    }
    gpu.gpuStage = "C";
    gpu.rebuild(1'100'000'000);
    assert(gpu.gpuMs.value(300) == 2.5); // Never add the parent's 20 ms.
    assert(gpu.gpuStages.size() == 2);
    gpu.gpuWorkload = int(bg2e::base::GpuProfileWorkload::Ui);
    gpu.rebuild(1'100'000'000);
    assert(gpu.gpuMs.samples() == 0); // Unavailable never becomes a zero-duration point.

}
