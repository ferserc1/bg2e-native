/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/Info.hpp>

#include <algorithm>
#include <string>
#include <stdexcept>

namespace bg2e::render {

GBufferManager::GBufferManager(Engine * engine)
    : _engine(engine)
{
    _colorFormats = {
        VK_FORMAT_R8G8B8A8_UNORM,        // albedo
        VK_FORMAT_R16G16B16A16_SFLOAT,        // normals (world space), A = light emission
        VK_FORMAT_R8G8B8A8_UNORM,        // materials (metalness/R, roughness/G, AO/B, sheen/A)
        VK_FORMAT_R8G8B8A8_UNORM,        // fresnel color + flags (RGB = fresnelTint, A = material flags)
        VK_FORMAT_R8G8B8A8_UNORM,        // sheen color (RGB = sheenColor, A = refractionFactor)
        VK_FORMAT_R8G8B8A8_UNORM         // baked RGB light multiplier (neutral white when absent)
    };
}

GBufferManager::GBufferManager(Engine* engine, const Configuration& configuration)
    : _engine(engine),
      _colorFormats(configuration.colorFormats),
      _depthFormat(configuration.depthFormat)
{
    if (!_engine)
    {
        throw std::invalid_argument("GBufferManager: engine must not be null");
    }
    if (_colorFormats.empty())
    {
        throw std::invalid_argument("GBufferManager: at least one color format is required");
    }
}

GBufferManager::~GBufferManager()
{
    cleanup();
}

void GBufferManager::build(VkExtent2D extent)
{
    if (!_engine)
    {
        throw std::invalid_argument("GBufferManager::build: engine must not be null");
    }
    if (extent.width == 0 || extent.height == 0)
    {
        throw std::invalid_argument("GBufferManager::build: extent must be non-zero");
    }

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(_engine->physicalDevice().handle(), &deviceProperties);
    if (extent.width > deviceProperties.limits.maxImageDimension2D ||
        extent.height > deviceProperties.limits.maxImageDimension2D)
    {
        throw std::invalid_argument("GBufferManager::build: extent exceeds maxImageDimension2D");
    }
    if (_colorFormats.size() > deviceProperties.limits.maxColorAttachments)
    {
        throw std::runtime_error("GBufferManager::build: device has too few color attachment slots");
    }

    for (VkFormat format : _colorFormats)
    {
        VkFormatProperties formatProperties{};
        vkGetPhysicalDeviceFormatProperties(_engine->physicalDevice().handle(), format, &formatProperties);
        const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if ((formatProperties.optimalTilingFeatures & required) != required)
        {
            throw std::runtime_error("GBufferManager::build: color format " +
                std::to_string(static_cast<int>(format)) +
                " does not support optimal color attachment and sampled-image usage");
        }
    }
    if (_depthFormat != VK_FORMAT_UNDEFINED)
    {
        VkFormatProperties formatProperties{};
        vkGetPhysicalDeviceFormatProperties(_engine->physicalDevice().handle(), _depthFormat, &formatProperties);
        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
        {
            throw std::runtime_error("GBufferManager::build: configured depth format does not support depth attachments");
        }
    }

    cleanup();
    _extent = extent;

    const VkImageUsageFlags colorUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t i = 0;
    for (auto format : _colorFormats)
    {
        auto image = vulkan::Image::createAllocatedImage(
            _engine,
            "g-buffer color attachment " + std::to_string(i++),
            format,
            extent,
            colorUsage,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,    // arrayLayers
            false, // useMipmaps
            0,    // maxMipmapLevels
            VK_SAMPLE_COUNT_1_BIT
        );
        _colorImages.push_back(std::shared_ptr<vulkan::Image>(image));
        _colorImagePtrs.push_back(image);
    }

    if (_depthFormat != VK_FORMAT_UNDEFINED)
    {
        auto depth = vulkan::Image::createAllocatedImage(
            _engine,
            "g-buffer depth attachment",
            _depthFormat,
            extent,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1, false, 0, VK_SAMPLE_COUNT_1_BIT
        );
        _depthImage = std::shared_ptr<vulkan::Image>(depth);
    }

    _colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void GBufferManager::resize(VkExtent2D newExtent)
{
    build(newExtent);
}

void GBufferManager::cleanup()
{
    for (const auto & img : _colorImages)
    {
        img->cleanup();
    }

    if (_depthImage)
    {
        _depthImage->cleanup();
    }

    _colorImages.clear();
    _colorImagePtrs.clear();
    _depthImage.reset();
    _colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    _depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

uint32_t GBufferManager::imageCount() const
{
    return static_cast<uint32_t>(_colorImages.size());
}

std::shared_ptr<vulkan::Image> GBufferManager::image(uint32_t index) const
{
    return _colorImages[index];
}

std::shared_ptr<vulkan::Image> GBufferManager::depthImage() const
{
    return _depthImage;
}

const std::vector<const vulkan::Image*>& GBufferManager::images() const
{
    return _colorImagePtrs;
}

const std::vector<VkFormat>& GBufferManager::formats() const
{
    return _colorFormats;
}

VkFormat GBufferManager::depthFormat() const
{
    return _depthFormat;
}

VkExtent2D GBufferManager::extent() const
{
    return _extent;
}

void GBufferManager::transitionToClear(VkCommandBuffer cmd)
{
    transitionTo(
        cmd,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_GENERAL
    );
}
void GBufferManager::transitionToAttachment(VkCommandBuffer cmd)
{
    transitionTo(
        cmd,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
}

void GBufferManager::transitionToShaderRead(VkCommandBuffer cmd)
{
    // ALL_COMMANDS keeps the legacy camera G-buffer usable on devices without
    // ray-tracing support while also covering the UV pass's later compute/RT reads.
    transitionTo(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GBufferManager::beginRender(VkCommandBuffer cmd, bool isTransparent)
{
    beginRenderImpl(cmd, isTransparent, true);
}

void GBufferManager::beginRender(VkCommandBuffer cmd)
{
    if (_depthFormat != VK_FORMAT_UNDEFINED)
    {
        throw std::logic_error("GBufferManager::beginRender(cmd): this overload requires a depthless configuration");
    }
    beginRenderImpl(cmd, false, false);
}

void GBufferManager::beginRenderImpl(VkCommandBuffer cmd, bool isTransparent, bool includeDepth)
{
    if (_colorImages.empty())
    {
        throw std::logic_error("GBufferManager::beginRender: attachments have not been built");
    }

    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 0.0f } };
    auto clearRange = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    std::vector<VkRenderingAttachmentInfo> attachments;
    VkExtent2D imageExtent = _extent;
    for (auto image : _colorImages)
    {
        vulkan::Image::cmdTransitionImage(
            cmd, image->handle(),
            _colorLayout,
            VK_IMAGE_LAYOUT_GENERAL
        );

        vkCmdClearColorImage(
            cmd,
            image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );

        vulkan::Image::cmdTransitionImage(
            cmd, image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        auto colorAttachment = vulkan::Info::attachmentInfo(image->imageView(), nullptr);
        attachments.push_back(colorAttachment);
    }

    VkRenderingAttachmentInfo depthAttachment{};
    VkRenderingAttachmentInfo* depthAttachmentPtr = nullptr;
    if (includeDepth && _depthImage)
    {
        vulkan::Image::cmdTransitionImage(
            cmd, _depthImage->handle(),
            _depthLayout,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_DEPTH_BIT)
        );

        float depthValue = 1.0f;
        depthAttachment = vulkan::Info::depthAttachmentInfo(
            _depthImage->imageView(),
            depthValue,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            isTransparent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR
        );
        depthAttachmentPtr = &depthAttachment;
    }
    auto renderInfo = vulkan::Info::renderingInfo(
        imageExtent,
        attachments.data(),
        depthAttachmentPtr,
        static_cast<uint32_t>(attachments.size())
    );
    vulkan::cmdBeginRendering(cmd, &renderInfo);

    _colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (depthAttachmentPtr)
    {
        _depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
}

void GBufferManager::transitionTo(VkCommandBuffer cmd, VkImageLayout colorLayout, VkImageLayout depthLayout)
{
    if (_colorLayout != colorLayout)
    {
        for (auto &image : _colorImages)
        {
            vulkan::Image::cmdTransitionImage(cmd, image->handle(),
                _colorLayout,
                colorLayout);
        }
        _colorLayout = colorLayout;
    }

    if (_depthImage && _depthLayout != depthLayout)
    {
        vulkan::Image::TransitionInfo transitionInfo;
        transitionInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vulkan::Image::cmdTransitionImage(cmd, _depthImage->handle(),
            _depthLayout,
            depthLayout,
            transitionInfo
        );
        _depthLayout = depthLayout;
    }
}

}
