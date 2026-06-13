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

class BG2E_API PipelineLayout : public gpu::PipelineLayout {
public:
    PipelineLayout(gpu::Device* gpuDevice, const gpu::PipelineLayoutDescription& description);
    ~PipelineLayout() override;

    bool isValid() const override;
    void cleanup() override;

    const gpu::PipelineLayoutDescription& description() const { return _description; }

    const std::vector<ResourceBinding>& resourceBindings() const { return _description.resourceBindings; }

    // Resolves a ResourceBinding to a Metal argument index in the texture/sampler
    // namespaces. Textures and samplers are independent index namespaces in Metal,
    // so index = binding is sufficient for them.
    uint32_t metalIndex(const ResourceBinding& b) const { return b.binding; }

    // Metal buffer-namespace index allocation.
    //
    // The Metal [[buffer(n)]] namespace is shared (per stage) by vertex attribute
    // buffers, push-constant data and uniform/storage buffers. To avoid
    // collisions the indices are partitioned as:
    //   [0 .. MaxVertexBuffers-1]   -> vertex attribute buffers (stage_in)
    //   uniform / storage buffers   -> MaxVertexBuffers + set*MaxBindingsPerSet + binding
    // The example shaders must declare the matching [[buffer(n)]] indices.
    static constexpr uint32_t MaxVertexBuffers  = 8;
    static constexpr uint32_t MaxBindingsPerSet = 8;

    uint32_t metalBufferIndex(const ResourceBinding& b) const
    {
        return MaxVertexBuffers + b.set * MaxBindingsPerSet + b.binding;
    }

    // Buffer index reserved for push-constant data on Metal. Kept in the
    // vertex-buffer region so it does not collide with uniform/storage buffers.
    static constexpr uint32_t PushConstantBufferIndex = 0;

    uint32_t pushConstantBufferIndex(ShaderStage /*stage*/) const
    {
        return PushConstantBufferIndex;
    }

private:
    gpu::PipelineLayoutDescription _description;
};

}
}
}
