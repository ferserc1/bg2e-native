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

#include <bg2e/gpu/PipelineLayout.hpp>
#include <bg2e/gpu/Common.hpp>

#include <cstdint>

namespace bg2e {
namespace gpu {
namespace metal {

// Metal pipeline layout — metadata container for push constant ranges and
// resource bindings. Validates the PipelineLayoutDescription at construction:
//
//   1. At most one PushConstantRange per ShaderStage.
//   2. UniformBuffer/StorageBuffer bindings use buffer indices that do not
//      collide with reserved slots (vertex >= 2, fragment/compute >= 1).
//
// Throws std::runtime_error on validation failure.
class BG2E_API PipelineLayout : public gpu::PipelineLayout {
public:
    PipelineLayout(gpu::Device* gpuDevice, const gpu::PipelineLayoutDescription& description);
    ~PipelineLayout() override;

    bool isValid() const override;
    void cleanup() override;

    const gpu::PipelineLayoutDescription& description() const { return _description; }

    const std::vector<ResourceBinding>& resourceBindings() const { return _description.resourceBindings; }

    // Metal texture/sampler argument index: uses the metal field of ShaderBinding.
    uint32_t metalIndex(const ResourceBinding& b) const { return b.binding.metal; }

    // Metal buffer argument index: uses the metal field of ShaderBinding directly.
    uint32_t metalBufferIndex(const ResourceBinding& b) const
    {
        return b.binding.metal;
    }

    // Buffer index reserved for push-constant data on Metal.
    //
    // Metal push constant buffer indices are fixed by bg2e::gpu convention:
    //   Vertex   -> buffer(1)
    //   Fragment -> buffer(0)
    //   Compute  -> buffer(0)
    //
    // In Metal vertex shaders, buffer(0) is reserved for the geometric vertex
    // buffer because bg2e::gpu always binds vertex data as a single vertex buffer.
    uint32_t pushConstantBufferIndex(ShaderStage stage) const
    {
        switch (stage)
        {
            case ShaderStage::Vertex:   return 1;
            case ShaderStage::Fragment: return 0;
            case ShaderStage::Compute:          return 0;
            case ShaderStage::RayGeneration:    return 0;
            case ShaderStage::Miss:             return 0;
            case ShaderStage::ClosestHit:       return 0;
        }
        return 0;
    }

private:
    gpu::PipelineLayoutDescription _description;
};

}
}
}
