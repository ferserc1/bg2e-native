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

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;
class SurfaceFrame;
class GraphicsPipeline;
class ComputePipeline;
class PipelineLayout;

class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer() = default;
#if BG2E_IS_MAC
    CommandBuffer(metal::Device* device, MTL::CommandBuffer* cmd);
#endif
    ~CommandBuffer() override;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    void begin() override;
    void end() override;
    void transition(gpu::Image* image, ImageLayout newLayout) override;
    void beginRendering(gpu::SurfaceFrame* frame) override;
    void endRendering() override;
    void beginCompute() override;
    void endCompute() override;
    void clearColor(uint32_t attachmentIndex, const gpu::Color& color) override;
    void clearDepth(float depth) override;
    void bindPipeline(gpu::GraphicsPipeline* pipeline) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
    void bindPipeline(gpu::ComputePipeline* pipeline) override;
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data) override;
    bool isValid() const override;

#if BG2E_IS_MAC
    MTL::CommandBuffer* handle() const { return _cmd; }
#endif

private:
#if BG2E_IS_MAC
    void ensureRenderEncoder();

    metal::Device*              _device      = nullptr;
    MTL::CommandBuffer*         _cmd         = nullptr;
    MTL::RenderPassDescriptor*  _passDesc    = nullptr;
    MTL::RenderCommandEncoder*  _encoder     = nullptr;
    MTL::ComputeCommandEncoder* _computeEncoder = nullptr;
    metal::SurfaceFrame*        _renderFrame = nullptr;
    bool                        _recording   = false;
    metal::GraphicsPipeline*    _boundPipeline = nullptr;
    metal::ComputePipeline*     _boundComputePipeline = nullptr;
    metal::PipelineLayout*      _boundLayout = nullptr;
#endif
};

}
}
}
