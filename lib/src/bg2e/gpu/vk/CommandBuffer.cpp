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

#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/Buffer.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/SurfaceFrame.hpp>
#include <bg2e/gpu/vk/GraphicsPipeline.hpp>
#include <bg2e/gpu/vk/ComputePipeline.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/ResourceSet.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

#include <algorithm>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

namespace {

VkImageLayout toVkLayout(ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout::Undefined:         return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::General:           return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout::ColorAttachment:   return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::DepthAttachment:   return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case ImageLayout::ShaderReadOnly:    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::TransferSrc:       return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::TransferDst:       return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::Present:           return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

}

CommandBuffer::CommandBuffer(vk::Device* device, VkCommandBuffer cmd, VkCommandPool pool)
    : _device(device), _cmd(cmd), _pool(pool)
{
}

void CommandBuffer::begin()
{
    auto beginInfo = Info::commandBufferBeginInfo();
    VK_ASSERT(vkBeginCommandBuffer(_cmd, &beginInfo));
}

void CommandBuffer::end()
{
    flushPendingRendering();
    VK_ASSERT(vkEndCommandBuffer(_cmd));
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout)
{
    auto* vkImg = dynamic_cast<vk::Image*>(image);
    if (!vkImg)
    {
        throw std::runtime_error("vk::CommandBuffer::transition: not a vk::Image");
    }

    VkImageLayout oldL = toVkLayout(image->currentLayout());
    VkImageLayout newL = toVkLayout(newLayout);
    if (oldL == newL)
    {
        return;
    }

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.oldLayout = oldL;
    barrier.newLayout = newL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkImg->handle();
    barrier.subresourceRange.aspectMask = vkImg->aspect();
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pImageMemoryBarriers = &barrier;
    depInfo.imageMemoryBarrierCount = 1;

    cmdPipelineBarrier2(_cmd, &depInfo);
    image->_currentLayout = newLayout;
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame* frame)
{
    _renderFrame = dynamic_cast<vk::SurfaceFrame*>(frame);
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingActive = true;
    _renderingEmitted = false;
    _hasColorClear = false;
    _hasDepthClear = false;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, uint32_t /*mipLevel*/)
{
    _renderFrame = nullptr;
    _renderColorImage = colorImage;
    _renderDepthImage = nullptr;
    _renderingActive = true;
    _renderingEmitted = false;
    _hasColorClear = false;
    _hasDepthClear = false;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t /*mipLevel*/)
{
    _renderFrame = nullptr;
    _renderColorImage = colorImage;
    _renderDepthImage = depthImage;
    _renderingActive = true;
    _renderingEmitted = false;
    _hasColorClear = false;
    _hasDepthClear = false;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
}

void CommandBuffer::beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel)
{
    if (!cubemap)
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): cubemap is null");
    }
    if (!cubemap->isValid())
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): cubemap is not valid");
    }
    if (!cubemap->image())
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): cubemap image is null");
    }
    if (!cubemap->image()->isCubemap())
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): image is not a cubemap");
    }
    if (static_cast<uint32_t>(face) > 5)
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): face must be in [0, 5]");
    }
    if (mipLevel >= cubemap->mipLevels())
    {
        throw std::runtime_error("vk::CommandBuffer::beginRendering(CubeMap): mipLevel out of range");
    }

    _renderFrame = nullptr;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderCubeMap = cubemap;
    _renderCubeFace = static_cast<uint32_t>(face);
    _renderCubeMipLevel = mipLevel;
    _renderingCubeMap = true;
    _renderingActive = true;
    _renderingEmitted = false;
    _hasColorClear = false;
    _hasDepthClear = false;
}

void CommandBuffer::clearColor(uint32_t attachmentIndex, const gpu::Color& color)
{
    (void)attachmentIndex;
    _clearColor = { color.r, color.g, color.b, color.a };
    _hasColorClear = true;
}

void CommandBuffer::clearDepth(float depth)
{
    _clearDepth = depth;
    _hasDepthClear = true;
}

