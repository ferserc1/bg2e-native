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

#pragma once

#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/vk/Queue.hpp>

#include <functional>

namespace bg2e {
namespace gpu {
namespace vk {

class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice, gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    std::shared_ptr<gpu::ShaderModule> createShaderModule(const gpu::ShaderModuleDescription& description) override;
    std::shared_ptr<gpu::PipelineLayout> createPipelineLayout(const gpu::PipelineLayoutDescription& description) override;
    std::shared_ptr<gpu::GraphicsPipeline> createGraphicsPipeline(const gpu::GraphicsPipelineDescription& description) override;
    std::shared_ptr<gpu::ComputePipeline> createComputePipeline(const gpu::ComputePipelineDescription& description) override;
    std::shared_ptr<gpu::Sampler> createSampler(const gpu::SamplerDescription& description) override;
    std::shared_ptr<gpu::ResourceSet> createResourceSet(gpu::PipelineLayout* layout, uint32_t setIndex, const std::string& debugName = {}) override;
    std::shared_ptr<gpu::Image> createImage(const ImageDescription& description) override;
    std::shared_ptr<gpu::CubeMap> createCubeMap(const gpu::CubeMapDescription& description) override;
    std::shared_ptr<gpu::Buffer> createBuffer(const std::string& debugName = {}) override;

    std::shared_ptr<gpu::RayTracingMesh> createRayTracingMesh(const RayTracingMeshDescription& description) override;
    std::shared_ptr<gpu::RayTracingScene> createRayTracingScene(const std::string& debugName = {}) override;
    std::shared_ptr<gpu::RayTracingPipeline> createRayTracingPipeline(const gpu::RayTracingPipelineDescription& description) override;

    void immediateSubmit(std::function<void(gpu::CommandBuffer* cmd)>&& function) override;

    VkDevice handle() const { return _device; }
    VmaAllocator allocator() const { return _allocator; }
    VkPhysicalDevice physicalDeviceHandle() const { return _physicalDevice; }

    // True when the device was created with the ray tracing extensions enabled.
    bool     rayTracingEnabled() const { return _rayTracingEnabled; }
    // Minimum scratch buffer device-address alignment for acceleration builds.
    uint32_t accelerationStructureScratchAlignment() const { return _asScratchAlignment; }

private:
    VkDevice _device{VK_NULL_HANDLE};
    VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
    VmaAllocator _allocator{VK_NULL_HANDLE};
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;
    vk::Queue _transferQueue;

    VkCommandPool   _immediateCmdPool   = VK_NULL_HANDLE;
    VkCommandBuffer _immediateCmdBuffer = VK_NULL_HANDLE;
    VkFence         _immediateCmdFence  = VK_NULL_HANDLE;

    bool     _rayTracingEnabled  = false;
    uint32_t _asScratchAlignment = 256;
};

}
}
}
