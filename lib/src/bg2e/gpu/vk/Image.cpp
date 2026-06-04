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

#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/Common.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

void Image::buildTargetImage(vk::Device* device, const Size2D& size, PixelFormat format)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _vkFormat = toVkFormat(format);
    _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
           | VK_IMAGE_USAGE_SAMPLED_BIT
           | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
           | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    _ownsImage = true;

    VkExtent3D extent{};
    extent.width = size.width;
    extent.height = size.height;
    extent.depth = 1;

    VkImageCreateInfo imgInfo = Info::imageCreateInfo(
        _vkFormat, _usage, extent, 1, VK_SAMPLE_COUNT_1_BIT
    );
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_ASSERT(vmaCreateImage(
        device->allocator(), &imgInfo, &allocInfo, &_image, &_allocation, nullptr
    ));

    createView();
}

void Image::buildDepthImage(vk::Device* device, const Size2D& size, PixelFormat format)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _vkFormat = toVkFormat(format);
    _aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencil(format))
    {
        _aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    _usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
           | VK_IMAGE_USAGE_SAMPLED_BIT;
    _ownsImage = true;

    VkExtent3D extent{};
    extent.width = size.width;
    extent.height = size.height;
    extent.depth = 1;

    VkImageCreateInfo imgInfo = Info::imageCreateInfo(
        _vkFormat, _usage, extent, 1, VK_SAMPLE_COUNT_1_BIT
    );
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_ASSERT(vmaCreateImage(
        device->allocator(), &imgInfo, &allocInfo, &_image, &_allocation, nullptr
    ));

    createView();
}

void Image::initFromSwapchainImage(vk::Device* device, VkImage image, PixelFormat format, const Size2D& size)
{
    cleanup();

    _device = device;
    _image = image;
    _size = size;
    _pixelFormat = format;
    _vkFormat = toVkFormat(format);
    _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _allocation = VK_NULL_HANDLE;
    _ownsImage = false;

    createView();
}

void Image::createView()
{
    VkImageViewCreateInfo viewInfo = Info::imageViewCreateInfo(
        _vkFormat, _image, _aspect
    );

    VK_ASSERT(vkCreateImageView(
        _device->handle(), &viewInfo, nullptr, &_imageView
    ));
}

void Image::resize(const Size2D& size)
{
    if (!_ownsImage)
    {
        throw std::runtime_error("vk::Image::resize: cannot resize a wrapped image");
    }

    PixelFormat storedFormat = _pixelFormat;
    bool wasDepth = isDepthFormat(storedFormat);

    cleanup();

    _size = size;

    if (wasDepth)
    {
        buildDepthImage(_device, size, storedFormat);
    }
    else
    {
        buildTargetImage(_device, size, storedFormat);
    }
}

void Image::cleanup()
{
    if (_imageView != VK_NULL_HANDLE && _device != nullptr)
    {
        vkDestroyImageView(_device->handle(), _imageView, nullptr);
        _imageView = VK_NULL_HANDLE;
    }
    if (_ownsImage && _image != VK_NULL_HANDLE && _device != nullptr)
    {
        vmaDestroyImage(_device->allocator(), _image, _allocation);
        _image = VK_NULL_HANDLE;
    }
    _allocation = VK_NULL_HANDLE;
}

}
}
}