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
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;

// Metal primitive acceleration structure for one submesh.
//
// The acceleration structure and scratch buffer are created lazily on the first
// build() and reused afterwards. build() must be recorded into a command buffer
// (use CommandBuffer::buildRayTracingMesh or Device::immediateSubmit).
class RayTracingMesh : public gpu::RayTracingMesh {
public:
    RayTracingMesh(metal::Device* device, const RayTracingMeshDescription& description);
    ~RayTracingMesh() override;

    RayTracingMesh(const RayTracingMesh&) = delete;
    RayTracingMesh& operator=(const RayTracingMesh&) = delete;

    void cleanup() override;
    bool isValid() const override;
    bool isBuilt() const override { return _built; }

#if BG2E_IS_MAC
    // Records the primitive acceleration structure build into `cmd`.
    void build(MTL::CommandBuffer* cmd);

    MTL::AccelerationStructure* handle() const { return _accelerationStructure; }
#endif

private:
    metal::Device* _device = nullptr;
    bool           _built  = false;

#if BG2E_IS_MAC
    MTL::PrimitiveAccelerationStructureDescriptor* buildDescriptor() const;

    MTL::AccelerationStructure* _accelerationStructure = nullptr;
    MTL::Buffer*                _scratchBuffer         = nullptr;
    NS::UInteger                _scratchSize           = 0;
#endif
};

}
}
}
