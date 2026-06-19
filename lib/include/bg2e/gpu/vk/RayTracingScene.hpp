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

#include <bg2e/gpu/RayTracingScene.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <string>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;

// Vulkan top-level acceleration structure (TLAS).
//
// Internal buffers (instance buffer, scratch buffer, TLAS storage buffer) are
// reused across rebuilds and only grown when the instance count exceeds the
// current capacity.
class RayTracingScene : public gpu::RayTracingScene {
public:
    RayTracingScene(vk::Device* device, const std::string& debugName = {});
    ~RayTracingScene() override;

    RayTracingScene(const RayTracingScene&) = delete;
    RayTracingScene& operator=(const RayTracingScene&) = delete;

    void buildOrUpdate(gpu::CommandBuffer* cmd) override;

    void cleanup() override;
    bool isValid() const override { return _tlas != VK_NULL_HANDLE; }

    // Records the TLAS build into `cmd` (called by CommandBuffer::buildRayTracingScene).
    void build(VkCommandBuffer cmd);

    VkAccelerationStructureKHR handle() const { return _tlas; }

private:
    void ensureCapacity(uint32_t instanceCount);
    void destroyBuffers();

    vk::Device* _device = nullptr;
    std::string _debugName;

    VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;

    VkBuffer      _asBuffer     = VK_NULL_HANDLE;
    VmaAllocation _asAlloc      = VK_NULL_HANDLE;
    VkBuffer      _scratchBuffer = VK_NULL_HANDLE;
    VmaAllocation _scratchAlloc  = VK_NULL_HANDLE;
    VkBuffer      _instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation _instanceAlloc  = VK_NULL_HANDLE;
    void*         _instanceMapped = nullptr;

    VkDeviceAddress _scratchAddress  = 0;
    VkDeviceAddress _instanceAddress = 0;

    uint32_t _capacity = 0;   // instance capacity of the current buffers
};

}
}
}
