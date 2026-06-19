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

#include <bg2e/gpu/vk/RayTracingScene.hpp>
#include <bg2e/gpu/vk/RayTracingMesh.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/extensions.hpp>

#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e {
namespace gpu {
namespace vk {

namespace {

VkDeviceAddress bufferDeviceAddress(vk::Device* device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = buffer;
    return vkGetBufferDeviceAddress(device->handle(), &info);
}

VkDeviceAddress alignUp(VkDeviceAddress value, VkDeviceAddress alignment)
{
    if (alignment == 0) return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

VkTransformMatrixKHR toVkTransform(const glm::mat4& m)
{
    // glm::mat4 is column-major (m[col][row]); VkTransformMatrixKHR is a
    // row-major 3x4 matrix.
    VkTransformMatrixKHR out{};
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            out.matrix[row][col] = m[col][row];
        }
    }
    return out;
}

}

RayTracingScene::RayTracingScene(vk::Device* device, const std::string& debugName)
    : gpu::RayTracingScene(device)
    , _device(device)
    , _debugName(debugName)
{
}

RayTracingScene::~RayTracingScene()
{
    cleanup();
}

void RayTracingScene::buildOrUpdate(gpu::CommandBuffer* cmd)
{
    // Route through the command buffer so both the explicit
    // cmd->buildRayTracingScene(scene) and scene->buildOrUpdate(cmd) entry
    // points converge on the same implementation.
    cmd->buildRayTracingScene(this);
}

void RayTracingScene::ensureCapacity(uint32_t instanceCount)
{
    if (instanceCount <= _capacity && _instanceBuffer != VK_NULL_HANDLE && _tlas != VK_NULL_HANDLE)
    {
        return;
    }

    destroyBuffers();

    const uint32_t newCapacity = instanceCount > 0 ? instanceCount : 1;

    // --- Instance buffer (host-visible, acceleration structure build input) ---
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size  = sizeof(VkAccelerationStructureInstanceKHR) * newCapacity;
        bufInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocOut{};
        VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                                  &_instanceBuffer, &_instanceAlloc, &allocOut));
        _instanceMapped  = allocOut.pMappedData;
        _instanceAddress = bufferDeviceAddress(_device, _instanceBuffer);
    }

    // --- Query TLAS build sizes for the new capacity ---
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = _instanceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    getAccelerationStructureBuildSizes(
        _device->handle(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &newCapacity,
        &sizeInfo
    );

    // --- TLAS storage buffer + object ---
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size  = sizeInfo.accelerationStructureSize;
        bufInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage         = VMA_MEMORY_USAGE_AUTO;
        allocCI.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                                  &_asBuffer, &_asAlloc, nullptr));
    }

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = _asBuffer;
    createInfo.size   = sizeInfo.accelerationStructureSize;
    createInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    VK_ASSERT(createAccelerationStructure(_device->handle(), &createInfo, nullptr, &_tlas));

    // --- Scratch buffer (padded for the device-address alignment) ---
    {
        const VkDeviceAddress scratchAlign = _device->accelerationStructureScratchAlignment();
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size  = sizeInfo.buildScratchSize + scratchAlign;
        bufInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage         = VMA_MEMORY_USAGE_AUTO;
        allocCI.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                                  &_scratchBuffer, &_scratchAlloc, nullptr));
        _scratchAddress = alignUp(bufferDeviceAddress(_device, _scratchBuffer), scratchAlign);
    }

    _capacity = newCapacity;
}

void RayTracingScene::build(VkCommandBuffer cmd)
{
    const uint32_t instanceCount = static_cast<uint32_t>(_instances.size());

    ensureCapacity(instanceCount);

    // --- Fill the instance buffer ---
    if (instanceCount > 0 && _instanceMapped != nullptr)
    {
        std::vector<VkAccelerationStructureInstanceKHR> instances(instanceCount);
        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            const auto& src = _instances[i];
            auto* vkMesh = dynamic_cast<vk::RayTracingMesh*>(src.mesh);
            if (!vkMesh || !vkMesh->isBuilt())
            {
                throw std::runtime_error("vk::RayTracingScene::build: instance references an unbuilt RayTracingMesh");
            }

            VkAccelerationStructureInstanceKHR& dst = instances[i];
            dst.transform                              = toVkTransform(src.transform);
            dst.instanceCustomIndex                    = src.instanceId & 0x00FFFFFFu;
            dst.mask                                   = src.mask;
            dst.instanceShaderBindingTableRecordOffset = 0;
            dst.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            dst.accelerationStructureReference         = vkMesh->deviceAddress();
        }
        std::memcpy(_instanceMapped, instances.data(),
                    instances.size() * sizeof(VkAccelerationStructureInstanceKHR));
        vmaFlushAllocation(_device->allocator(), _instanceAlloc, 0, VK_WHOLE_SIZE);
    }

    // --- Build the TLAS ---
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = _instanceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount             = 1;
    buildInfo.pGeometries               = &geometry;
    buildInfo.dstAccelerationStructure  = _tlas;
    buildInfo.scratchData.deviceAddress = _scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount  = instanceCount;
    range.primitiveOffset = 0;
    range.firstVertex     = 0;
    range.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;
    cmdBuildAccelerationStructures(cmd, 1, &buildInfo, &pRange);

    // Make the freshly built TLAS visible to acceleration structure reads in
    // the fragment shader (ray queries).
    VkMemoryBarrier2 barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    barrier.srcStageMask  = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                            VK_ACCESS_2_SHADER_READ_BIT;

    VkDependencyInfo depInfo{};
    depInfo.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.memoryBarrierCount = 1;
    depInfo.pMemoryBarriers    = &barrier;
    cmdPipelineBarrier2(cmd, &depInfo);
}

void RayTracingScene::destroyBuffers()
{
    if (_tlas != VK_NULL_HANDLE)
    {
        destroyAccelerationStructure(_device->handle(), _tlas, nullptr);
        _tlas = VK_NULL_HANDLE;
    }
    if (_asBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(_device->allocator(), _asBuffer, _asAlloc);
        _asBuffer = VK_NULL_HANDLE;
        _asAlloc  = VK_NULL_HANDLE;
    }
    if (_scratchBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(_device->allocator(), _scratchBuffer, _scratchAlloc);
        _scratchBuffer = VK_NULL_HANDLE;
        _scratchAlloc  = VK_NULL_HANDLE;
    }
    if (_instanceBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(_device->allocator(), _instanceBuffer, _instanceAlloc);
        _instanceBuffer = VK_NULL_HANDLE;
        _instanceAlloc  = VK_NULL_HANDLE;
        _instanceMapped = nullptr;
    }
    _capacity = 0;
}

void RayTracingScene::cleanup()
{
    destroyBuffers();
    _instances.clear();
}

}
}
}
