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

#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;

// Vulkan bottom-level acceleration structure (BLAS) for one submesh.
//
// The acceleration structure storage buffer and scratch buffer are created in
// the constructor (sized through vkGetAccelerationStructureBuildSizesKHR). The
// actual build is recorded into a command buffer by build(); use
// CommandBuffer::buildRayTracingMesh or Device::immediateSubmit.
class RayTracingMesh : public gpu::RayTracingMesh {
public:
    RayTracingMesh(vk::Device* device, const RayTracingMeshDescription& description);
    ~RayTracingMesh() override;

    RayTracingMesh(const RayTracingMesh&) = delete;
    RayTracingMesh& operator=(const RayTracingMesh&) = delete;

    void cleanup() override;
    bool isValid() const override { return _accelerationStructure != VK_NULL_HANDLE; }
    bool isBuilt() const override { return _built; }

    // Records the BLAS build into `cmd`.
    void build(VkCommandBuffer cmd);

    VkAccelerationStructureKHR handle() const { return _accelerationStructure; }
    VkDeviceAddress deviceAddress() const { return _deviceAddress; }

private:
    vk::Device* _device = nullptr;

    VkAccelerationStructureKHR _accelerationStructure = VK_NULL_HANDLE;
    VkBuffer       _asBuffer      = VK_NULL_HANDLE;
    VmaAllocation  _asAlloc       = VK_NULL_HANDLE;
    VkBuffer       _scratchBuffer = VK_NULL_HANDLE;
    VmaAllocation  _scratchAlloc  = VK_NULL_HANDLE;

    VkDeviceAddress _deviceAddress = 0;
    bool            _built         = false;

    // Cached geometry inputs (device addresses are stable for the lifetime of
    // the source buffers).
    VkDeviceAddress _vertexAddress  = 0;
    VkDeviceAddress _indexAddress   = 0;
    VkDeviceAddress _scratchAddress = 0;
    VkFormat        _vertexFormat   = VK_FORMAT_R32G32B32_SFLOAT;
    uint32_t        _vertexStride   = 0;
    uint32_t        _maxVertex      = 0;
    uint32_t        _firstIndex     = 0;
    uint32_t        _primitiveCount = 0;
};

}
}
}
