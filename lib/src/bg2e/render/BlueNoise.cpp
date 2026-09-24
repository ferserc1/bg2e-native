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

#include <bg2e/render/BlueNoise.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <cstring>
#include <memory>
#include <vector>

// Self-descriptive precomputed data: defines BG2E_BLUE_NOISE_WIDTH,
// BG2E_BLUE_NOISE_HEIGHT, BG2E_BLUE_NOISE_CHANNELS, BG2E_BLUE_NOISE_LAYERS
// and bg2e_blue_noise_data. Replace this file to swap the noise data.
#include "blue_noise_data.h"

namespace bg2e::render {

static VkFormat blueNoiseFormat()
{
#if BG2E_BLUE_NOISE_CHANNELS == 1
    return VK_FORMAT_R8_UNORM;
#elif BG2E_BLUE_NOISE_CHANNELS == 2
    return VK_FORMAT_R8G8_UNORM;
#elif BG2E_BLUE_NOISE_CHANNELS == 3
    return VK_FORMAT_R8G8B8_UNORM;
#elif BG2E_BLUE_NOISE_CHANNELS == 4
    return VK_FORMAT_R8G8B8A8_UNORM;
#else
#error "Unsupported BG2E_BLUE_NOISE_CHANNELS value (expected 1-4)"
#endif
}

BlueNoise::BlueNoise(Engine* engine)
    : _engine{engine}
{
}

BlueNoise::~BlueNoise()
{
    cleanup();
}

void BlueNoise::build()
{
    _size = glm::uvec2(BG2E_BLUE_NOISE_WIDTH, BG2E_BLUE_NOISE_HEIGHT);
    _layerCount = BG2E_BLUE_NOISE_LAYERS;

    const size_t layerSize = size_t(_size.x) * _size.y * BG2E_BLUE_NOISE_CHANNELS;
    const size_t dataSize = layerSize * _layerCount;

    auto uploadBuffer = std::unique_ptr<vulkan::Buffer>(
        vulkan::Buffer::createAllocatedBuffer(
            _engine,
            dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            "BlueNoise upload buffer"
        )
    );
    memcpy(uploadBuffer->allocatedData(), bg2e_blue_noise_data, dataSize);
    uploadBuffer->flushAllocatedData();

    auto imgInfo = vulkan::Info::imageCreateInfo(
        blueNoiseFormat(),
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        { _size.x, _size.y, 1 },
        _layerCount,
        VK_SAMPLE_COUNT_1_BIT
    );

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(
        _engine->allocator(),
        &imgInfo,
        &allocInfo,
        &_image,
        &_allocation,
        nullptr
    );
    vmaSetAllocationName(_engine->allocator(), _allocation, "BlueNoise image");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = blueNoiseFormat();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = _layerCount;
    VK_ASSERT(vkCreateImageView(_engine->device().handle(), &viewInfo, nullptr, &_imageView));

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        vulkan::Image::cmdTransitionImage(
            cmd,
            _image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, _layerCount)
        );

        std::vector<VkBufferImageCopy> regions(_layerCount);
        for (uint32_t layer = 0; layer < _layerCount; ++layer)
        {
            VkBufferImageCopy& region = regions[layer];
            region.bufferOffset = layerSize * layer;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = layer;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = { _size.x, _size.y, 1 };
        }
        vkCmdCopyBufferToImage(
            cmd,
            uploadBuffer->handle(),
            _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(regions.size()),
            regions.data()
        );

        vulkan::Image::cmdTransitionImage(
            cmd,
            _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, _layerCount)
        );
    });

    uploadBuffer->cleanup();

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build(
        VK_FILTER_NEAREST, VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT
    );

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });
}

void BlueNoise::cleanup()
{
    if (_imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(_engine->device().handle(), _imageView, nullptr);
        _imageView = VK_NULL_HANDLE;
    }
    if (_image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(_engine->allocator(), _image, _allocation);
        _image = VK_NULL_HANDLE;
        _allocation = VK_NULL_HANDLE;
    }
}

}