void CommandBuffer::flushPendingRendering()
{
    if (!_renderingActive || _renderingEmitted)
    {
        return;
    }

    // Cubemap face rendering path
    if (_renderingCubeMap && _renderCubeMap)
    {
        auto* vkImg = dynamic_cast<vk::Image*>(_renderCubeMap->image());
        if (!vkImg)
        {
            throw std::runtime_error("vk::CommandBuffer::flushPendingRendering: cubemap image is not a vk::Image");
        }

        VkImageView faceView = vkImg->cubemapFaceMipView(_renderCubeFace, _renderCubeMipLevel);
        if (faceView == VK_NULL_HANDLE)
        {
            throw std::runtime_error("vk::CommandBuffer::flushPendingRendering: invalid cubemap face/mip view");
        }

        VkClearValue clearColor{};
        clearColor.color = _clearColor;

        VkRenderingAttachmentInfo colorAttachment = Info::attachmentInfo(
            faceView,
            _hasColorClear ? &clearColor : nullptr
        );

        uint32_t mipSize = std::max(1u, _renderCubeMap->size() >> _renderCubeMipLevel);

        VkRenderingInfo renderInfo = Info::renderingInfo(
            { mipSize, mipSize },
            &colorAttachment,
            nullptr
        );

        cmdBeginRendering(_cmd, &renderInfo);
        _renderingEmitted = true;
        return;
    }

    // Determine color image source
    vk::Image* colorImg = nullptr;
    if (_renderFrame)
    {
        colorImg = dynamic_cast<vk::Image*>(_renderFrame->colorImage());
    }
    else if (_renderColorImage)
    {
        colorImg = dynamic_cast<vk::Image*>(_renderColorImage);
    }

    if (!colorImg)
    {
        throw std::runtime_error("vk::CommandBuffer::flushPendingRendering: no color image");
    }

    VkClearValue clearColor{};
    clearColor.color = _clearColor;

    VkRenderingAttachmentInfo colorAttachment = Info::attachmentInfo(
        colorImg->imageView(),
        _hasColorClear ? &clearColor : nullptr
    );

    // Determine depth image source
    vk::Image* depthImg = nullptr;
    if (_renderFrame)
    {
        depthImg = dynamic_cast<vk::Image*>(_renderFrame->depthImage());
    }
    else if (_renderDepthImage)
    {
        depthImg = dynamic_cast<vk::Image*>(_renderDepthImage);
    }

    VkRenderingAttachmentInfo* depthAttachment = nullptr;
    VkRenderingAttachmentInfo depthAtt{};
    if (depthImg && depthImg->isValid())
    {
        depthAtt = Info::depthAttachmentInfo(depthImg->imageView(), _clearDepth);
        depthAttachment = &depthAtt;
    }

    VkRenderingInfo renderInfo = Info::renderingInfo(
        { colorImg->width(), colorImg->height() },
        &colorAttachment,
        depthAttachment
    );

    cmdBeginRendering(_cmd, &renderInfo);
    _renderingEmitted = true;
}

void CommandBuffer::endRendering()
{
    flushPendingRendering();
    if (_renderingEmitted)
    {
        cmdEndRendering(_cmd);
    }
    _renderingActive = false;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
}

void CommandBuffer::beginCompute()
{
    // Vulkan does not require a dedicated compute encoder, but the scope is
    // tracked for API consistency with the Metal backend.
    if (_renderingActive)
    {
        throw std::runtime_error("vk::CommandBuffer::beginCompute: cannot begin a compute scope inside a rendering scope");
    }
    if (_computeActive)
    {
        throw std::runtime_error("vk::CommandBuffer::beginCompute: a compute scope is already active");
    }
    _computeActive = true;
}

void CommandBuffer::endCompute()
{
    if (!_computeActive)
    {
        throw std::runtime_error("vk::CommandBuffer::endCompute: no active compute scope; call beginCompute() first");
    }
    _computeActive = false;
}

void CommandBuffer::bindPipeline(gpu::GraphicsPipeline* pipeline)
{
    flushPendingRendering();

    auto* vkPipe = dynamic_cast<vk::GraphicsPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline: not a vk::GraphicsPipeline");
    }

    if (_renderFrame || _renderColorImage || _renderingCubeMap)
    {
        uint32_t w = 0, h = 0;
        if (_renderingCubeMap && _renderCubeMap)
        {
            uint32_t mipSize = std::max(1u, _renderCubeMap->size() >> _renderCubeMipLevel);
            w = mipSize;
            h = mipSize;
        }
        else if (_renderFrame)
        {
            w = static_cast<uint32_t>(_renderFrame->colorImage()->width());
            h = static_cast<uint32_t>(_renderFrame->colorImage()->height());
        }
        else
        {
            w = _renderColorImage->width();
            h = _renderColorImage->height();
        }

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(h);
        viewport.width = static_cast<float>(w);
        viewport.height = -1.0f * static_cast<float>(h);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {w, h};
        vkCmdSetScissor(_cmd, 0, 1, &scissor);
    }

    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->handle());
    _boundLayoutHandle = vkPipe->layoutHandle();
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    flushPendingRendering();
    vkCmdDraw(_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::bindPipeline(gpu::ComputePipeline* pipeline)
{
    if (!_computeActive)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline(ComputePipeline): no active compute scope; call beginCompute() first");
    }

    auto* vkPipe = dynamic_cast<vk::ComputePipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline(ComputePipeline): not a vk::ComputePipeline");
    }

    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipe->handle());
    _boundLayoutHandle = vkPipe->layoutHandle();
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    if (!_computeActive)
    {
        throw std::runtime_error("vk::CommandBuffer::dispatch: no active compute scope; call beginCompute() first");
    }

    vkCmdDispatch(_cmd, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data)
{
    if (_boundLayoutHandle == VK_NULL_HANDLE)
    {
        throw std::runtime_error("vk::CommandBuffer::pushConstants: no pipeline bound");
    }

    vkCmdPushConstants(_cmd, _boundLayoutHandle, shaderStageToVkFlags(stage), offset, size, data);
}

