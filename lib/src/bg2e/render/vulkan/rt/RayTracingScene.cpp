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

#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/vulkan/rt/RTAccelerationStructureSize.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

RayTracingScene::RayTracingScene(Engine * engine)
    :_engine(engine)
{
}

RayTracingScene::~RayTracingScene()
{
    cleanup();
}

bool RayTracingScene::update(VkCommandBuffer cmd, scene::Node* node)
{
    // This method can be called without ray tracing support
    if (!_engine->rayTracingSupported())
    {
        return false;
    }

    CollectRayTracingInstancesVisitor visitor(_engine);
    node->accept(&visitor);

    const auto & instances = visitor.rayTracingInstances();

    if (instances.empty())
    {
        cleanup();
        return true;
    }

    VkDeviceSize bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    _instanceBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
        _engine,
        bufferSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    auto dataPtr = _instanceBuffer->allocatedData();
    memcpy(dataPtr,instances.data(), bufferSize);
    _instanceBuffer->flushAllocatedData();
    
    auto instancesDeviceAddress = _instanceBuffer->deviceAddress();

    RTAccelerationStructureSize sizes(_engine);

    auto buildSizes = sizes.getTLASSizes(
        instancesDeviceAddress,
        instances.size()
    );

    _tlasBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
        _engine,
        buildSizes.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    ));

    _scratchBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
        _engine,
        buildSizes.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    ));

    VkAccelerationStructureCreateInfoKHR tlasInfo = {};
    tlasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    tlasInfo.buffer = _tlasBuffer->handle();
    tlasInfo.offset = 0;
    tlasInfo.size = buildSizes.accelerationStructureSize;
    tlasInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (createAccelerationStructure(
        _engine->device().handle(),
        &tlasInfo,
        nullptr,
        &_tlas
    ) != VK_SUCCESS)
    {
        cleanup();
        throw std::runtime_error("failed to create acceleration structure");
    }

    auto buildInfo = sizes.buildInfo();
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = _tlas;
    buildInfo.scratchData.deviceAddress = _scratchBuffer->deviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo {};
    buildRangeInfo.primitiveCount = static_cast<uint32_t>(instances.size());

    const VkAccelerationStructureBuildRangeInfoKHR * buildRangeInfos[] = { &buildRangeInfo };

    cmdBuildAccelerationStructures(
        cmd, 1, &buildInfo, buildRangeInfos
    );

    // Memory barrier: the fragment shader must wait until the acceleration structure is built
    // before read the TLAS
    VkMemoryBarrier2 memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    memoryBarrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    memoryBarrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;

    cmdPipelineBarrier2(cmd, &dependencyInfo);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = _tlas;
    _tlasDeviceAddress = getAccelerationStructureDeviceAddress(
        _engine->device().handle(),
        &addressInfo
    );

    return true;
}

void RayTracingScene::cleanup()
{
    if (_tlas != VK_NULL_HANDLE)
    {
        destroyAccelerationStructure(
            _engine->device().handle(),
            _tlas,
            nullptr
        );
    }
    _tlas = VK_NULL_HANDLE;

    if (_tlasBuffer) {
        _tlasBuffer.reset();
    }

    if (_instanceBuffer) {
        _instanceBuffer.reset();
    }

    if (_scratchBuffer) {
        _scratchBuffer.reset();
    }

    _tlasDeviceAddress = 0;
}

}
}
}
}
