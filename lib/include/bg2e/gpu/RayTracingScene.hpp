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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace bg2e {
namespace gpu {

class CommandBuffer;
class RayTracingMesh;

// A single instance of a RayTracingMesh inside a RayTracingScene.
struct RayTracingInstance {
    gpu::RayTracingMesh* mesh       = nullptr;
    glm::mat4            transform  = glm::mat4(1.0f); // world transform
    uint32_t            instanceId  = 0;               // user instance id
    uint32_t            mask        = 0xFF;            // visibility mask
};

// Top-level ray tracing acceleration structure containing instances of
// RayTracingMesh objects.
//
// The CPU-side instance list lives in this object. Internal GPU buffers
// (instance buffer, scratch buffer, acceleration structure storage) are owned
// by the backend implementation and reused across rebuilds following a
// capacity-based growth model: they are only recreated when the instance count
// exceeds the current capacity.
//
// Typical usage per frame:
//
//   scene->clearInstances();
//   scene->addInstance(mesh, worldMatrix, instanceId, mask);
//   scene->buildOrUpdate(cmd);   // before beginRendering
//
class BG2E_API RayTracingScene : public DeviceResource {
public:
    explicit RayTracingScene(Device* device) : DeviceResource(device) {}
    ~RayTracingScene() override = default;

    // CPU-side instance list management (common to all backends).
    void clearInstances() { _instances.clear(); }

    void addInstance(gpu::RayTracingMesh* mesh, const glm::mat4& transform,
                     uint32_t instanceId, uint32_t mask = 0xFF)
    {
        _instances.push_back({ mesh, transform, instanceId, mask });
    }

    const std::vector<RayTracingInstance>& instances() const { return _instances; }
    size_t instanceCount() const { return _instances.size(); }

    // Build or update the top-level acceleration structure from the current
    // instance list, recording the GPU commands into `cmd`. Must be called
    // outside an active rendering scope.
    virtual void buildOrUpdate(gpu::CommandBuffer* cmd) = 0;

protected:
    std::vector<RayTracingInstance> _instances;
};

}
}
