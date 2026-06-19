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
#include <bg2e/gpu/metal/common.hpp>

#include <string>
#include <vector>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;

// Metal instance acceleration structure.
//
// Reuses the instance descriptor buffer and scratch buffer across rebuilds,
// growing them only when the instance count exceeds the current capacity.
class RayTracingScene : public gpu::RayTracingScene {
public:
    RayTracingScene(metal::Device* device, const std::string& debugName = {});
    ~RayTracingScene() override;

    RayTracingScene(const RayTracingScene&) = delete;
    RayTracingScene& operator=(const RayTracingScene&) = delete;

    void buildOrUpdate(gpu::CommandBuffer* cmd) override;

    void cleanup() override;
    bool isValid() const override;

#if BG2E_IS_MAC
    // Records the instance acceleration structure build into `cmd`.
    void build(MTL::CommandBuffer* cmd);

    MTL::AccelerationStructure* handle() const { return _instanceAS; }

    // Primitive acceleration structures referenced by the current build. They
    // must be made resident (useResource) before the instance acceleration
    // structure is used in a shader.
    const std::vector<MTL::AccelerationStructure*>& referencedPrimitives() const
    {
        return _referencedPrimitives;
    }
#endif

private:
    metal::Device* _device = nullptr;
    std::string    _debugName;

#if BG2E_IS_MAC
    MTL::AccelerationStructure* _instanceAS     = nullptr;
    MTL::Buffer*                _instanceBuffer = nullptr;
    MTL::Buffer*                _scratchBuffer  = nullptr;
    NS::UInteger                _instanceCapacity = 0;
    NS::UInteger                _asSize           = 0;
    NS::UInteger                _scratchSize      = 0;

    std::vector<MTL::AccelerationStructure*> _referencedPrimitives;
#endif
};

}
}
}
