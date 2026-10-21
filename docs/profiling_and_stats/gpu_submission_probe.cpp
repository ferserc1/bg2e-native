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

// CPU-only lifecycle/correlation probe. It does not validate either native GPU.
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/detail/SubmissionTimingState.hpp>
#include <bg2e/gpu/detail/SubmissionState.hpp>

#include <cassert>
#include <cmath>
#include <filesystem>
#include <latch>
#include <thread>

using bg2e::base::CpuProfiler;
using bg2e::base::GpuProfileSample;
using bg2e::base::GpuProfileStatus;
using bg2e::base::GpuProfileWorkload;
using bg2e::gpu::detail::CompletionRecord;
using bg2e::gpu::detail::SubmissionTimingState;
using bg2e::gpu::detail::timingCpuNow;

GpuProfileSample available(GpuProfileSample request)
{
    request.status = GpuProfileStatus::Available;
    request.device = request.queue = request.command = request.submission = 1;
    request.submitted = timingCpuNow();
    request.durationMs = 0.25;
    request.backend = bg2e::base::GpuProfileBackend::Vulkan;
    return request;
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    double ms = 0;
    using bg2e::gpu::detail::timestampDuration;
    assert(timestampDuration(250, 5, 8, 10, 1000, ms));
    assert(std::abs(ms - 0.00011) < 1e-12);
    assert(!timestampDuration(250, 5, 8, 10, 2560, ms));
    assert(!timestampDuration(1, 2, 0, 1, 100, ms));
    assert(timestampDuration(UINT64_MAX - 2, 3, 64, 1, 100, ms));
    assert(std::abs(ms - 0.000006) < 1e-12);

    auto state = std::make_shared<SubmissionTimingState>();
    state->capabilities.supported = true;
    state->capabilities.device = 10;
    state->capabilities.queue = 20;
    GpuProfileSample request;
    request.token = 100;
    request.epoch = 7;
    request.recording = 11;
    request.presentationFrame = 42;
    request.requested = timingCpuNow();
    auto first = state->reserve(request);
    assert(first);
    first->markSubmitted(33);
    first->context(99, GpuProfileWorkload::Scene); // Cannot change submitted identity.
    first->abandon(); // Submitted ownership survives wrapper abandonment.
    first->resolve = [](auto& sample)
    {
        sample.durationMs = 2.5;
        return GpuProfileStatus::Available;
    };
    bool ready = false;
    int waits = 0;
    auto completion = std::make_shared<CompletionRecord>([&]
    {
        return ready;
    }, [&]
    {
        ++waits;
    }, [first](bool success)
    {
        first->complete(success);
    });
    assert(!completion->pollNonblocking());
    assert(state->take().pending == 1);
    ready = true;
    assert(completion->pollNonblocking() && waits == 0);
    auto batch = state->take();
    assert(batch.pending == 0 && batch.results.size() == 1);
    assert(batch.results[0].recording == 11 && batch.results[0].presentationFrame == 42);
    assert(batch.results[0].submission == 33 && batch.results[0].durationMs == 2.5);
    assert(batch.results[0].sceneFrame == 0);

    // A profiling poll must return even if normal application code is waiting
    // on this record's mutex. The latches simulate that CPU concurrency only.
    std::latch entered(1), release(1);
    CompletionRecord waiting([]
    {
        return false;
    }, [&]
    {
        entered.count_down();
        release.wait();
    });
    std::thread waiter([&]
    {
        waiting.wait();
    });
    entered.wait();
    assert(!waiting.pollNonblocking());
    release.count_down();
    waiter.join();

    std::vector<std::shared_ptr<bg2e::gpu::detail::SubmissionTimingTicket>> tickets;
    for (size_t i = 0; i < SubmissionTimingState::capacity; ++i)
    {
        request.token = 200 + i;
        auto ticket = state->reserve(request);
        assert(ticket);
        tickets.push_back(ticket);
    }
    assert(!state->reserve(request));
    tickets.front()->abandon();
    auto replacement = state->reserve(request);
    assert(replacement);
    state->close();
    assert(state->take().pending == 0);
    assert(!state->capabilities.supported);

    auto pressure = std::make_shared<SubmissionTimingState>();
    pressure->capabilities.supported = true;
    for (size_t i = 0; i < 200; ++i)
    {
        auto ticket = pressure->reserve(request);
        assert(ticket);
        ticket->markSubmitted(i + 1);
        ticket->completeNative(GpuProfileStatus::Available, 1);
    }
    batch = pressure->take();
    assert(batch.results.size() == 128 && batch.dropped == 72);

    const std::filesystem::path directory(argv[1]);
    std::filesystem::create_directories(directory);
    assert(!std::filesystem::exists(directory / "gpu_first.jsonl"));
    assert(!std::filesystem::exists(directory / "gpu_second.jsonl"));
    CpuProfiler::Collection live;
    CpuProfiler::setGpuTimingEnabled(false);
    assert(!CpuProfiler::beginGpu(1, 1, GpuProfileWorkload::Scene).token);
    assert(CpuProfiler::enabled());
    CpuProfiler::setGpuTimingEnabled(true);
    const auto preStart = CpuProfiler::beginGpu(1, 1, GpuProfileWorkload::Scene);
    assert(CpuProfiler::start(directory / "gpu_first.jsonl"));
    CpuProfiler::completeGpu(available(preStart)); // Live only, never pre-start data in a file.
    const auto original = CpuProfiler::beginGpu(2, 2, GpuProfileWorkload::SceneAndUi);
    const auto unresolved = CpuProfiler::beginGpu(3, 0, GpuProfileWorkload::Ui);
    auto result = available(original);
    result.recording = 9999; // A backend cannot rewrite the reserved identity.
    result.epoch = 9999;
    CpuProfiler::completeGpu(result);
    CpuProfiler::stop(); // Exactly one unresolved GPU token; no native wait.
    assert(CpuProfiler::start(directory / "gpu_second.jsonl"));
    CpuProfiler::completeGpu(available(unresolved)); // Late previous-session completion.
    const auto current = CpuProfiler::beginGpu(4, 0, GpuProfileWorkload::Auxiliary);
    CpuProfiler::completeGpu(available(current));
    CpuProfiler::stop();
    const auto snapshot = CpuProfiler::snapshot();
    bool oldIdentity = false;
    for (const auto& sample : snapshot.gpuSamples)
    {
        if (sample.token == unresolved.token)
        {
            oldIdentity = sample.recording == original.recording && sample.presentationNumber == 3;
        }
    }
    assert(oldIdentity && snapshot.gpuPending == 0);
    assert(CpuProfiler::snapshot(timingCpuNow()).gpuSamples.empty()); // Reader cutoff never touches a file.
}
