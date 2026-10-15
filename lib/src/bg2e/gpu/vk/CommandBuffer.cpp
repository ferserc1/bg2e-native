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
#include <bg2e/gpu/vk/RayTracingMesh.hpp>
#include <bg2e/gpu/vk/RayTracingScene.hpp>
#include <bg2e/gpu/vk/RayTracingPipeline.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

namespace {

VkImageLayout toVkLayout(ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout::Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::General:
        return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout::ColorAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::DepthAttachment:
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case ImageLayout::ShaderReadOnly:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::TransferSrc:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::TransferDst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
    if (_recording)
    {
        throw std::logic_error("Vulkan command buffer is already recording");
    }
    if (_completion && !_completion->completed())
    {
        throw std::logic_error("Vulkan command buffer is still in flight");
    }
    if (!_allocation)
    {
        throw std::logic_error("Vulkan command buffer has no managed allocation");
    }
    std::lock_guard poolLock(_allocation->pool->mutex);
    if (!_allocation->pool->pool)
    {
        throw std::logic_error("Vulkan command pool is closed");
    }
    VK_ASSERT(vkResetCommandBuffer(_cmd, 0));
    _completion.reset();
    _submissionFrame.reset();
    _presentFrame = nullptr;
    _executable = false;
    auto beginInfo = Info::commandBufferBeginInfo();
    VK_ASSERT(vkBeginCommandBuffer(_cmd, &beginInfo));
    _recording = true;
}

void CommandBuffer::end()
{
    if (!_recording || _renderingActive || _computeActive)
    {
        throw std::logic_error("Vulkan command buffer must close active scopes before end");
    }
    std::lock_guard poolLock(_allocation->pool->mutex);
    flushPendingRendering();
    _boundRTPipeline = nullptr;
    VK_ASSERT(vkEndCommandBuffer(_cmd));
    _recording = false;
    _executable = true;
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout)
{
    if (!image)
    {
        throw std::invalid_argument("Image transition requires an image");
    }
    transition(image, newLayout, image->fullRange());
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout, const ImageSubresourceRange& range)
{
    auto* native = dynamic_cast<vk::Image*>(image);
    if (!_recording || _renderingActive || _computeActive || !native || !native->isValid() || image->device() != _device)
    {
        throw std::logic_error("Image transition requires a valid recording device outside active scopes");
    }
    image->validateRange(range);
    image->validateLayout(newLayout);
    std::vector<VkImageMemoryBarrier2> barriers;
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount; ++layer)
    {
        for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.mipCount; ++mip)
        {
            const auto oldLayout = image->currentLayout({mip, 1, layer, 1});
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.srcAccessMask = oldLayout == ImageLayout::Undefined ? 0 : VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            barrier.oldLayout = toVkLayout(oldLayout);
            barrier.newLayout = toVkLayout(newLayout);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = native->handle();
            barrier.subresourceRange = {native->aspect(), mip, 1, layer, 1};
            barriers.push_back(barrier); // Same-layout barriers still order writes.
        }
    }
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependency.pImageMemoryBarriers = barriers.data();
    cmdPipelineBarrier2(_cmd, &dependency);
    image->setLayout(range, newLayout);
}

void CommandBuffer::materializeRenderPass()
{
    if (!_recording || !_renderingActive || _computeActive)
    {
        throw std::logic_error("Vulkan materializeRenderPass requires an active rendering scope.");
    }
    flushPendingRendering();
}

void CommandBuffer::beginRendering(const RenderingDescription& description)
{
    if (!_recording || _renderingActive || _computeActive || !_device)
    {
        throw std::logic_error("Attachment rendering requires recording outside another scope");
    }
    const auto extent = validateRendering(description, *_device);
    for (const auto& attachment : description.colors)
    {
        if (!dynamic_cast<vk::Image*>(attachment.image))
        {
            throw std::invalid_argument("Color attachment is not a Vulkan image");
        }
    }
    if (description.depth && !dynamic_cast<vk::Image*>(description.depth->image))
    {
        throw std::invalid_argument("Depth attachment is not a Vulkan image");
    }
    _attachmentRendering = description;
    _attachmentExtent = extent;
    _renderFrame = nullptr;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
    _renderingActive = true;
    _renderingEmitted = false;
    _hasColorClear = false;
    _hasDepthClear = false;
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame* frame)
{
    if (!_recording || _renderingActive || _computeActive)
    {
        throw std::logic_error("Rendering requires recording outside another active scope");
    }
    _attachmentRendering.reset();
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

void CommandBuffer::beginRendering(gpu::Image* colorImage, uint32_t mipLevel)
{
    RenderingDescription description;
    description.colors.push_back({colorImage});
    description.colors.back().subresource = {mipLevel, 1, 0, 1};
    beginRendering(description);
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t mipLevel)
{
    RenderingDescription description;
    description.colors.push_back({colorImage});
    description.colors.back().subresource = {mipLevel, 1, 0, 1};
    if (depthImage)
    {
        description.depth = DepthAttachmentDescription{depthImage, AttachmentLoadOperation::Clear};
        description.depth->subresource = {mipLevel, 1, 0, 1};
    }
    beginRendering(description);
}

void CommandBuffer::beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel)
{
    if (!cubemap || !cubemap->isValid() || static_cast<uint32_t>(face) >= 6)
    {
        throw std::invalid_argument("Cubemap rendering requires a valid cube and face");
    }
    RenderingDescription description;
    description.colors.push_back({cubemap->image()});
    description.colors.back().subresource = {mipLevel, 1, static_cast<uint32_t>(face), 1};
    beginRendering(description);
}

