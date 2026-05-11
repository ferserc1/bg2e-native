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
            VK_SAMPLE_COUNT_1_BIT
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
        1, false, 0, VK_SAMPLE_COUNT_1_BIT
    );
    _depthImage = std::shared_ptr<vulkan::Image>(depth);
}

void GBufferManager::resize(VkExtent2D newExtent)
{
    build(newExtent);
}

void GBufferManager::cleanup()
{
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

void GBufferManager::transitionToAttachment(VkCommandBuffer cmd)
{
    for (auto &image : _colorImages)
    {
        vulkan::Image::cmdTransitionImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    vulkan::Image::cmdTransitionImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void GBufferManager::transitionToShaderRead(VkCommandBuffer cmd)
{
    for (auto &image : _colorImages)
    {
        vulkan::Image::cmdTransitionImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    vulkan::Image::cmdTransitionImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GBufferManager::clear(VkCommandBuffer cmd)
{
    VkClearColorValue black{ { 0.0f, 0.0f, 0.0f, 0.0f } };
    for (auto &image : _colorImages)
    {
        auto range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &black, 1, &range);
    }

    VkClearDepthStencilValue depthClear{ 1.0f, 0 };
    auto depthRange = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
    vkCmdClearDepthStencilImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthClear, 1, &depthRange);
}

}
