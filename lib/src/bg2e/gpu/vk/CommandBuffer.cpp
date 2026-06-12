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
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

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
    if (!_renderingActive || _renderingEmitted || !_renderFrame)
    {
        return;
    }

    auto* colorImg = dynamic_cast<vk::Image*>(_renderFrame->colorImage());
    if (!colorImg)
    {
        throw std::runtime_error("vk::CommandBuffer::flushPendingRendering: invalid color image");
    }

    VkClearValue clearColor{};
    clearColor.color = _clearColor;

    VkRenderingAttachmentInfo colorAttachment = Info::attachmentInfo(
        colorImg->imageView(),
        _hasColorClear ? &clearColor : nullptr
    );

    VkRenderingAttachmentInfo* depthAttachment = nullptr;
    auto* depthImg = dynamic_cast<vk::Image*>(_renderFrame->depthImage());
    if (depthImg && depthImg->isValid())
    {
        VkClearValue depthClear{};
        depthClear.depthStencil.depth = _clearDepth;

        VkRenderingAttachmentInfo depthAtt = Info::depthAttachmentInfo(
            depthImg->imageView(), _clearDepth
        );
        depthAttachment = &depthAtt;
    }

    VkRenderingInfo renderInfo = Info::renderingInfo(
        { static_cast<uint32_t>(_renderFrame->colorImage()->width()),
          static_cast<uint32_t>(_renderFrame->colorImage()->height()) },
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

    if (_renderFrame)
    {
        uint32_t w = static_cast<uint32_t>(_renderFrame->colorImage()->width());
        uint32_t h = static_cast<uint32_t>(_renderFrame->colorImage()->height());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(w);
        viewport.height = static_cast<float>(h);
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

}
}
}