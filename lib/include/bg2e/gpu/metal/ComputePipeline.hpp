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

#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class BG2E_API ComputePipeline : public gpu::ComputePipeline {
public:
#if BG2E_IS_MAC
    ComputePipeline(MTL::Device* device, const gpu::ComputePipelineDescription& description);
#else
    ComputePipeline(void* /*device*/, const gpu::ComputePipelineDescription& description);
#endif
    ~ComputePipeline() override;

    bool isValid() const override;
    void cleanup() override;

#if BG2E_IS_MAC
    MTL::ComputePipelineState* computePipelineState() const { return _computePipelineState; }
    NS::UInteger maxTotalThreadsPerThreadgroup() const { return _maxTotalThreadsPerThreadgroup; }
    NS::UInteger threadExecutionWidth() const { return _threadExecutionWidth; }
#endif

private:
#if BG2E_IS_MAC
    MTL::ComputePipelineState* _computePipelineState = nullptr;
    NS::UInteger _maxTotalThreadsPerThreadgroup = 0;
    NS::UInteger _threadExecutionWidth = 0;
#endif
};

}
}
}
