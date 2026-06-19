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

#include <bg2e/gpu/vk/RayTracingMesh.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Buffer.hpp>
#include <bg2e/gpu/vk/extensions.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

namespace {

VkFormat toVkVertexFormat(gpu::Format format)
{
    switch (format)
    {
        case gpu::Format::R32_SFLOAT:          return VK_FORMAT_R32_SFLOAT;
        case gpu::Format::R32G32_SFLOAT:       return VK_FORMAT_R32G32_SFLOAT;
        case gpu::Format::R32G32B32_SFLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;
        case gpu::Format::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:                               return VK_FORMAT_R32G32B32_SFLOAT;
    }
}

void createDeviceBuffer(vk::Device* device, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkBuffer& outBuffer, VmaAllocation& outAlloc, const char* debugTag)
{
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = size;
    bufInfo.usage = usage;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage         = VMA_MEMORY_USAGE_AUTO;
    allocCI.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkResult result = vmaCreateBuffer(device->allocator(), &bufInfo, &allocCI,
                                      &outBuffer, &outAlloc, nullptr);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("vk::RayTracingMesh: failed to allocate ") + debugTag);
    }
}

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

}

RayTracingMesh::RayTracingMesh(vk::Device* device, const RayTracingMeshDescription& description)
    : gpu::RayTracingMesh(device)
    , _device(device)
{
    _description = description;

    auto* vkVertex = dynamic_cast<vk::Buffer*>(description.vertexBuffer);
    auto* vkIndex  = dynamic_cast<vk::Buffer*>(description.indexBuffer);
    if (!vkVertex || !vkIndex)
    {
        throw std::runtime_error("vk::RayTracingMesh: vertex/index buffers must be vk::Buffer objects");
    }
    if (description.indexCount == 0 || (description.indexCount % 3) != 0)
    {
        throw std::runtime_error("vk::RayTracingMesh: indexCount must be a non-zero multiple of 3");
    }

    _vertexFormat   = toVkVertexFormat(description.vertexFormat);
    _vertexStride   = description.vertexStride;
    _maxVertex      = description.vertexCount > 0 ? description.vertexCount - 1 : 0;
    _firstIndex     = description.firstIndex;
    _primitiveCount = description.indexCount / 3;

    _vertexAddress = vkVertex->deviceAddress() + description.positionOffset;
    _indexAddress  = vkIndex->deviceAddress();

    // --- Query build sizes ---
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat  = _vertexFormat;
    geometry.geometry.triangles.vertexData.deviceAddress = _vertexAddress;
    geometry.geometry.triangles.vertexStride  = _vertexStride;
    geometry.geometry.triangles.maxVertex     = _maxVertex;
    geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = _indexAddress;
    geometry.geometry.triangles.transformData.deviceAddress = 0;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
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
        &_primitiveCount,
        &sizeInfo
    );

    // --- Acceleration structure storage buffer + object ---
    createDeviceBuffer(_device, sizeInfo.accelerationStructureSize,
                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       _asBuffer, _asAlloc, "BLAS storage buffer");

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = _asBuffer;
    createInfo.size   = sizeInfo.accelerationStructureSize;
    createInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    VK_ASSERT(createAccelerationStructure(_device->handle(), &createInfo, nullptr, &_accelerationStructure));

    // --- Scratch buffer (padded for the device-address alignment) ---
    const VkDeviceAddress scratchAlign = _device->accelerationStructureScratchAlignment();
    createDeviceBuffer(_device, sizeInfo.buildScratchSize + scratchAlign,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       _scratchBuffer, _scratchAlloc, "BLAS scratch buffer");
    _scratchAddress = alignUp(bufferDeviceAddress(_device, _scratchBuffer), scratchAlign);

    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
    addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addrInfo.accelerationStructure = _accelerationStructure;
    _deviceAddress = getAccelerationStructureDeviceAddress(_device->handle(), &addrInfo);
}

RayTracingMesh::~RayTracingMesh()
{
    cleanup();
}

void RayTracingMesh::build(VkCommandBuffer cmd)
{
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat  = _vertexFormat;
    geometry.geometry.triangles.vertexData.deviceAddress = _vertexAddress;
    geometry.geometry.triangles.vertexStride  = _vertexStride;
    geometry.geometry.triangles.maxVertex     = _maxVertex;
    geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = _indexAddress;
    geometry.geometry.triangles.transformData.deviceAddress = 0;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount             = 1;
    buildInfo.pGeometries               = &geometry;
    buildInfo.dstAccelerationStructure  = _accelerationStructure;
    buildInfo.scratchData.deviceAddress = _scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount  = _primitiveCount;
    range.primitiveOffset = _firstIndex * static_cast<uint32_t>(sizeof(uint32_t));
    range.firstVertex     = 0;
    range.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;
    cmdBuildAccelerationStructures(cmd, 1, &buildInfo, &pRange);

    _built = true;
}

void RayTracingMesh::cleanup()
{
    if (_accelerationStructure != VK_NULL_HANDLE)
    {
        destroyAccelerationStructure(_device->handle(), _accelerationStructure, nullptr);
        _accelerationStructure = VK_NULL_HANDLE;
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
    _built = false;
}

}
}
}
