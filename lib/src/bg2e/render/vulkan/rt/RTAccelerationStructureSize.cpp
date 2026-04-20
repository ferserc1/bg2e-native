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

#include <bg2e/render/vulkan/rt/RTAccelerationStructureSize.hpp>

namespace bg2e::render::vulkan::rt
{

RTAccelerationStructureSize::RTAccelerationStructureSize(Engine * eng)
    :_engine{eng}
{
}

VkAccelerationStructureBuildSizesInfoKHR RTAccelerationStructureSize::getTLASSizes(
    VkDeviceAddress instancesDeviceAddress,
    size_t instanceCount,
    VkAccelerationStructureBuildTypeKHR buildType
) {
    _instanceData = {};
    _instanceData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    _instanceData.arrayOfPointers = VK_FALSE;
    _instanceData.data.deviceAddress = instancesDeviceAddress;

    _geometry = {};
    _geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    _geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    _geometry.flags = 0;
    _geometry.geometry.instances = _instanceData;

    _primitiveCount = static_cast<uint32_t>(instanceCount);

    _buildInfo = {};
    _buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    _buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    _buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    _buildInfo.geometryCount = 1;
    _buildInfo.pGeometries = &_geometry;

    return getSizes(buildType);
}

VkAccelerationStructureBuildSizesInfoKHR RTAccelerationStructureSize::getBLASSizes(
    VkDeviceAddress vertexDeviceAddress,
    VkDeviceAddress indexDeviceAddress,
    VkFormat positionFormat,
    VkDeviceSize vertexStride,
    uint32_t maxVertex,
    uint32_t triangleCount,
    VkIndexType indexType,
    VkAccelerationStructureBuildTypeKHR buildType
){
    _trianglesData = {};
    _trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    _trianglesData.vertexData.deviceAddress = vertexDeviceAddress;
    _trianglesData.indexData.deviceAddress = indexDeviceAddress;
    _trianglesData.indexType = indexType;
    _trianglesData.vertexFormat = positionFormat;
    _trianglesData.vertexStride = vertexStride;
    _trianglesData.transformData.deviceAddress = 0;
    _trianglesData.maxVertex = maxVertex;

    _geometry = {};
    _geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    _geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    _geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    _geometry.geometry.triangles = _trianglesData;

    _buildInfo = {};
    _buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    _buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    _buildInfo.geometryCount = 1;
    _buildInfo.pGeometries = &_geometry;
    _buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    _primitiveCount = triangleCount;

    return getSizes(buildType);
}


VkAccelerationStructureBuildSizesInfoKHR RTAccelerationStructureSize::getSizes(
    VkAccelerationStructureBuildTypeKHR buildType
) const {
    if (_primitiveCount == 0)
    {
        throw std::runtime_error("RTAccelerationStructureSize::getSizes(): primitive count is 0");
    }

    VkAccelerationStructureBuildSizesInfoKHR buildSizes{};
    buildSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    getAccelerationStructureBuildSizes(
        _engine->device().handle(),
        buildType,
        &_buildInfo,
        &_primitiveCount,
        &buildSizes
    );

    return buildSizes;
}

}