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

#include <bg2e/gpu/metal/Queue.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/base/Log.hpp>

#include "Diagnostics.hpp"

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

Queue::Queue(CommandQueueHandle commandQueue)
    : _commandQueue(commandQueue)
{
}

Queue::~Queue()
{
    bg2e_deep_trace("metal", "queue.destroy", "queue=" << _diagnosticIdentity.value);
    if (_timings)
    {
        _timings->close();
    }
    if (_commandQueue)
    {
        _commandQueue->release();
        _commandQueue = nullptr;
    }
}

Queue::Queue(Queue&& other) noexcept
    : _diagnosticIdentity(other._diagnosticIdentity), _timings(std::move(other._timings)), _commandQueue(other._commandQueue), _submissions(std::move(other._submissions)), _device(other._device)
{
    other._commandQueue = nullptr;
    other._device = nullptr;
}

Queue& Queue::operator=(Queue&& other) noexcept
{
    if (this != &other)
    {
        if (_commandQueue)
        {
            _commandQueue->release();
        }
        if (_timings)
        {
            _timings->close();
        }
        _diagnosticIdentity = other._diagnosticIdentity;
        _timings = std::move(other._timings);
        _commandQueue = other._commandQueue;
        _device = other._device;
        _submissions = std::move(other._submissions);
        other._commandQueue = nullptr;
        other._device = nullptr;
    }
    return *this;
}

uint32_t Queue::familyIndex() const
{
    return 0;
}

bool Queue::isValid() const
{
    return _commandQueue != nullptr;
}

void Queue::setDevice(metal::Device* device)
{
    detail::DiagnosticScope diagnosticScope("metal", "Queue.setDevice", _diagnosticIdentity.value);
    _device = device;
    _submissions = device->submissionState();
    try
    {
        _timings = std::make_shared<detail::SubmissionTimingState>();
        auto& capabilities = _timings->capabilities;
        capabilities.supported = _commandQueue != nullptr;
        capabilities.device = device->timingIdentity();
        capabilities.queue = detail::nextTimingIdentity();
        capabilities.backend = SubmissionTimingBackend::Metal;
        capabilities.description = "Metal completed command-buffer GPUStartTime/GPUEndTime (runtime checked per request)";
    }
    catch (...)
    {
        _timings.reset();
    }
}

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& debugName) const
{
    detail::DiagnosticScope diagnosticScope("metal", "queue.createCommandBuffer", _diagnosticIdentity.value);
    requireDebugName(debugName);
    if (!_submissions)
    {
        throw std::logic_error("Metal queue has no device");
    }
    auto admission = _submissions->admit();
    if (!_commandQueue)
    {
        throw std::runtime_error("metal::Queue::createCommandBuffer: queue not initialized");
    }

    MTL::CommandBuffer* mtlCmd = nullptr;
    if (base::Log::deepDebugEnabled())
    {
        auto* descriptor = MTL::CommandBufferDescriptor::alloc()->init();
        descriptor->setRetainedReferences(true);
        descriptor->setErrorOptions(MTL::CommandBufferErrorOptionEncoderExecutionStatus);
        mtlCmd = _commandQueue->commandBuffer(descriptor);
        descriptor->release();
    }
    else
    {
        mtlCmd = _commandQueue->commandBuffer();
    }
    if (!mtlCmd)
    {
        throw std::runtime_error("metal::Queue::createCommandBuffer: failed to create command buffer");
    }

    if (!debugName.empty())
    {
        mtlCmd->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }

    auto command = std::make_shared<metal::CommandBuffer>(_device, mtlCmd);
    command->_timingState = _timings;
    bg2e_deep_trace("metal", "command.create", "queue=" << _diagnosticIdentity.value
        << " command=" << command->_diagnosticIdentity.value << " name=" << debugName
        << " encoderExecutionStatus=" << base::Log::deepDebugEnabled());
    return command;
}

