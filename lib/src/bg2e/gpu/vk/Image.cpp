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
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/base/Log.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

static void setObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const std::string& name)
{
    if (!base::Log::isDebug() || name.empty() || setDebugUtilsObjectName == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name.c_str();
    setDebugUtilsObjectName(device, &nameInfo);
}

void Image::buildTargetImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
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
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

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

    if (!debugName.empty())
    {
        vmaSetAllocationName(device->allocator(), _allocation, debugName.c_str());
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(_image), debugName);
    }

    createView();

    if (!debugName.empty())
    {
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(_imageView), debugName + "_view");
    }
}

void Image::buildDepthImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
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
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

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

    if (!debugName.empty())
    {
        vmaSetAllocationName(device->allocator(), _allocation, debugName.c_str());
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(_image), debugName);
    }

    createView();

    if (!debugName.empty())
    {
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(_imageView), debugName + "_view");
    }
}

void Image::buildSampledImage(vk::Device* device, const Size2D& size, PixelFormat format, VkImageUsageFlags usage, const std::string& debugName)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _vkFormat = toVkFormat(format);
    _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _ownsImage = true;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

    VkExtent3D extent{};
    extent.width = size.width;
    extent.height = size.height;
    extent.depth = 1;

    VkImageCreateInfo imgInfo = Info::imageCreateInfo(
        _vkFormat, usage, extent, 1, VK_SAMPLE_COUNT_1_BIT
    );
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_ASSERT(vmaCreateImage(
        device->allocator(), &imgInfo, &allocInfo, &_image, &_allocation, nullptr
    ));

    if (!debugName.empty())
    {
        vmaSetAllocationName(device->allocator(), _allocation, debugName.c_str());
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(_image), debugName);
    }

    createView();

    if (!debugName.empty())
    {
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(_imageView), debugName + "_view");
    }
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
    _currentLayout = ImageLayout::Undefined;

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
        buildDepthImage(_device, size, storedFormat, _debugName);
    }
    else
    {
        buildTargetImage(_device, size, storedFormat, _debugName);
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

void Image::readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout currentLayout)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("vk::Image::readPixelsRGBA8: only R8G8B8A8_UNORM is supported");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;
    const VkDeviceSize imageSize = VkDeviceSize(w) * h * 4;

    VkBuffer staging = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
    allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo allocOut{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                              &staging, &stagingAlloc, &allocOut));

    // _currentLayout is kept in sync with the real Vulkan layout by every
    // CommandBuffer::transition() call, so it already reflects the image's
    // actual layout and must NOT be overwritten here. Assigning _currentLayout
    // directly would only change the tracked flag (not the image), which could
    // make the transition below compute a wrong oldLayout and, when it happens
    // to match newLayout, skip a barrier that is actually required.
    //
    // The `currentLayout` argument only selects the layout the image is restored
    // to once the copy-to-buffer has finished.
    const ImageLayout restoreLayout = currentLayout;

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        // Source layout is taken from the real, tracked layout of the image.
        cmd->transition(this, ImageLayout::TransferSrc);

        VkCommandBuffer raw = dynamic_cast<vk::CommandBuffer*>(cmd)->handle();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { w, h, 1 };

        vkCmdCopyImageToBuffer(raw, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               staging, 1, &region);

        cmd->transition(this, restoreLayout);
    });

    outData.resize(imageSize);
    std::memcpy(outData.data(), allocOut.pMappedData, imageSize);

    vmaDestroyBuffer(_device->allocator(), staging, stagingAlloc);
}

void Image::uploadRGBA8(const void* pixels, const Size2D& size)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("vk::Image::uploadRGBA8: only R8G8B8A8_UNORM is supported");
    }

    if (size.width != _size.width || size.height != _size.height) {
        throw std::runtime_error("vk::Image::uploadRGBA8: size mismatch");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;
    const VkDeviceSize bufferSize = VkDeviceSize(w) * h * 4;

    VkBuffer staging = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = bufferSize;
    bufInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo allocOut{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                              &staging, &stagingAlloc, &allocOut));

    std::memcpy(allocOut.pMappedData, pixels, bufferSize);

    // Do not force the tracked layout here. _currentLayout already reflects the
    // image's real Vulkan layout (e.g. Undefined for a freshly created image),
    // and the transition below uses it as the source layout. Overwriting it
    // would make the barrier declare a wrong oldLayout and, if it happened to
    // match TransferDst, skip the required transition entirely. The image is
    // left in ShaderReadOnly by the final transition, which keeps the tracked
    // layout in sync with the real one.

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        // Source layout is taken from the real, tracked layout of the image.
        cmd->transition(this, ImageLayout::TransferDst);

        VkCommandBuffer raw = dynamic_cast<vk::CommandBuffer*>(cmd)->handle();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { w, h, 1 };

        vkCmdCopyBufferToImage(raw, staging, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

        cmd->transition(this, ImageLayout::ShaderReadOnly);
    });

    vmaDestroyBuffer(_device->allocator(), staging, stagingAlloc);
}

}
}
}