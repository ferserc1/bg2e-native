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
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;
class SurfaceFrame;

class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer() = default;
    CommandBuffer(vk::Device* device, VkCommandBuffer cmd, VkCommandPool pool);

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
    bool isValid() const override { return _cmd != VK_NULL_HANDLE; }

    VkCommandBuffer handle() const { return _cmd; }

    void setPresentFrame(vk::SurfaceFrame* frame) { _presentFrame = frame; }
    vk::SurfaceFrame* presentFrame() const        { return _presentFrame; }

private:
    void flushPendingRendering();

    vk::Device*       _device = nullptr;
    VkCommandBuffer   _cmd    = VK_NULL_HANDLE;
    VkCommandPool     _pool   = VK_NULL_HANDLE;

    vk::SurfaceFrame* _renderFrame  = nullptr;
    bool              _renderingActive = false;
    bool              _computeActive = false;
    bool              _renderingEmitted = false;
    VkClearColorValue _clearColor = {{ 0, 0, 0, 1 }};
    float             _clearDepth = 1.0f;
    bool              _hasColorClear = false;
    bool              _hasDepthClear = false;
    VkPipelineLayout  _boundLayoutHandle = VK_NULL_HANDLE;

    vk::SurfaceFrame* _presentFrame = nullptr;
};

}
}
}