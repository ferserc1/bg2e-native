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

#include <bg2e/gpu/metal/PhysicalDevice.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <stdexcept>
#include <iostream>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

PhysicalDevice::~PhysicalDevice()
{
    if (_device)
    {
        _device->release();
        _device = nullptr;
    }
}

void PhysicalDevice::choose(gpu::Instance& /*instance*/, gpu::Surface& /*surface*/)
{
    NS::Array* devices = MTL::CopyAllDevices();
    if (!devices || devices->count() == 0)
    {
        throw std::runtime_error("No Metal-compatible GPU found");
    }

    MTL::Device* bestDevice = nullptr;
    PhysicalDeviceProperties* bestProps = nullptr;

    for (NS::UInteger i = 0; i < devices->count(); ++i)
    {
        auto* mtlDevice = static_cast<MTL::Device*>(devices->object(i));
        auto* props = PhysicalDeviceProperties::create();

        props->name = mtlDevice->name()->utf8String();
        props->totalHeapMemoryMB = static_cast<size_t>(
            mtlDevice->recommendedMaxWorkingSetSize() / (1024ULL * 1024ULL)
        );

        if (mtlDevice->isRemovable())
        {
            props->deviceType = PhysicalDeviceProperties::VirtualGPU;
        }
        else if (mtlDevice->isLowPower())
        {
            props->deviceType = PhysicalDeviceProperties::IntegratedGPU;
        }
        else
        {
            props->deviceType = PhysicalDeviceProperties::DiscreteGPU;
        }

        if (mtlDevice->supportsRaytracing())
        {
            props->rayTracing.available            = true;
            props->rayTracing.rayTracingPipeline   = true;
            props->rayTracing.rayQuery             = true;
            props->rayTracing.accelerationStructure = true;
            props->rayTracing.bufferDeviceAddress  = true;
        }

        if (bestDevice == nullptr || props->getScore() > bestProps->getScore())
        {
            delete bestProps;
            bestProps = props;
            bestDevice = mtlDevice;
        }
        else
        {
            delete props;
        }
    }

    bestDevice->retain();
    _device = bestDevice;
    _properties = std::shared_ptr<PhysicalDeviceProperties>(bestProps);

    std::cout << "Selected GPU (Metal): " << _properties->name << std::endl;

    devices->release();
}

bool PhysicalDevice::isValid() const
{
    return _device != nullptr;
}

const std::shared_ptr<PhysicalDeviceProperties> PhysicalDevice::properties() const
{
    return _properties;
}

#else

PhysicalDevice::~PhysicalDevice() {}

void PhysicalDevice::choose(gpu::Instance&, gpu::Surface&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

bool PhysicalDevice::isValid() const { return false; }

const std::shared_ptr<PhysicalDeviceProperties> PhysicalDevice::properties() const
{
    return nullptr;
}

#endif

}
}
}
