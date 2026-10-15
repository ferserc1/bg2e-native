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
#include <algorithm>
#include <iostream>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

RenderingCapabilities queryRenderingCapabilities(MTL::Device* device)
{
    RenderingCapabilities result;
    if (!device)
    {
        return result;
    }
    result.maxColorAttachments = 8;
    result.maxImageDimension2D = device->supportsFamily(MTL::GPUFamilyApple2) &&
        !device->supportsFamily(MTL::GPUFamilyApple3) ? 8192 : 16384;
    result.independentBlend = true;
    if (device->supportsFamily(MTL::GPUFamilyApple1))
    {
        result.maxColorAttachmentBytesPerSample = device->supportsFamily(MTL::GPUFamilyApple7) ? 128 :
            (device->supportsFamily(MTL::GPUFamilyApple4) ? 64 : 32);
    }
    return result;
}

FormatCapabilities queryFormatCapabilities(MTL::Device* device, PixelFormat format)
{
    FormatCapabilities result;
    if (!device || toMetalPixelFormat(format) == MTL::PixelFormatInvalid)
    {
        return result;
    }
    if (format == PixelFormat::D24_UNORM_S8_UINT && !device->isDepth24Stencil8PixelFormatSupported())
    {
        return result;
    }
    result.usages = ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst;
    if (isDepthFormat(format))
    {
        result.usages |= ImageUsage::DepthStencil;
        result.linearFiltering = format == PixelFormat::D16_UNORM || format == PixelFormat::D24_UNORM_S8_UINT ||
            !device->supportsFamily(MTL::GPUFamilyApple1) || device->supportsFamily(MTL::GPUFamilyApple9);
        return result;
    }
    result.usages |= ImageUsage::ColorAttachment;
    // Metal sRGB/BGRA formats cannot be shader-write destinations.
    if (format != PixelFormat::R8G8B8A8_SRGB && format != PixelFormat::B8G8R8A8_SRGB &&
        format != PixelFormat::B8G8R8A8_UNORM)
    {
        result.usages |= ImageUsage::Storage;
    }
    const bool integer = isUnsignedIntegerFormat(format) || isSignedIntegerFormat(format);
    const bool float32 = format == PixelFormat::R32_SFLOAT || format == PixelFormat::R32G32_SFLOAT ||
        format == PixelFormat::R32G32B32A32_SFLOAT;
    result.colorAttachmentBytesPerSample = pixelFormatBytes(format);
    if (device->supportsFamily(MTL::GPUFamilyApple1))
    {
        // Small normalized/half formats may expand in tile memory. Use an
        // eight-byte floor; native pipeline creation is the final validator.
        result.colorAttachmentBytesPerSample = std::max(8u, result.colorAttachmentBytesPerSample);
    }
    result.blend = !integer;
    result.linearFiltering = !integer && (!float32 || device->supports32BitFloatFiltering());
    // The native blit encoder copies exact texels; it does not rescale images.
    result.blitSource = false;
    result.blitDestination = false;
    result.mipmapGeneration = result.linearFiltering;
    return result;
}

FormatCapabilities PhysicalDevice::formatCapabilities(PixelFormat format) const
{
    return queryFormatCapabilities(_device, format);
}

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

        props->rendering = queryRenderingCapabilities(mtlDevice);
        props->name = mtlDevice->name()->utf8String();
        props->totalHeapMemoryMB = static_cast<size_t>(
            mtlDevice->recommendedMaxWorkingSetSize() / (1024ULL * 1024ULL));

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
            props->rayTracing.available = true;
            props->rayTracing.rayTracingPipeline = true;
            props->rayTracing.rayQuery = true;
            props->rayTracing.rayQueryInRenderStages = mtlDevice->supportsRaytracingFromRender();
            props->rayTracing.accelerationStructure = true;
            props->rayTracing.bufferDeviceAddress = true;
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

FormatCapabilities PhysicalDevice::formatCapabilities(PixelFormat) const
{
    return {};
}

PhysicalDevice::~PhysicalDevice()
{
}

void PhysicalDevice::choose(gpu::Instance&, gpu::Surface&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

bool PhysicalDevice::isValid() const
{
    return false;
}

const std::shared_ptr<PhysicalDeviceProperties> PhysicalDevice::properties() const
{
    return nullptr;
}

#endif

}
}
}
