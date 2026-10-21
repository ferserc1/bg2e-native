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

#include <bg2e/gpu/CommandBuffer.hpp>
#include <cassert>
#include <stdexcept>
#include <string>

using namespace bg2e;

class TestCommand : public gpu::CommandBuffer {
  public:
    TestCommand()
    {
        _timingState = std::make_shared<gpu::detail::SubmissionTimingState>();
        _timingState->capabilities.supported = true;
        begin();
    }

    void begin() override
    {
        beginRecordingCompletion();
        gpu::SubmissionTimingRequest request;
        request.detailed = true;
        _submissionTiming = _timingState->reserve(request);
    }
    void end() override
    {
    }
    void transition(gpu::Image*, gpu::ImageLayout) override
    {
    }
    void beginRendering(gpu::SurfaceFrame*) override
    {
    }
    void endRendering() override
    {
    }
    void beginCompute() override
    {
    }
    void endCompute() override
    {
    }
    void clearColor(uint32_t, const gpu::Color&) override
    {
    }
    void clearDepth(float) override
    {
    }
    bool hasActiveScope() const override
    {
        return false;
    }
    bool isValid() const override
    {
        return true;
    }

    auto ticket()
    {
        return _submissionTiming;
    }
    auto take()
    {
        return _timingState->take();
    }
};

int main()
{
    TestCommand command;
    const auto parent = command.beginTimingInterval("parent", 7);
    {
        gpu::GpuTimingScope child(command, "child", 99);
    }
    try
    {
        gpu::GpuTimingScope failure(command, "exception");
        throw std::runtime_error("probe");
    }
    catch (const std::runtime_error&)
    {
    }
    command.endTimingInterval(parent);
    command.endTimingInterval(parent); // Duplicate end is harmless.
    auto ticket = command.ticket();
    assert(ticket->sample.intervals[1].parent == parent);
    assert(ticket->sample.intervals[1].context == 99);
    assert(ticket->sample.intervals[1].ended);
    assert(ticket->sample.intervals[1].status == base::GpuProfileStatus::Unsupported);
    assert(ticket->sample.intervals[2].status == base::GpuProfileStatus::Aborted);
    for (int i = 0; i < 64; ++i)
    {
        gpu::GpuTimingScope detail(command, "layer", i, true);
    }
    assert(ticket->sample.intervalCount == 24); // Eight coarse slots reserved.
    assert(ticket->sample.intervalsDropped == 43);
    const auto missing = command.beginTimingInterval("missing");
    assert(missing == 25);
    ticket->markSubmitted(1);
    assert(command.beginTimingInterval("after submit") == 0);
    ticket->completeNative(base::GpuProfileStatus::Available, 1);
    auto batch = command.take();
    assert(batch.results.size() == 1);
    assert(batch.results[0].intervals[missing - 1].status == base::GpuProfileStatus::Aborted);
    assert(batch.results[0].intervals[0].status == base::GpuProfileStatus::Unsupported);
    command.begin();
    ticket = command.ticket();
    ticket->sample.passTiming = false;
    assert(command.beginTimingInterval("total only") == 0);
    ticket->abandon();
    assert(command.take().results[0].intervalCount == 0);
}
