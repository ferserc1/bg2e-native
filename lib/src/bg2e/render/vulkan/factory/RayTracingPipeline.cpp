/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/render/vulkan/factory/RayTracingPipeline.hpp>
#include <bg2e/render/vulkan/factory/ShaderModule.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/base/Log.hpp>

#include <cstring>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

RayTracingPipeline::RayTracingPipeline(Engine* engine)
    : _engine(engine)
{
    _raygenGroupIndex = 0;
    _missGroupIndex = 0;
    _hitGroupIndex = 0;
}

RayTracingPipeline::~RayTracingPipeline()
{
    for (auto& module : _shaderModules)
    {
        if (module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(_engine->device().handle(), module, nullptr);
        }
    }
    _shaderModules.clear();
    _stages.clear();
    _groups.clear();
}

uint32_t RayTracingPipeline::addShaderStage(
    VkShaderStageFlagBits stage,
    const std::string& fileName,
    const std::string& entryPoint
)
{
    VkShaderModule shaderModule = factory::ShaderModule::loadFromSPV(
        fileName, _engine->device().handle()
    );
    _shaderModules.push_back(shaderModule);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.module = shaderModule;
    stageInfo.pName = entryPoint.c_str();

    _stages.push_back(stageInfo);

    return static_cast<uint32_t>(_stages.size() - 1);
}

void RayTracingPipeline::setRayGenShader(const std::string& fileName, const std::string& entryPoint)
{
    _raygenGroupIndex = addShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR, fileName, entryPoint);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = _raygenGroupIndex;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    _groups.push_back(group);
}

void RayTracingPipeline::setMissShader(const std::string& fileName, const std::string& entryPoint)
{
    _missGroupIndex = addShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR, fileName, entryPoint);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = _missGroupIndex;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    _groups.push_back(group);
}

void RayTracingPipeline::setClosestHitShader(const std::string& fileName, const std::string& entryPoint)
{
    _hitGroupIndex = addShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, fileName, entryPoint);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = _hitGroupIndex;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    _groups.push_back(group);
}

VkDeviceSize RayTracingPipeline::alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

VkPipeline RayTracingPipeline::build(
    VkPipelineLayout layout,
    uint32_t maxRecursionDepth,
    const std::string& name
)
{
    if (_stages.empty() || _groups.empty())
    {
        throw std::runtime_error("RayTracingPipeline: No shader stages or groups configured");
    }

    VkRayTracingPipelineCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    createInfo.stageCount = static_cast<uint32_t>(_stages.size());
    createInfo.pStages = _stages.data();
    createInfo.groupCount = static_cast<uint32_t>(_groups.size());
    createInfo.pGroups = _groups.data();
    createInfo.maxPipelineRayRecursionDepth = maxRecursionDepth;
    createInfo.layout = layout;

    vulkan::createRayTracingPipelines(
        _engine->device().handle(),
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1,
        &createInfo,
        nullptr,
        &_pipeline
    );

    if (base::Log::isDebug() && !name.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_pipeline);
        nameInfo.pObjectName = name.c_str();

        setDebugUtilsObjectName(_engine->device().handle(), &nameInfo);
    }

    return _pipeline;
}

RayTracingPipeline::SBTData RayTracingPipeline::createSBT(const std::string& name)
{
    if (_pipeline == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "RayTracingPipeline::createSBT() called before build() or build() failed"
        );
    }

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
    rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 props{};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &rtProps;

    vkGetPhysicalDeviceProperties2(_engine->physicalDevice().handle(), &props);

    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;
    VkDeviceSize handleSizeAligned = alignUp(handleSize, handleAlignment);

    uint32_t groupCount = static_cast<uint32_t>(_groups.size());
    size_t handleDataSize = static_cast<size_t>(groupCount) * handleSize;

    std::vector<uint8_t> handleData(handleDataSize);
    vulkan::getRayTracingShaderGroupHandles(
        _engine->device().handle(),
        _pipeline,
        0,
        groupCount,
        handleDataSize,
        handleData.data()
    );

    VkDeviceSize raygenOffset = 0;
    VkDeviceSize raygenStride = alignUp(handleSizeAligned, baseAlignment);
    VkDeviceSize raygenSize = raygenStride;

    VkDeviceSize missOffset = raygenOffset + raygenSize;
    VkDeviceSize missStride = handleSizeAligned;
    VkDeviceSize missSize = alignUp(missStride, baseAlignment);

    VkDeviceSize hitOffset = missOffset + missSize;
    VkDeviceSize hitStride = handleSizeAligned;
    VkDeviceSize hitSize = alignUp(hitStride, baseAlignment);

    VkDeviceSize sbtSize = hitOffset + hitSize;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sbtSize;
    bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer sbtHandle = VK_NULL_HANDLE;
    VmaAllocation sbtAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo sbtInfo{};

    VK_ASSERT(vmaCreateBuffer(
        _engine->allocator(),
        &bufferInfo,
        &allocInfo,
        &sbtHandle,
        &sbtAllocation,
        &sbtInfo
    ));

    void* mappedData = getMappedData(sbtAllocation);
    std::memset(mappedData, 0, sbtSize);

    std::memcpy(
        static_cast<uint8_t*>(mappedData) + raygenOffset,
        handleData.data() + _raygenGroupIndex * handleSize,
        handleSize
    );

    std::memcpy(
        static_cast<uint8_t*>(mappedData) + missOffset,
        handleData.data() + _missGroupIndex * handleSize,
        handleSize
    );

    std::memcpy(
        static_cast<uint8_t*>(mappedData) + hitOffset,
        handleData.data() + _hitGroupIndex * handleSize,
        handleSize
    );

    VkBufferDeviceAddressInfo addrInfo{};
    addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer = sbtHandle;
    VkDeviceAddress bufferAddress = vkGetBufferDeviceAddress(_engine->device().handle(), &addrInfo);

    SBTData sbtData;

    sbtData.raygenRegion.deviceAddress = bufferAddress + raygenOffset;
    sbtData.raygenRegion.stride = raygenStride;
    sbtData.raygenRegion.size = raygenSize;

    sbtData.missRegion.deviceAddress = bufferAddress + missOffset;
    sbtData.missRegion.stride = missStride;
    sbtData.missRegion.size = missStride; // Only one entry

    sbtData.hitRegion.deviceAddress = bufferAddress + hitOffset;
    sbtData.hitRegion.stride = hitStride;
    sbtData.hitRegion.size = hitStride;  // Only one entry

    sbtData.callableRegion.deviceAddress = 0;
    sbtData.callableRegion.stride = 0;
    sbtData.callableRegion.size = 0;

    auto finalBuffer = new vulkan::Buffer(
        _engine,
        sbtHandle,
        sbtAllocation,
        sbtInfo,
        name.empty() ? "RT_SBT" : name + "_SBT"
    );

    finalBuffer->flushAllocatedData();

    sbtData.buffer.reset(finalBuffer);

    return sbtData;
}

}
}
}
}