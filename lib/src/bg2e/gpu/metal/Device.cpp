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

#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/PhysicalDevice.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/gpu/Surface.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

void Device::create(gpu::Instance* /*instance*/, gpu::PhysicalDevice* physicalDevice, gpu::Surface* surface)
{
    auto* metalPhysDevice = dynamic_cast<metal::PhysicalDevice*>(physicalDevice);
    if (!metalPhysDevice || !metalPhysDevice->isValid())
    {
        throw std::runtime_error("metal::Device::create: invalid PhysicalDevice");
    }

    _device = metalPhysDevice->metalDevice();
    _device->retain();

    auto* gfxQueue      = _device->newCommandQueue();
    auto* presentQueue  = _device->newCommandQueue();
    auto* transferQueue = _device->newCommandQueue();

    if (!gfxQueue || !presentQueue || !transferQueue)
    {
        throw std::runtime_error("metal::Device::create: failed to create command queues");
    }

    _graphicsQueue = metal::Queue(gfxQueue);
    _presentQueue  = metal::Queue(presentQueue);
    _transferQueue = metal::Queue(transferQueue);

    _graphicsQueue.setDevice(this);
    _presentQueue.setDevice(this);
    _transferQueue.setDevice(this);

    if (surface)
    {
        surface->createRenderTarget(this, physicalDevice);
    }
}

void Device::cleanup()
{
    _graphicsQueue = metal::Queue();
    _presentQueue  = metal::Queue();
    _transferQueue = metal::Queue();

    if (_device)
    {
        _device->release();
        _device = nullptr;
    }
}

void Device::waitIdle()
{
    // Metal has no device-level wait-idle; completion is tracked per command buffer.
}

bool Device::isValid() const
{
    return _device != nullptr;
}

const gpu::Queue& Device::graphicsQueue() const { return _graphicsQueue; }
const gpu::Queue& Device::presentQueue()  const { return _presentQueue;  }
const gpu::Queue& Device::transferQueue() const { return _transferQueue; }

#else

void Device::create(gpu::Instance*, gpu::PhysicalDevice*, gpu::Surface*)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Device::cleanup() {}
void Device::waitIdle() {}
bool Device::isValid() const { return false; }

const gpu::Queue& Device::graphicsQueue() const { return _graphicsQueue; }
const gpu::Queue& Device::presentQueue()  const { return _presentQueue;  }
const gpu::Queue& Device::transferQueue() const { return _transferQueue; }

#endif

}
}
}
