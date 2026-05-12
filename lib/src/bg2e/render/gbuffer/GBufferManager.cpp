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

namespace bg2e::render {

GBufferManager::GBufferManager(Engine * engine)
    : _engine(engine)
{
    _colorFormats = {
        VK_FORMAT_R8G8B8A8_UNORM,        // albedo
        VK_FORMAT_R8G8B8A8_SNORM,        // normals (world space)
        VK_FORMAT_R8G8B8A8_UNORM,        // materials (metalness/R, roughness/G, AO/B, sheen/A)
        VK_FORMAT_R32G32B32A32_SFLOAT    // positions (world space)
    };
}

GBufferManager::~GBufferManager()
{
    cleanup();
}

void GBufferManager::build(VkExtent2D extent)
{
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
            format,
            extent,
            colorUsage,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,    // arrayLayers
            false, // useMipmaps
            0,    // maxMipmapLevels
            VK_SAMPLE_COUNT_1_BIT,
            "g-buffer color attachment " + std::to_string(i++)
        );
        _colorImages.push_back(std::shared_ptr<vulkan::Image>(image));
        _colorImagePtrs.push_back(image);
    }

    auto depth = vulkan::Image::createAllocatedImage(
        _engine,
        _depthFormat,
        extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1, false, 0, VK_SAMPLE_COUNT_1_BIT,
        "g-buffer depth attachment"
    );
    _depthImage = std::shared_ptr<vulkan::Image>(depth);

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
    transitionTo(
        cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void GBufferManager::beginRender(VkCommandBuffer cmd)
{
    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 0.0f } };
    auto clearRange = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    std::vector<VkRenderingAttachmentInfo> attachments;
    VkExtent2D imageExtent = _colorImages[0]->extent2D();
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

    vulkan::Image::cmdTransitionImage(
        cmd, _depthImage->handle(),
        _depthLayout,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );

    float depthValue = 1.0f;
    auto depthAttachment = vulkan::Info::depthAttachmentInfo(_depthImage->imageView(), depthValue);
    auto renderInfo = vulkan::Info::renderingInfo(
        imageExtent,
        attachments.data(),
        &depthAttachment,
        static_cast<uint32_t>(attachments.size())
    );
    vulkan::cmdBeginRendering(cmd, &renderInfo);

    _colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    _depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

    if (_depthLayout != depthLayout)
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