void CommandBuffer::clearColor(uint32_t attachmentIndex, const gpu::Color& color)
{
    if (!_renderingActive || _renderingEmitted)
    {
        throw std::logic_error("Clear color must precede render-pass materialization");
    }
    if (_attachmentRendering)
    {
        if (attachmentIndex >= _attachmentRendering->colors.size())
        {
            throw std::out_of_range("Color attachment index is out of range");
        }
        auto& attachment = _attachmentRendering->colors[attachmentIndex];
        if (isUnsignedIntegerFormat(attachment.image->pixelFormat()) || isSignedIntegerFormat(attachment.image->pixelFormat()))
        {
            throw std::invalid_argument("Use typed integer clear values in RenderingDescription");
        }
        attachment.load = AttachmentLoadOperation::Clear;
        attachment.clear.floatingPoint = color;
        return;
    }
    if (attachmentIndex != 0)
    {
        throw std::out_of_range("Single-target rendering has only attachment zero");
    }
    _clearColor = {color.r, color.g, color.b, color.a};
    _hasColorClear = true;
}

void CommandBuffer::clearDepth(float depth)
{
    if (!_renderingActive || _renderingEmitted)
    {
        throw std::logic_error("Clear depth must precede render-pass materialization");
    }
    if (_attachmentRendering)
    {
        if (!_attachmentRendering->depth || depth < 0.0f || depth > 1.0f || !std::isfinite(depth))
        {
            throw std::invalid_argument("Depth attachment/clear value is incompatible");
        }
        _attachmentRendering->depth->load = AttachmentLoadOperation::Clear;
        _attachmentRendering->depth->clearDepth = depth;
        return;
    }
    _clearDepth = depth;
    _hasDepthClear = true;
}

