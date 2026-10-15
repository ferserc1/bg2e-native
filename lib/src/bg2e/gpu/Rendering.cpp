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

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <cmath>

namespace bg2e::gpu {

void GraphicsPipeline::validateDescription(const GraphicsPipelineDescription& description, const Device& device)
{
    const auto colors = description.resolvedColorAttachments();
    if (!device.isValid())
    {
        throw std::invalid_argument("Rendering requires a valid logical device");
    }
    const auto limits = device.renderingCapabilities();
    if (colors.size() > limits.maxColorAttachments ||
        (colors.empty() && description.depthFormat == PixelFormat::Undefined))
    {
        throw std::invalid_argument("Pipeline attachment count is unsupported");
    }
    if (!description.vertexShader || !description.fragmentShader || !description.layout ||
        description.vertexShader->device() != &device || description.fragmentShader->device() != &device ||
        description.layout->device() != &device || !description.vertexShader->isValid() ||
        !description.fragmentShader->isValid() || !description.layout->isValid() ||
        description.vertexShader->stage() != ShaderStage::Vertex || description.fragmentShader->stage() != ShaderStage::Fragment)
    {
        throw std::invalid_argument("Pipeline shaders/layout must belong to the device");
    }
    uint32_t bytes = 0;
    for (const auto& color : colors)
    {
        const auto& blend = color.blend;
        if (static_cast<uint32_t>(blend.sourceColor) > static_cast<uint32_t>(BlendFactor::OneMinusDestinationAlpha) ||
            static_cast<uint32_t>(blend.destinationColor) > static_cast<uint32_t>(BlendFactor::OneMinusDestinationAlpha) ||
            static_cast<uint32_t>(blend.sourceAlpha) > static_cast<uint32_t>(BlendFactor::OneMinusDestinationAlpha) ||
            static_cast<uint32_t>(blend.destinationAlpha) > static_cast<uint32_t>(BlendFactor::OneMinusDestinationAlpha) ||
            static_cast<uint32_t>(blend.colorOperation) > static_cast<uint32_t>(BlendOperation::Max) ||
            static_cast<uint32_t>(blend.alphaOperation) > static_cast<uint32_t>(BlendOperation::Max))
        {
            throw std::invalid_argument("Invalid attachment blend operation/factor");
        }
        const auto capabilities = device.formatCapabilities(color.format);
        if (isDepthFormat(color.format) || !capabilities.supports(ImageUsage::ColorAttachment) ||
            (color.blend.enabled && !capabilities.blend) ||
            (static_cast<uint32_t>(color.blend.writeMask) & ~15u) != 0)
        {
            throw std::invalid_argument("Pipeline color format/blend/write mask is unsupported");
        }
        if (!limits.independentBlend && color.blend != colors.front().blend)
        {
            throw std::invalid_argument("Device does not support independent attachment blend state");
        }
        bytes += capabilities.colorAttachmentBytesPerSample;
    }
    if (limits.maxColorAttachmentBytesPerSample && bytes > limits.maxColorAttachmentBytesPerSample)
    {
        throw std::invalid_argument("Pipeline aggregate color attachment size exceeds device limit");
    }
    if (description.depthFormat != PixelFormat::Undefined &&
        (!isDepthFormat(description.depthFormat) ||
         !device.formatCapabilities(description.depthFormat).supports(ImageUsage::DepthStencil)))
    {
        throw std::invalid_argument("Pipeline depth format is unsupported");
    }
    if (description.depthState && static_cast<uint32_t>(description.depthState->compare) > static_cast<uint32_t>(CompareOperation::Always))
    {
        throw std::invalid_argument("Invalid depth comparison operation");
    }
    if (description.depthState && description.depthFormat == PixelFormat::Undefined &&
        (description.depthState->testEnabled || description.depthState->writeEnabled))
    {
        throw std::invalid_argument("Depth test/write state requires a depth format");
    }
}

Size2D CommandBuffer::validateRendering(const RenderingDescription& description, const Device& device)
{
    if (!device.isValid())
    {
        throw std::invalid_argument("Rendering requires a valid logical device");
    }
    const auto limits = device.renderingCapabilities();
    if (description.colors.size() > limits.maxColorAttachments ||
        (description.colors.empty() && !description.depth))
    {
        throw std::invalid_argument("Rendering attachment count is unsupported");
    }
    Size2D extent;
    std::vector<ImageView> seen;
    const auto validateOperations = [](AttachmentLoadOperation load, AttachmentStoreOperation store)
    {
        if (static_cast<uint32_t>(load) > static_cast<uint32_t>(AttachmentLoadOperation::DontCare) ||
            static_cast<uint32_t>(store) > static_cast<uint32_t>(AttachmentStoreOperation::DontCare))
        {
            throw std::invalid_argument("Invalid rendering load/store operation");
        }
    };
    auto validateImage = [&](Image* image, ImageUsage usage, const ImageSubresourceRange& range)
    {
        if (!image || !image->isValid() || image->device() != &device ||
            image->size().isZero() ||
            !hasFlag(image->usage(), usage) || !device.formatCapabilities(image->pixelFormat()).supports(usage))
        {
            throw std::invalid_argument("Rendering image/device/type/usage is incompatible");
        }
        image->validateRange(range);
        if (range.mipCount != 1 || range.layerCount != 1)
        {
            throw std::invalid_argument("Rendering views must select exactly one mip and layer");
        }
        const auto selectedExtent = image->mipExtent(range.baseMipLevel);
        if (selectedExtent.width > limits.maxImageDimension2D || selectedExtent.height > limits.maxImageDimension2D)
        {
            throw std::invalid_argument("Rendering extent exceeds device limit");
        }
        if (!extent.isZero() && selectedExtent != extent)
        {
            throw std::invalid_argument("Rendering attachment extents must match");
        }
        for (const auto& previous : seen)
        {
            if (previous.image == image && previous.range.baseMipLevel == range.baseMipLevel &&
                previous.range.baseArrayLayer == range.baseArrayLayer)
            {
                throw std::invalid_argument("Rendering attachments must not alias");
            }
        }
        seen.push_back({image, range});
        extent = selectedExtent;
    };
    uint32_t bytes = 0;
    for (const auto& attachment : description.colors)
    {
        validateOperations(attachment.load, attachment.store);
        validateImage(attachment.image, ImageUsage::ColorAttachment, attachment.subresource);
        if (isDepthFormat(attachment.image->pixelFormat()) ||
            attachment.image->currentLayout(attachment.subresource) != ImageLayout::ColorAttachment)
        {
            throw std::invalid_argument("Color attachment format/layout is incompatible");
        }
        bytes += device.formatCapabilities(attachment.image->pixelFormat()).colorAttachmentBytesPerSample;
    }
    if (limits.maxColorAttachmentBytesPerSample && bytes > limits.maxColorAttachmentBytesPerSample)
    {
        throw std::invalid_argument("Rendering aggregate color attachment size exceeds device limit");
    }
    if (description.depth)
    {
        validateOperations(description.depth->load, description.depth->store);
        validateImage(description.depth->image, ImageUsage::DepthStencil, description.depth->subresource);
        if (!isDepthFormat(description.depth->image->pixelFormat()) ||
            description.depth->image->currentLayout(description.depth->subresource) != ImageLayout::DepthAttachment ||
            !std::isfinite(description.depth->clearDepth) || description.depth->clearDepth < 0.0f ||
            description.depth->clearDepth > 1.0f)
        {
            throw std::invalid_argument("Depth attachment format/layout/clear is incompatible");
        }
    }
    return extent;
}

void CommandBuffer::validatePipelineAttachments(const RenderingDescription& description, const GraphicsPipeline& pipeline)
{
    if (description.colors.size() != pipeline.colorAttachments().size())
    {
        throw std::invalid_argument("Pipeline/rendering color attachment counts differ");
    }
    for (size_t i = 0; i < description.colors.size(); ++i)
    {
        if (!description.colors[i].image || description.colors[i].image->pixelFormat() != pipeline.colorAttachments()[i].format)
        {
            throw std::invalid_argument("Pipeline/rendering color attachment formats differ");
        }
    }
    if (description.depth && !description.depth->image)
    {
        throw std::invalid_argument("Pipeline/rendering depth image is missing");
    }
    const auto depthFormat = description.depth ? description.depth->image->pixelFormat() : PixelFormat::Undefined;
    if (depthFormat != pipeline.depthFormat())
    {
        throw std::invalid_argument("Pipeline/rendering depth attachment formats differ");
    }
}

void CommandBuffer::validateCopy(const ImageView& source, const ImageView& destination, const Device& device)
{
    if (!source.image || !destination.image || !device.isValid() ||
        source.image->device() != &device || destination.image->device() != &device)
    {
        throw std::invalid_argument("Copy image/device is invalid");
    }
    source.image->validateRange(source.range);
    destination.image->validateRange(destination.range);
    if (source.range.mipCount != 1 || source.range.layerCount != 1 ||
        destination.range.mipCount != 1 || destination.range.layerCount != 1 ||
        source.image->pixelFormat() != destination.image->pixelFormat() ||
        (isDepthFormat(source.image->pixelFormat()) && source.image->pixelFormat() != PixelFormat::D16_UNORM &&
            source.image->pixelFormat() != PixelFormat::D32_SFLOAT) ||
        source.image->mipExtent(source.range.baseMipLevel) != destination.image->mipExtent(destination.range.baseMipLevel) ||
        !hasFlag(source.image->usage(), ImageUsage::TransferSrc) || !hasFlag(destination.image->usage(), ImageUsage::TransferDst) ||
        source.image->currentLayout(source.range) != ImageLayout::TransferSrc ||
        destination.image->currentLayout(destination.range) != ImageLayout::TransferDst ||
        (source.image == destination.image && source.range == destination.range))
    {
        throw std::invalid_argument("Copy requires distinct, equally sized/formatted color or depth-only subresources in transfer layouts");
    }
}

void CommandBuffer::validateMipmapGeneration(const Image& image, const Device& device, ImageLayout finalLayout)
{
    image.validateRange(image.fullRange());
    image.validateLayout(finalLayout);
    if (!device.isValid() || image.device() != &device)
    {
        throw std::invalid_argument("Mipmap image must belong to the recording device");
    }
    for (uint32_t layer = 0; layer < image.arrayLayers(); ++layer)
    {
        if (image.currentLayout({0, 1, layer, 1}) == ImageLayout::Undefined)
        {
            throw std::invalid_argument("Every image layer needs initialized base-mip data before mipmap generation");
        }
    }
    if (image.mipLevels() == 1)
    {
        return; // A single-level image only needs the final transition.
    }
    if (isDepthFormat(image.pixelFormat()) || isUnsignedIntegerFormat(image.pixelFormat()) ||
        isSignedIntegerFormat(image.pixelFormat()) || !device.formatCapabilities(image.pixelFormat()).mipmapGeneration ||
        !hasFlag(image.usage(), ImageUsage::TransferSrc) || !hasFlag(image.usage(), ImageUsage::TransferDst))
    {
        throw std::invalid_argument("Filtered mipmap generation is unsupported for this format/usage");
    }
}

}
