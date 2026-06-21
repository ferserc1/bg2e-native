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

class CubeMap;

namespace vk {

class Device;
class SurfaceFrame;
class RayTracingPipeline;

class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer() = default;
    CommandBuffer(vk::Device* device, VkCommandBuffer cmd, VkCommandPool pool);

    void begin() override;
    void end() override;
    void transition(gpu::Image* image, ImageLayout newLayout) override;
    void beginRendering(gpu::SurfaceFrame* frame) override;
    void beginRendering(gpu::Image* colorImage, uint32_t mipLevel = 0) override;
    void beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t mipLevel = 0) override;
    void beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel = 0) override;
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
    void bindResourceSet(gpu::GraphicsPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set) override;
    void bindResourceSet(gpu::ComputePipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set) override;
    void bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset) override;
    void bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                     int32_t vertexOffset, uint32_t firstInstance) override;
    void copyImage(gpu::Image* src, gpu::Image* dst) override;
    // TODO: function not fully tested
    void blitImage(gpu::Image* src, gpu::Image* dst) override;
    void buildRayTracingMesh(gpu::RayTracingMesh* mesh) override;
    void buildRayTracingScene(gpu::RayTracingScene* scene) override;
    void bindPipeline(gpu::RayTracingPipeline* pipeline) override;
    void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set) override;
    void traceRays(uint32_t width, uint32_t height, uint32_t depth) override;
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

    vk::RayTracingPipeline* _boundRTPipeline = nullptr;

    vk::SurfaceFrame* _presentFrame = nullptr;

    gpu::Image* _renderColorImage = nullptr;
    gpu::Image* _renderDepthImage = nullptr;

    gpu::CubeMap*  _renderCubeMap     = nullptr;
    uint32_t       _renderCubeFace    = 0;
    uint32_t       _renderCubeMipLevel = 0;
    bool           _renderingCubeMap  = false;
};

}
}
}