void CommandBuffer::flushPendingRendering()
{
    if (!_renderingActive || _renderingEmitted)
    {
        return;
    }

    if (_attachmentRendering)
    {
        const auto loadOperation = [](AttachmentLoadOperation operation)
        {
            switch (operation)
            {
            case AttachmentLoadOperation::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case AttachmentLoadOperation::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case AttachmentLoadOperation::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        };
        const auto storeOperation = [](AttachmentStoreOperation operation)
        {
            return operation == AttachmentStoreOperation::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        };
        std::vector<VkRenderingAttachmentInfo> colors;
        for (const auto& attachment : _attachmentRendering->colors)
        {
            auto* image = static_cast<vk::Image*>(attachment.image);
            VkRenderingAttachmentInfo native{};
            native.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            native.imageView = image->subresourceView(attachment.subresource, true);
            native.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            native.loadOp = loadOperation(attachment.load);
            native.storeOp = storeOperation(attachment.store);
            if (isUnsignedIntegerFormat(image->pixelFormat()))
            {
                std::copy(attachment.clear.unsignedInteger.begin(), attachment.clear.unsignedInteger.end(), native.clearValue.color.uint32);
            }
            else if (isSignedIntegerFormat(image->pixelFormat()))
            {
                std::copy(attachment.clear.signedInteger.begin(), attachment.clear.signedInteger.end(), native.clearValue.color.int32);
            }
            else
            {
                const auto& value = attachment.clear.floatingPoint;
                native.clearValue.color = {{value.r, value.g, value.b, value.a}};
            }
            colors.push_back(native);
        }
        VkRenderingAttachmentInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        if (_attachmentRendering->depth)
        {
            const auto& attachment = *_attachmentRendering->depth;
            depth.imageView = static_cast<vk::Image*>(attachment.image)->subresourceView(attachment.subresource, true);
            depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth.loadOp = loadOperation(attachment.load);
            depth.storeOp = storeOperation(attachment.store);
            depth.clearValue.depthStencil = {attachment.clearDepth, attachment.clearStencil};
        }
        VkRenderingInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        info.renderArea.extent = {_attachmentExtent.width, _attachmentExtent.height};
        info.layerCount = 1;
        info.colorAttachmentCount = static_cast<uint32_t>(colors.size());
        info.pColorAttachments = colors.data();
        info.pDepthAttachment = _attachmentRendering->depth ? &depth : nullptr;
        info.pStencilAttachment = _attachmentRendering->depth && hasStencil(_attachmentRendering->depth->image->pixelFormat()) ? &depth : nullptr;
        cmdBeginRendering(_cmd, &info);
        _renderingEmitted = true;
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
            _hasColorClear ? &clearColor : nullptr);

        uint32_t mipSize = std::max(1u, _renderCubeMap->size() >> _renderCubeMipLevel);

        VkRenderingInfo renderInfo = Info::renderingInfo(
            {mipSize, mipSize},
            &colorAttachment,
            nullptr);

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
        _hasColorClear ? &clearColor : nullptr);

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
        {colorImg->width(), colorImg->height()},
        &colorAttachment,
        depthAttachment);

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
    _attachmentRendering.reset();
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
    auto* vkPipe = dynamic_cast<vk::GraphicsPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline: not a vk::GraphicsPipeline");
    }

    if (_attachmentRendering)
    {
        if (!vkPipe->isValid() || vkPipe->device() != _device)
        {
            throw std::invalid_argument("Pipeline must belong to the rendering device");
        }
        validatePipelineAttachments(*_attachmentRendering, *vkPipe);
    }
    flushPendingRendering();

    if (_attachmentRendering || _renderFrame || _renderColorImage || _renderingCubeMap)
    {
        uint32_t w = 0, h = 0;
        if (_attachmentRendering)
        {
            w = _attachmentExtent.width;
            h = _attachmentExtent.height;
        }
        else if (_renderingCubeMap && _renderCubeMap)
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
        0, nullptr);
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
        0, nullptr);
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset)
{
    auto* vkBuf = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuf)
    {
        throw std::runtime_error("vk::CommandBuffer::bindVertexBuffer: not a vk::Buffer");
    }

    VkBuffer b = vkBuf->handle();
    VkDeviceSize off = static_cast<VkDeviceSize>(offset);
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

void CommandBuffer::copyImage(gpu::Image* source, gpu::Image* destination)
{
    copyImage(ImageView{source, {}}, ImageView{destination, {}});
}

