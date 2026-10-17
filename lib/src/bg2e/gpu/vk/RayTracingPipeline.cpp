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

#include <bg2e/gpu/vk/RayTracingPipeline.hpp>
#include <bg2e/gpu/vk/ShaderModule.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/extensions.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::gpu::vk {
namespace {
uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    if (!alignment)
    {
        throw std::runtime_error("Invalid shader binding table alignment");
    }
    return (value + alignment - 1) / alignment * alignment;
}
}

RayTracingPipeline::RayTracingPipeline(gpu::Device* gpuDevice, VkDevice device, VmaAllocator allocator,
    const gpu::RayTracingPipelineDescription& description)
    : gpu::RayTracingPipeline(gpuDevice), _device(device), _allocator(allocator)
{
    try
    {
        auto* nativeDevice = dynamic_cast<vk::Device*>(gpuDevice);
        auto* layout = dynamic_cast<vk::PipelineLayout*>(description.layout);
        if (!nativeDevice || !gpuDevice->rayTracingCapabilities().pipelineSupported() ||
            !createRayTracingPipelines || !getRayTracingShaderGroupHandles || !layout || layout->device() != gpuDevice || !layout->isValid() || !description.raygenShader)
        {
            throw std::invalid_argument("Vulkan tracing requires a supported device, layout and raygen shader");
        }
        _layoutHandle = layout->handle();
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 deviceProperties{};
        deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties.pNext = &properties;
        vkGetPhysicalDeviceProperties2(nativeDevice->physicalDeviceHandle(), &deviceProperties);
        if (!description.maxRecursionDepth || description.maxRecursionDepth > properties.maxRayRecursionDepth)
        {
            throw std::invalid_argument("Ray recursion depth exceeds the device limit");
        }

        std::vector<VkPipelineShaderStageCreateInfo> stages;
        auto addStage = [&](gpu::ShaderModule* module, ShaderStage expected, VkShaderStageFlagBits flag)
        {
            if (!module)
            {
                return uint32_t(VK_SHADER_UNUSED_KHR);
            }
            auto* native = dynamic_cast<vk::ShaderModule*>(module);
            if (!native || native->device() != gpuDevice || !native->isValid() || native->stage() != expected)
            {
                throw std::invalid_argument("Tracing shader stage/device is incompatible");
            }
            VkPipelineShaderStageCreateInfo stage{};
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = flag;
            stage.module = native->handle();
            stage.pName = native->entryPoint().c_str();
            stages.push_back(stage);
            return static_cast<uint32_t>(stages.size() - 1);
        };
        const auto raygen = addStage(description.raygenShader, ShaderStage::RayGeneration, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        const auto miss = addStage(description.missShader, ShaderStage::Miss, VK_SHADER_STAGE_MISS_BIT_KHR);
        const auto closest = addStage(description.closestHitShader, ShaderStage::ClosestHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        const auto any = addStage(description.anyHitShader, ShaderStage::AnyHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
        auto group = [](VkRayTracingShaderGroupTypeKHR type)
        {
            VkRayTracingShaderGroupCreateInfoKHR result{};
            result.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            result.type = type;
            result.generalShader = VK_SHADER_UNUSED_KHR;
            result.closestHitShader = VK_SHADER_UNUSED_KHR;
            result.anyHitShader = VK_SHADER_UNUSED_KHR;
            result.intersectionShader = VK_SHADER_UNUSED_KHR;
            return result;
        };
        auto raygenGroup = group(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
        raygenGroup.generalShader = raygen;
        groups.push_back(raygenGroup);
        uint32_t missGroupIndex = VK_SHADER_UNUSED_KHR;
        if (miss != VK_SHADER_UNUSED_KHR)
        {
            auto missGroup = group(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
            missGroup.generalShader = miss;
            missGroupIndex = static_cast<uint32_t>(groups.size());
            groups.push_back(missGroup);
        }
        uint32_t hitGroupIndex = VK_SHADER_UNUSED_KHR;
        if (closest != VK_SHADER_UNUSED_KHR || any != VK_SHADER_UNUSED_KHR)
        {
            auto hitGroup = group(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);
            hitGroup.closestHitShader = closest;
            hitGroup.anyHitShader = any;
            hitGroupIndex = static_cast<uint32_t>(groups.size());
            groups.push_back(hitGroup);
        }
        VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
        pipelineInfo.pStages = stages.data();
        pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
        pipelineInfo.pGroups = groups.data();
        pipelineInfo.maxPipelineRayRecursionDepth = description.maxRecursionDepth;
        pipelineInfo.layout = _layoutHandle;
        if (createRayTracingPipelines(_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan tracing pipeline");
        }

        const uint64_t handleSize = properties.shaderGroupHandleSize;
        const uint64_t stride = alignUp(handleSize, properties.shaderGroupHandleAlignment);
        const uint64_t slotSize = alignUp(stride, properties.shaderGroupBaseAlignment);
        if (!handleSize || stride > properties.maxShaderGroupStride)
        {
            throw std::runtime_error("Shader binding table stride exceeds the device limit");
        }
        std::vector<uint8_t> handles(handleSize * groups.size());
        if (getRayTracingShaderGroupHandles(_device, _pipeline, 0, static_cast<uint32_t>(groups.size()),
                handles.size(), handles.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to get tracing shader group handles");
        }
        // Extra prefix permits an aligned device address independently of VMA's base alignment.
        const uint64_t bytes = slotSize * groups.size() + properties.shaderGroupBaseAlignment;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bytes;
        bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateInfo allocation{};
        allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (vmaCreateBuffer(_allocator, &bufferInfo, &allocation, &_sbtBuffer, &_sbtAllocation, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate shader binding table");
        }
        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = _sbtBuffer;
        const auto address = vkGetBufferDeviceAddress(_device, &addressInfo);
        const auto aligned = alignUp(address, properties.shaderGroupBaseAlignment);
        const auto prefix = aligned - address;
        if (!address)
        {
            throw std::runtime_error("Shader binding table has no device address");
        }
        void* mapped = nullptr;
        if (vmaMapMemory(_allocator, _sbtAllocation, &mapped) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map shader binding table");
        }
        std::memset(mapped, 0, bytes);
        for (size_t i = 0; i < groups.size(); ++i)
        {
            std::memcpy(static_cast<uint8_t*>(mapped) + prefix + i * slotSize, handles.data() + i * handleSize, handleSize);
        }
        const auto flushResult = vmaFlushAllocation(_allocator, _sbtAllocation, 0, bytes);
        vmaUnmapMemory(_allocator, _sbtAllocation);
        if (flushResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to flush shader binding table");
        }
        _raygenRegion = {aligned, stride, stride};
        if (missGroupIndex != VK_SHADER_UNUSED_KHR)
        {
            _missRegion = {aligned + missGroupIndex * slotSize, stride, stride};
        }
        if (hitGroupIndex != VK_SHADER_UNUSED_KHR)
        {
            _hitRegion = {aligned + hitGroupIndex * slotSize, stride, stride};
        }
    }
    catch (...)
    {
        cleanup();
        throw;
    }
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::isValid() const
{
    return _pipeline != VK_NULL_HANDLE;
}

void RayTracingPipeline::cleanup()
{
    if (_sbtBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(_allocator, _sbtBuffer, _sbtAllocation);
        _sbtBuffer = VK_NULL_HANDLE;
        _sbtAllocation = VK_NULL_HANDLE;
    }
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    _raygenRegion = {};
    _missRegion = {};
    _hitRegion = {};
    _callableRegion = {};
}

}
