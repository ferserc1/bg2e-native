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

#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class PipelineLayout;

class BG2E_API GraphicsPipeline : public gpu::GraphicsPipeline {
public:
#if BG2E_IS_MAC
    GraphicsPipeline(gpu::Device* gpuDevice, MTL::Device* device, const gpu::GraphicsPipelineDescription& description);
#else
    GraphicsPipeline(gpu::Device* gpuDevice, void* /*device*/, const gpu::GraphicsPipelineDescription& description);
#endif
    ~GraphicsPipeline() override;

    bool isValid() const override;
    void cleanup() override;

#if BG2E_IS_MAC
    MTL::RenderPipelineState* renderPipelineState() const { return _renderPipelineState; }
    // Non-null when the pipeline was created with a depth attachment format;
    // bound on the render encoder so depth testing/writing is actually applied.
    MTL::DepthStencilState* depthStencilState() const { return _depthStencilState; }
#endif
    gpu::PrimitiveTopology topology() const { return _topology; }
    metal::PipelineLayout* layout() const { return _layout; }

private:
    gpu::PrimitiveTopology _topology;
    metal::PipelineLayout* _layout = nullptr;

#if BG2E_IS_MAC
    MTL::RenderPipelineState* _renderPipelineState = nullptr;
    MTL::DepthStencilState*   _depthStencilState   = nullptr;
#endif
};

}
}
}