void CommandBuffer::copyImage(const ImageView& source, const ImageView& destination)
{
    if (!_recording || _renderingActive || _computeActive || !_device)
    {
        throw std::logic_error("Image copies require recording outside active scopes");
    }
    validateCopy(source, destination, *_device);
    auto* nativeSource = dynamic_cast<vk::Image*>(source.image);
    auto* nativeDestination = dynamic_cast<vk::Image*>(destination.image);
    if (!nativeSource || !nativeDestination)
    {
        throw std::invalid_argument("Image copy requires Vulkan images");
    }
    const auto extent = source.image->mipExtent(source.range.baseMipLevel);
    VkImageCopy region{};
    const VkImageAspectFlags aspect = isDepthFormat(source.image->pixelFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource = {aspect, source.range.baseMipLevel, source.range.baseArrayLayer, 1};
    region.dstSubresource = {aspect, destination.range.baseMipLevel, destination.range.baseArrayLayer, 1};
    region.extent = {extent.width, extent.height, 1};
    vkCmdCopyImage(_cmd, nativeSource->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        nativeDestination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::generateMipmaps(gpu::Image* image, ImageLayout finalLayout)
{
    auto* native = dynamic_cast<vk::Image*>(image);
    if (!_recording || _renderingActive || _computeActive || !native || !_device)
    {
        throw std::logic_error("Mipmap generation requires recording outside active scopes");
    }
    validateMipmapGeneration(*image, *_device, finalLayout);
    for (uint32_t mip = 1; mip < image->mipLevels(); ++mip)
    {
        transition(image, ImageLayout::TransferSrc, {mip - 1, 1, 0, image->arrayLayers()});
        transition(image, ImageLayout::TransferDst, {mip, 1, 0, image->arrayLayers()});
        const auto sourceExtent = image->mipExtent(mip - 1);
        const auto targetExtent = image->mipExtent(mip);
        VkImageBlit region{};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, 0, image->arrayLayers()};
        region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, image->arrayLayers()};
        region.srcOffsets[1] = {static_cast<int32_t>(sourceExtent.width), static_cast<int32_t>(sourceExtent.height), 1};
        region.dstOffsets[1] = {static_cast<int32_t>(targetExtent.width), static_cast<int32_t>(targetExtent.height), 1};
        vkCmdBlitImage(_cmd, native->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            native->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
        transition(image, finalLayout, {mip - 1, 1, 0, image->arrayLayers()});
    }
    transition(image, finalLayout, {image->mipLevels() - 1, 1, 0, image->arrayLayers()});
}

void CommandBuffer::blitImage(gpu::Image* source, gpu::Image* destination)
{
    auto* nativeSource = dynamic_cast<vk::Image*>(source);
    auto* nativeDestination = dynamic_cast<vk::Image*>(destination);
    if (!_recording || _renderingActive || _computeActive || !nativeSource || !nativeDestination ||
        !source->isValid() || !destination->isValid() || source->device() != _device || destination->device() != _device ||
        source == destination || isDepthFormat(source->pixelFormat()) || source->pixelFormat() != destination->pixelFormat() ||
        !hasFlag(source->usage(), ImageUsage::TransferSrc) || !hasFlag(destination->usage(), ImageUsage::TransferDst) ||
        source->currentLayout({}) != ImageLayout::TransferSrc || destination->currentLayout({}) != ImageLayout::TransferDst)
    {
        throw std::invalid_argument("Blit requires matching color formats, transfer usages/layouts and no active scope");
    }
    const auto capabilities = _device->formatCapabilities(source->pixelFormat());
    if (!capabilities.blitSource || !capabilities.blitDestination || !capabilities.linearFiltering)
    {
        throw std::invalid_argument("Filtered Vulkan blit is unsupported for this format");
    }
    VkImageBlit region{};
    region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.srcOffsets[1] = {static_cast<int32_t>(source->width()), static_cast<int32_t>(source->height()), 1};
    region.dstOffsets[1] = {static_cast<int32_t>(destination->width()), static_cast<int32_t>(destination->height()), 1};
    vkCmdBlitImage(_cmd, nativeSource->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        nativeDestination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
}

void CommandBuffer::buildRayTracingMesh(gpu::RayTracingMesh* mesh)
{
    auto* vkMesh = dynamic_cast<vk::RayTracingMesh*>(mesh);
    if (!vkMesh)
    {
        throw std::runtime_error("vk::CommandBuffer::buildRayTracingMesh: not a vk::RayTracingMesh");
    }
    vkMesh->build(_cmd);
}

void CommandBuffer::buildRayTracingScene(gpu::RayTracingScene* scene)
{
    auto* vkScene = dynamic_cast<vk::RayTracingScene*>(scene);
    if (!vkScene)
    {
        throw std::runtime_error("vk::CommandBuffer::buildRayTracingScene: not a vk::RayTracingScene");
    }
    vkScene->build(_cmd);
}

void CommandBuffer::bindPipeline(gpu::RayTracingPipeline* pipeline)
{
    auto* vkPipe = dynamic_cast<vk::RayTracingPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline(RayTracingPipeline): not a vk::RayTracingPipeline");
    }

    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkPipe->handle());
    _boundLayoutHandle = vkPipe->layoutHandle();
    _boundRTPipeline = vkPipe;
}

void CommandBuffer::bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
{
    auto* vkPipe = dynamic_cast<vk::RayTracingPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(RayTracingPipeline): not a vk::RayTracingPipeline");
    }

    auto* vkSet = dynamic_cast<vk::ResourceSet*>(set);
    if (!vkSet)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(RayTracingPipeline): not a vk::ResourceSet");
    }

    VkDescriptorSet ds = vkSet->handle();
    vkCmdBindDescriptorSets(
        _cmd,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        vkPipe->layoutHandle(),
        setIndex,
        1, &ds,
        0, nullptr);
}

void CommandBuffer::traceRays(uint32_t width, uint32_t height, uint32_t depth)
{
    if (_boundRTPipeline == nullptr)
    {
        throw std::runtime_error("vk::CommandBuffer::traceRays: no ray tracing pipeline bound");
    }

    cmdTraceRays(
        _cmd,
        &_boundRTPipeline->raygenSBT(),
        &_boundRTPipeline->missSBT(),
        &_boundRTPipeline->hitSBT(),
        &_boundRTPipeline->callableSBT(),
        width, height, depth);
}

}
}
}