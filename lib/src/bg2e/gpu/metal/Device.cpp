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
#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/PhysicalDevice.hpp>
#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/ComputePipeline.hpp>
#include <bg2e/gpu/metal/Sampler.hpp>
#include <bg2e/gpu/metal/ResourceSet.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>
#include <memory>

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

    if (base::Log::isDebug())
    {
        gfxQueue->setLabel(NS::String::string("Graphics Queue", NS::UTF8StringEncoding));
        presentQueue->setLabel(NS::String::string("Present Queue", NS::UTF8StringEncoding));
        transferQueue->setLabel(NS::String::string("Transfer Queue", NS::UTF8StringEncoding));
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

std::shared_ptr<gpu::ShaderModule> Device::createShaderModule(const gpu::ShaderModuleDescription& description)
{
    return std::make_shared<metal::ShaderModule>(this, _device, description);
}

std::shared_ptr<gpu::PipelineLayout> Device::createPipelineLayout(const gpu::PipelineLayoutDescription& description)
{
    return std::make_shared<metal::PipelineLayout>(this, description);
}

std::shared_ptr<gpu::GraphicsPipeline> Device::createGraphicsPipeline(const gpu::GraphicsPipelineDescription& description)
{
    return std::make_shared<metal::GraphicsPipeline>(this, _device, description);
}

std::shared_ptr<gpu::ComputePipeline> Device::createComputePipeline(const gpu::ComputePipelineDescription& description)
{
    return std::make_shared<metal::ComputePipeline>(this, _device, description);
}

std::shared_ptr<gpu::Sampler> Device::createSampler(const gpu::SamplerDescription& description)
{
    return std::make_shared<metal::Sampler>(this, _device, description);
}

std::shared_ptr<gpu::ResourceSet> Device::createResourceSet(gpu::PipelineLayout* layout, uint32_t setIndex, const std::string& /*debugName*/)
{
    auto* metalLayout = dynamic_cast<metal::PipelineLayout*>(layout);
    if (!metalLayout)
    {
        throw std::runtime_error("metal::Device::createResourceSet: invalid PipelineLayout");
    }
    return std::make_shared<metal::ResourceSet>(this, metalLayout, setIndex);
}

std::shared_ptr<gpu::Image> Device::createImage(const ImageDescription& description)
{
    auto img = std::make_shared<metal::Image>(this);
    if (isDepthFormat(description.format))
    {
        img->buildDepthImage(this, description.size, description.format, description.debugName);
    }
    else if (hasFlag(description.usage, ImageUsage::Storage))
    {
        img->buildStorageImage(this, description.size, description.format, description.debugName);
    }
    else
    {
        img->buildSampledImage(this, description.size, description.format, description.debugName);
    }
    return img;
}

std::shared_ptr<gpu::Buffer> Device::createBuffer(const std::string& debugName)
{
    return std::make_shared<metal::Buffer>(this, debugName);
}

void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&& function)
{
    auto cmdSP = _graphicsQueue.createCommandBuffer();
    auto* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmdSP.get());
    if (!mtlCmd)
    {
        throw std::runtime_error("metal::Device::immediateSubmit: unexpected command buffer type");
    }

    cmdSP->begin();
    function(cmdSP.get());
    cmdSP->end();

    mtlCmd->handle()->commit();
    mtlCmd->handle()->waitUntilCompleted();
}

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

std::shared_ptr<gpu::ShaderModule> Device::createShaderModule(const gpu::ShaderModuleDescription& description)
{
    return std::make_shared<metal::ShaderModule>(this, nullptr, description);
}

std::shared_ptr<gpu::PipelineLayout> Device::createPipelineLayout(const gpu::PipelineLayoutDescription& description)
{
    return std::make_shared<metal::PipelineLayout>(this, description);
}

std::shared_ptr<gpu::GraphicsPipeline> Device::createGraphicsPipeline(const gpu::GraphicsPipelineDescription& description)
{
    return std::make_shared<metal::GraphicsPipeline>(this, nullptr, description);
}

std::shared_ptr<gpu::ComputePipeline> Device::createComputePipeline(const gpu::ComputePipelineDescription& description)
{
    return std::make_shared<metal::ComputePipeline>(this, nullptr, description);
}

std::shared_ptr<gpu::Sampler> Device::createSampler(const gpu::SamplerDescription& description)
{
    return std::make_shared<metal::Sampler>(this, nullptr, description);
}

std::shared_ptr<gpu::ResourceSet> Device::createResourceSet(gpu::PipelineLayout* layout, uint32_t setIndex, const std::string& /*debugName*/)
{
    auto* metalLayout = dynamic_cast<metal::PipelineLayout*>(layout);
    if (!metalLayout)
    {
        throw std::runtime_error("metal::Device::createResourceSet: invalid PipelineLayout");
    }
    return std::make_shared<metal::ResourceSet>(this, metalLayout, setIndex);
}

std::shared_ptr<gpu::Image> Device::createImage(const ImageDescription&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

std::shared_ptr<gpu::Buffer> Device::createBuffer(const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

}
}
}