void CommandBuffer::bindResourceSet(gpu::GraphicsPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
{
    flushPendingRendering();

    auto* vkPipe = dynamic_cast<vk::GraphicsPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(GraphicsPipeline): not a vk::GraphicsPipeline");
    }

    auto* vkSet = dynamic_cast<vk::ResourceSet*>(set);
    if (!vkSet)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(GraphicsPipeline): not a vk::ResourceSet");
    }

    VkDescriptorSet ds = vkSet->handle();
    vkCmdBindDescriptorSets(
        _cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        vkPipe->layoutHandle(),
        setIndex,
        1, &ds,
        0, nullptr
    );
}

void CommandBuffer::bindResourceSet(gpu::ComputePipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
{
    if (!_computeActive)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(ComputePipeline): no active compute scope; call beginCompute() first");
    }

    auto* vkPipe = dynamic_cast<vk::ComputePipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(ComputePipeline): not a vk::ComputePipeline");
    }

    auto* vkSet = dynamic_cast<vk::ResourceSet*>(set);
    if (!vkSet)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(ComputePipeline): not a vk::ResourceSet");
    }

    VkDescriptorSet ds = vkSet->handle();
    vkCmdBindDescriptorSets(
        _cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        vkPipe->layoutHandle(),
        setIndex,
        1, &ds,
        0, nullptr
    );
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset)
{
    auto* vkBuf = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuf)
    {
        throw std::runtime_error("vk::CommandBuffer::bindVertexBuffer: not a vk::Buffer");
    }

    VkBuffer      b   = vkBuf->handle();
    VkDeviceSize  off = static_cast<VkDeviceSize>(offset);
    vkCmdBindVertexBuffers(_cmd, binding, 1, &b, &off);
}

void CommandBuffer::bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset)
{
    auto* vkBuf = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuf)
    {
        throw std::runtime_error("vk::CommandBuffer::bindIndexBuffer: not a vk::Buffer");
    }

    vkCmdBindIndexBuffer(_cmd, vkBuf->handle(),
                         static_cast<VkDeviceSize>(offset),
                         VK_INDEX_TYPE_UINT32);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t firstIndex, int32_t vertexOffset,
                                uint32_t firstInstance)
{
    flushPendingRendering();
    vkCmdDrawIndexed(_cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::copyImage(gpu::Image* src, gpu::Image* dst)
{
    auto* vkSrc = dynamic_cast<vk::Image*>(src);
    auto* vkDst = dynamic_cast<vk::Image*>(dst);
    if (!vkSrc || !vkDst)
    {
        throw std::runtime_error("vk::CommandBuffer::copyImage: not a vk::Image");
    }

    VkImageCopy region{};
    region.srcSubresource.aspectMask = vkSrc->aspect();
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = vkDst->aspect();
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent = {src->width(), src->height(), 1};

    vkCmdCopyImage(
        _cmd,
        vkSrc->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vkDst->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region
    );
}

// TODO: function not fully tested
void CommandBuffer::blitImage(gpu::Image* src, gpu::Image* dst)
{
    auto* vkSrc = dynamic_cast<vk::Image*>(src);
    auto* vkDst = dynamic_cast<vk::Image*>(dst);
    if (!vkSrc || !vkDst)
    {
        throw std::runtime_error("vk::CommandBuffer::blitImage: not a vk::Image");
    }

    VkImageBlit region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffsets[0] = {0, 0, 0};
    region.srcOffsets[1] = {
        static_cast<int32_t>(src->width()),
        static_cast<int32_t>(src->height()), 1
    };
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffsets[0] = {0, 0, 0};
    region.dstOffsets[1] = {
        static_cast<int32_t>(dst->width()),
        static_cast<int32_t>(dst->height()), 1
    };

    vkCmdBlitImage(
        _cmd,
        vkSrc->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vkDst->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region,
        VK_FILTER_LINEAR
    );
}

}
}
}