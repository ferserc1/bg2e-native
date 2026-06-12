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
    : _commandQueue(other._commandQueue), _device(other._device)
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

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& debugName) const
{
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
    auto* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!mtlCmd)
    {
        throw std::runtime_error("metal::Queue::submit: not a metal::CommandBuffer");
    }

    mtlCmd->handle()->commit();
}

#else

Queue::Queue(CommandQueueHandle) {}
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
