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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e::render::vulkan::rt
{

class BG2E_API RTAccelerationStructureSize {
public:
    RTAccelerationStructureSize(Engine * eng);

    VkAccelerationStructureBuildSizesInfoKHR getTLASSizes(
        VkDeviceAddress instancesDeviceAddress,
        size_t instanceCount,
        VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR
    );

    VkAccelerationStructureBuildSizesInfoKHR getBLASSizes(
        VkDeviceAddress vertexDeviceAddress,
        VkDeviceAddress indexDeviceAddress,
        VkFormat positionFormat,
        VkDeviceSize vertexStride,
        uint32_t maxVertex,
        uint32_t triangleCount,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32,
        VkAccelerationStructureBuildTypeKHR buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR
    );

    [[nodiscard]] VkAccelerationStructureBuildGeometryInfoKHR buildInfo() const { return _buildInfo; }

protected:
    Engine * _engine;

    uint32_t _primitiveCount = 0;
    VkAccelerationStructureGeometryTrianglesDataKHR _trianglesData{};
    VkAccelerationStructureGeometryInstancesDataKHR _instanceData{};
    VkAccelerationStructureGeometryKHR _geometry{};
    VkAccelerationStructureBuildGeometryInfoKHR _buildInfo{};

    VkAccelerationStructureBuildSizesInfoKHR getSizes(
        VkAccelerationStructureBuildTypeKHR buildType
    ) const;
};

}
