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
#include <bg2e/base/Log.hpp>

#include <stdexcept>
#include <vector>

namespace bg2e {
namespace gpu {
namespace vk {

RayTracingPipeline::RayTracingPipeline(
    gpu::Device* gpuDevice, VkDevice device, VmaAllocator allocator,
    const gpu::RayTracingPipelineDescription& description)
    : gpu::RayTracingPipeline(gpuDevice), _device(device), _allocator(allocator)
{
    auto* vkRgenModule = dynamic_cast<vk::ShaderModule*>(description.raygenShader);
    auto* vkLayout     = dynamic_cast<vk::PipelineLayout*>(description.layout);

    if (!vkRgenModule || !vkLayout)
    {
        throw std::runtime_error("vk::RayTracingPipeline: description contains non-Vulkan objects");
    }

    _layoutHandle = vkLayout->handle();

    // --- Shader stages ---
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

    // Stage 0: Ray generation (always present)
    VkPipelineShaderStageCreateInfo rgenStage{};
    rgenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rgenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    rgenStage.module = vkRgenModule->handle();
    rgenStage.pName = vkRgenModule->entryPoint().c_str();
    stages.push_back(rgenStage);

    // Raygen group (general)
    VkRayTracingShaderGroupCreateInfoKHR rgenGroup{};
    rgenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rgenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rgenGroup.generalShader = 0;
    rgenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rgenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rgenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    groups.push_back(rgenGroup);

    // Stage 1: Miss shader (optional)
    uint32_t missStageIndex = VK_SHADER_UNUSED_KHR;
    if (description.missShader && description.missShader->isValid())
    {
        auto* vkMissModule = dynamic_cast<vk::ShaderModule*>(description.missShader);
        if (vkMissModule)
        {
            VkPipelineShaderStageCreateInfo missStage{};
            missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            missStage.module = vkMissModule->handle();
            missStage.pName = vkMissModule->entryPoint().c_str();
            missStageIndex = static_cast<uint32_t>(stages.size());
            stages.push_back(missStage);

            VkRayTracingShaderGroupCreateInfoKHR missGroup{};
            missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            missGroup.generalShader = missStageIndex;
            missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            groups.push_back(missGroup);
        }
    }

    // Stage 2: Closest hit shader (optional)
    uint32_t chitStageIndex = VK_SHADER_UNUSED_KHR;
    if (description.closestHitShader && description.closestHitShader->isValid())
    {
        auto* vkChitModule = dynamic_cast<vk::ShaderModule*>(description.closestHitShader);
        if (vkChitModule)
        {
            VkPipelineShaderStageCreateInfo chitStage{};
            chitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            chitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            chitStage.module = vkChitModule->handle();
            chitStage.pName = vkChitModule->entryPoint().c_str();
            chitStageIndex = static_cast<uint32_t>(stages.size());
            stages.push_back(chitStage);

            // Hit group with closest hit
            VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
            hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
            hitGroup.closestHitShader = chitStageIndex;
            hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            groups.push_back(hitGroup);
        }
    }

    // --- Pipeline creation ---
    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = description.maxRecursionDepth;
    pipelineInfo.layout = _layoutHandle;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult result = createRayTracingPipelines(
        _device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);

    if (result != VK_SUCCESS)
    {
        bg2e_log_debug << "vk::RayTracingPipeline: createRayTracingPipelines failed with result "
                       << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan ray tracing pipeline");
    }

    // --- SBT creation ---
    // Query properties for handle size and alignment
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
    rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &rtProps;

    // Get the physical device from the Device
    auto* vkDevice = dynamic_cast<vk::Device*>(gpuDevice);
    VkPhysicalDevice physDev = vkDevice->physicalDeviceHandle();

    if (physDev != VK_NULL_HANDLE)
    {
        vkGetPhysicalDeviceProperties2(physDev, &props2);
    }

    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProps.shaderGroupBaseAlignment;
    uint32_t alignedHandleSize = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);

    uint32_t groupCount = static_cast<uint32_t>(groups.size());
    uint32_t sbtSize = alignedHandleSize * groupCount;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    result = getRayTracingShaderGroupHandles(
        _device, _pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get ray tracing shader group handles");
    }

    // Create SBT buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sbtSize;
    bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
                     | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                     | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VK_ASSERT(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &_sbtBuffer, &_sbtAllocation, nullptr));

    // Copy handles to SBT buffer
    void* mapped = nullptr;
    VK_ASSERT(vmaMapMemory(_allocator, _sbtAllocation, &mapped));
    memcpy(mapped, shaderHandleStorage.data(), sbtSize);
    vmaUnmapMemory(_allocator, _sbtAllocation);

    // Get buffer device address
    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = _sbtBuffer;
    VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(_device, &addressInfo);

    // Set up SBT regions
    // Region layout: [raygen] [miss] [hit] [callable]
    uint32_t raygenOffset = 0;
    uint32_t missOffset = alignedHandleSize;    // after raygen
    uint32_t hitOffset = missOffset + alignedHandleSize; // after miss
    // callable is empty

    _raygenRegion.deviceAddress = sbtAddress + raygenOffset;
    _raygenRegion.stride = alignedHandleSize;
    _raygenRegion.size = alignedHandleSize;

    _missRegion.deviceAddress = sbtAddress + missOffset;
    _missRegion.stride = alignedHandleSize;
    _missRegion.size = (missStageIndex != VK_SHADER_UNUSED_KHR) ? alignedHandleSize : 0;

    _hitRegion.deviceAddress = sbtAddress + hitOffset;
    _hitRegion.stride = alignedHandleSize;
    _hitRegion.size = (chitStageIndex != VK_SHADER_UNUSED_KHR) ? alignedHandleSize : 0;

    _callableRegion.deviceAddress = 0;
    _callableRegion.stride = 0;
    _callableRegion.size = 0;

    // Debug naming
    if (base::Log::isDebug() && !description.debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_pipeline);
        nameInfo.pObjectName = description.debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
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
}

}
}
}
