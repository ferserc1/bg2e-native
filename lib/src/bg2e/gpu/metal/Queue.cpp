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
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/base/Log.hpp>

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
    if (_commandQueue)
    {
        _commandQueue->release();
        _commandQueue = nullptr;
    }
}

Queue::Queue(Queue&& other) noexcept
    : _commandQueue(other._commandQueue), _submissions(std::move(other._submissions)), _device(other._device)
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
    _device = device;
    _submissions = device->submissionState();
}

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& debugName) const
{
    if (!_submissions) throw std::logic_error("Metal queue has no device");
    auto admission = _submissions->admit();
    if (!_commandQueue)
    {
        throw std::runtime_error("metal::Queue::createCommandBuffer: queue not initialized");
    }

    MTL::CommandBuffer* mtlCmd = _commandQueue->commandBuffer();
    if (!mtlCmd)
    {
        throw std::runtime_error("metal::Queue::createCommandBuffer: failed to create command buffer");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        mtlCmd->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }

    return std::make_shared<metal::CommandBuffer>(_device, mtlCmd);
}

void Queue::submit(gpu::CommandBuffer* cmd) const
{
    if (!_submissions) throw std::logic_error("Metal queue has no device");
    auto admission = _submissions->admit();
    auto* command = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!command || command->_device != _device || !command->handle() ||
        command->handle()->commandQueue() != _commandQueue)
        throw std::invalid_argument("Metal command buffer belongs to another queue/device");
    if (command->_submitted || command->_recording || !command->_executable)
        throw std::logic_error("Metal command buffer is already submitted or not executable");
    auto* handle = command->handle();
    handle->retain();
    auto native = std::shared_ptr<MTL::CommandBuffer>(handle, [](MTL::CommandBuffer* value) { value->release(); });
    auto record = std::make_shared<detail::CompletionRecord>(
        [native] {
            auto status = native->status();
            if (status == MTL::CommandBufferStatusError)
                throw std::runtime_error("Metal command buffer execution failed");
            return status == MTL::CommandBufferStatusCompleted;
        },
        [native] {
            native->waitUntilCompleted();
            if (native->status() == MTL::CommandBufferStatusError)
                throw std::runtime_error("Metal command buffer execution failed");
        });
    try {
        _submissions->track(record);
        command->_completion = record;
        if (command->_submissionFrame) command->_submissionFrame->trackSubmission(record);
        handle->commit();
    }
    catch (...) { _submissions->cancel(record, std::current_exception()); throw; }
    command->_submitted = true;
    command->_executable = false;
}

#else

Queue::Queue(CommandQueueHandle) {}
void Queue::setDevice(metal::Device* device) { _device = device; _submissions = device->submissionState(); }
Queue::~Queue() {}
Queue::Queue(Queue&&) noexcept {}
Queue& Queue::operator=(Queue&&) noexcept { return *this; }
uint32_t Queue::familyIndex() const { return 0; }
bool Queue::isValid() const { return false; }

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& /*debugName*/) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Queue::submit(gpu::CommandBuffer*) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

}
}
}