void Queue::submit(gpu::CommandBuffer* cmd) const
{
    base::CpuProfileScope profile("metal.queue.submit");
    if (!_submissions)
    {
        throw std::logic_error("Metal queue has no device");
    }
    auto admission = _submissions->admit();
    auto* command = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!command || command->_device != _device || !command->handle() ||
        command->handle()->commandQueue() != _commandQueue)
    {
        throw std::invalid_argument("Metal command buffer belongs to another queue/device");
    }
    if (command->_submitted || command->_recording || !command->_executable)
    {
        throw std::logic_error("Metal command buffer is already submitted or not executable");
    }
    struct TimingFailure {
        std::shared_ptr<detail::SubmissionTimingTicket> ticket;
        bool accepted = false;

        ~TimingFailure()
        {
            if (ticket && !accepted)
            {
                ticket->complete(false);
            }
        }
    } timingFailure{command->_submissionTiming};
    const uint64_t submission = base::Log::deepDebugEnabled() ? base::Log::nextDiagnosticIdentity() : 0;
    const uint64_t commandIdentity = command->_diagnosticIdentity.value;
    auto* handle = command->handle();
    handle->retain();
    auto native = std::shared_ptr<MTL::CommandBuffer>(handle, [](MTL::CommandBuffer* value)
                                                      {
                                                          value->release();
                                                      });
    auto record = std::make_shared<detail::CompletionRecord>(
        [native, submission, commandIdentity]
        {
            auto status = native->status();
            if (status == MTL::CommandBufferStatusError)
            {
                throw std::runtime_error("Metal command buffer execution failed");
            }
            return status == MTL::CommandBufferStatusCompleted;
        },
        [native, submission, commandIdentity]
        {
            base::CpuProfileScope waitProfile("metal.queue.waitUntilCompleted");
            base::CpuProfileScope::count("metal.queue.completionWaits", 1);
            bg2e_deep_trace("metal", "submission.wait.enter", "submission=" << submission << " command=" << commandIdentity);
            native->waitUntilCompleted();
            bg2e_deep_trace("metal", "submission.wait.return", "submission=" << submission
                << " command=" << commandIdentity << " status=" << native->status());
            if (native->status() == MTL::CommandBufferStatusError)
            {
                throw std::runtime_error("Metal command buffer execution failed");
            }
        },
        [ticket = command->_submissionTiming](bool success)
        {
            // Success is resolved by the native completed handler. A failed
            // admission/execution may never deliver valid native timestamps.
            if (ticket && !success)
            {
                ticket->complete(false);
            }
        });
    try
    {
        _submissions->track(record);
        if (command->_submissionTiming)
        {
            command->_submissionTiming->markSubmitted(record->id);
        }
        command->_completion = record;
        if (command->_submissionFrame)
        {
            command->_submissionFrame->trackSubmission(record);
        }
        if (base::Log::deepDebugEnabled())
        {
            handle->addCompletedHandler(MTL::HandlerFunction([submission, commandIdentity](MTL::CommandBuffer* completed)
            {
                logCommandBufferCompletion(completed, submission, commandIdentity);
            }));
        }
        bg2e_deep_trace("metal", "submission.commit.enter", "queue=" << _diagnosticIdentity.value
            << " submission=" << submission << " record=" << record->id << " command=" << commandIdentity
            << " surface=" << command->_diagnosticSurface << " frame=" << command->_diagnosticFrame
            << " slot=" << command->_diagnosticSlot);
        handle->commit();
        bg2e_deep_trace("metal", "submission.commit.return", "submission=" << submission << " command=" << commandIdentity);
    }
    catch (...)
    {
        _submissions->cancel(record, std::current_exception());
        throw;
    }
    timingFailure.accepted = true;
    command->publishRecordingSubmission(record);
    command->_submitted = true;
    command->_executable = false;
}

#else

Queue::Queue(CommandQueueHandle)
{
}

void Queue::setDevice(metal::Device* device)
{
    detail::DiagnosticScope diagnosticScope("metal", "Queue.setDevice", _diagnosticIdentity.value);
    _device = device;
    _submissions = device->submissionState();
}

Queue::~Queue()
{
    bg2e_deep_trace("metal", "queue.destroy", "queue=" << _diagnosticIdentity.value);
}

Queue::Queue(Queue&&) noexcept
{
}

Queue& Queue::operator=(Queue&&) noexcept
{
    return *this;
}

uint32_t Queue::familyIndex() const
{
    return 0;
}

bool Queue::isValid() const
{
    return false;
}

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& /*debugName*/) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Queue::submit(gpu::CommandBuffer*) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

SubmissionTimingCapabilities Queue::submissionTimingCapabilities() const noexcept
{
    return _timings ? _timings->capabilities : SubmissionTimingCapabilities{};
}

SubmissionTimingBatch Queue::pollSubmissionTimings() const noexcept
{
    if (_submissions)
    {
        _submissions->pollNonblocking();
    }
    return _timings ? _timings->take() : SubmissionTimingBatch{};
}


}
}
}
