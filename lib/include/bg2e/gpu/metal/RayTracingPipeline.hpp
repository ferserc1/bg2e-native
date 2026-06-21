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

#include <bg2e/gpu/RayTracingPipeline.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class PipelineLayout;

class BG2E_API RayTracingPipeline : public gpu::RayTracingPipeline {
public:
#if BG2E_IS_MAC
    RayTracingPipeline(gpu::Device* gpuDevice, MTL::Device* device,
                       const gpu::RayTracingPipelineDescription& description);
#else
    RayTracingPipeline(gpu::Device* gpuDevice, void* /*device*/,
                       const gpu::RayTracingPipelineDescription& description);
#endif
    ~RayTracingPipeline() override;

    bool isValid() const override;
    void cleanup() override;

#if BG2E_IS_MAC
    MTL::ComputePipelineState* computePipelineState() const { return _computePipelineState; }
    NS::UInteger maxTotalThreadsPerThreadgroup() const { return _maxTotalThreadsPerThreadgroup; }
#endif
    metal::PipelineLayout* layout() const { return _layout; }

private:
    metal::PipelineLayout* _layout = nullptr;
#if BG2E_IS_MAC
    MTL::ComputePipelineState* _computePipelineState = nullptr;
    NS::UInteger _maxTotalThreadsPerThreadgroup = 0;
#endif
};

}
}
}
