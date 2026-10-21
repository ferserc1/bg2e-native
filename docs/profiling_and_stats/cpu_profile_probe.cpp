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

// Standalone verification probe; intentionally outside CMake source discovery.
#include <bg2e/base/CpuProfile.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using bg2e::base::CpuProfiler;
using bg2e::base::CpuProfileScope;
using bg2e::base::CpuProfileSample;

void frame(uint64_t number, bool scene)
{
    CpuProfileScope root("probe.frame", true);
    CpuProfileScope stage("probe.first");
    {
        CpuProfileScope child("probe.child");
        CpuProfileScope::count("probe.calls", 2);
        CpuProfileScope::count("probe.calls", 3);
    }
    stage.next("probe.second");
    root.frameCompleted(number, scene ? 1 : 0, scene);
}

void workload(size_t iterations)
{
    for (size_t i = 0; i < iterations; ++i)
    {
        CpuProfileScope scope("probe.pressure");
    }
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    const std::filesystem::path directory(argv[1]);
    std::filesystem::create_directories(directory);
    const auto first = directory / "first.jsonl";
    const auto second = directory / "second.jsonl";
    assert(!std::filesystem::exists(first) && !std::filesystem::exists(second));
    assert(!CpuProfiler::enabled());
    const auto disabledStart = std::chrono::steady_clock::now();
    workload(10000);
    const auto disabledEnd = std::chrono::steady_clock::now();
    {
        CpuProfiler::Collection live;
        {
            CpuProfiler::Collection anotherConsumer;
        }
        assert(CpuProfiler::enabled() && !CpuProfiler::recording());
        frame(1, true);
        frame(2, false);
        try
        {
            CpuProfileScope aborted("probe.aborted", true);
            throw std::runtime_error("intentional unwind");
        }
        catch (const std::runtime_error&)
        {
        }
        std::thread worker([]
        {
            CpuProfiler::count("probe.worker", 4096, "bytes");
            CpuProfiler::count("probe.worker", 1024, "bytes");
        });
        worker.join(); // Thread exit publishes counters without frame scopes.
        const auto snapshot = CpuProfiler::snapshot();
        assert(snapshot.presentationFrames == 2 && snapshot.sceneFrames == 1);
        assert(snapshot.incompleteFrames == 1);
        auto child = std::find_if(snapshot.samples.begin(), snapshot.samples.end(), [](const auto& sample)
        {
            return sample.label == "probe.child";
        });
        assert(child != snapshot.samples.end() && child->parent && child->presentationFrame);
        assert(child->inclusiveMs >= child->exclusiveMs && child->exclusiveMs >= 0);
        auto counts = std::find_if(snapshot.totals.begin(), snapshot.totals.end(), [](const auto& sample)
        {
            return sample.label == "probe.calls";
        });
        assert(counts != snapshot.totals.end() && counts->value == 10 && counts->maximumValue == 3);
        assert(!CpuProfiler::start(directory / "missing" / "bad.jsonl"));
        assert(CpuProfiler::enabled() && !CpuProfiler::recording());
        {
            CpuProfileScope crossing("probe.crossing");
            assert(CpuProfiler::start(first));
            const auto session = CpuProfiler::recordingSession();
            assert(!CpuProfiler::start(second));
            assert(CpuProfiler::recordingSession() == session);
            frame(3, true);
            CpuProfiler::count("probe.final", 7);
            CpuProfiler::stop(); // Drains the final counter, no 2-second interval.
            assert(CpuProfiler::enabled() && !CpuProfiler::recording());
            assert(CpuProfiler::start(second));
            frame(4, false);
            CpuProfiler::stop();
        }
        assert(!CpuProfiler::start(first)); // Never silently overwrite.
        const auto liveStart = std::chrono::steady_clock::now();
        workload(10000);
        const auto liveEnd = std::chrono::steady_clock::now();
        std::cout << "disabled_ms=" << std::chrono::duration<double, std::milli>(disabledEnd - disabledStart).count()
                  << " live_ms=" << std::chrono::duration<double, std::milli>(liveEnd - liveStart).count() << '\n';
        workload(70000);
        const auto pressure = CpuProfiler::snapshot();
        assert(pressure.samples.size() <= 512 * 128 && pressure.evicted > 0);
    }
    assert(!CpuProfiler::enabled());
    assert(CpuProfiler::start(directory / "recording_only.jsonl"));
    frame(5, true);
    CpuProfiler::stop();
    assert(!CpuProfiler::enabled());
}
