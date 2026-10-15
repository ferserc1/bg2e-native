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
#include <bg2e/base/Image.hpp>

#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

static void setObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const std::string& name)
{
    if (!base::Log::isDebug() || name.empty() || setDebugUtilsObjectName == nullptr)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name.c_str();
    setDebugUtilsObjectName(device, &nameInfo);
}

namespace {
VkImageUsageFlags nativeUsage(ImageUsage usage)
{
    VkImageUsageFlags result = 0;
    if (hasFlag(usage, ImageUsage::ColorAttachment))
    {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (hasFlag(usage, ImageUsage::DepthStencil))
    {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (hasFlag(usage, ImageUsage::Sampled))
    {
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (hasFlag(usage, ImageUsage::Storage))
    {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (hasFlag(usage, ImageUsage::TransferSrc))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(usage, ImageUsage::TransferDst))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    return result;
}
ImageUsage genericUsage(VkImageUsageFlags usage)
{
    ImageUsage result = ImageUsage::None;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        result |= ImageUsage::ColorAttachment;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        result |= ImageUsage::DepthStencil;
    }
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        result |= ImageUsage::Sampled;
    }
    if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        result |= ImageUsage::Storage;
    }
    if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        result |= ImageUsage::TransferSrc;
    }
    if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        result |= ImageUsage::TransferDst;
    }
    return result;
}
}

void Image::buildImage(vk::Device* device, const ImageDescription& description)
{
    gpu::Image::validateDescription(description);
    if (!device || !device->isValid() || !device->formatCapabilities(description.format).supports(description.usage))
    {
        throw std::invalid_argument("Vulkan image device/format/usage is unsupported");
    }
    const auto format = toVkFormat(description.format);
    const auto flags = description.type == ImageType::Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u;
    const auto usages = nativeUsage(description.usage);
    VkImageFormatProperties properties{};
    const auto status = vkGetPhysicalDeviceImageFormatProperties(device->physicalDeviceHandle(), format,
        VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usages, flags, &properties);
    const auto levels = effectiveMipLevels(description);
    if (status != VK_SUCCESS || description.size.width > properties.maxExtent.width ||
        description.size.height > properties.maxExtent.height || levels > properties.maxMipLevels)
    {
        throw std::invalid_argument("Vulkan image extent/mips/usage combination is unsupported");
    }
    if (isValid() && _device)
    {
        _device->waitIdle();
    }
    cleanup();
    _device = device;
    DeviceResource::_device = device;
    _size = description.size;
    _pixelFormat = description.format;
    _imageType = description.type;
    _mipLevels = levels;
    _vkFormat = format;
    _usage = usages;
    _aspect = isDepthFormat(_pixelFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (hasStencil(_pixelFormat))
    {
        _aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    _ownsImage = true;
    _debugName = description.debugName;
    resetLayoutTracking();
    auto info = Info::imageCreateInfo(_vkFormat, _usage, {width(), height(), 1}, arrayLayers(), VK_SAMPLE_COUNT_1_BIT);
    info.mipLevels = _mipLevels;
    info.flags = flags;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo allocation{};
    allocation.usage = VMA_MEMORY_USAGE_AUTO;
    allocation.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VK_ASSERT(vmaCreateImage(device->allocator(), &info, &allocation, &_image, &_allocation, nullptr));
    if (hasFlag(description.usage, ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::ColorAttachment | ImageUsage::DepthStencil))
    {
        createView();
    }
    if (!_debugName.empty())
    {
        vmaSetAllocationName(device->allocator(), _allocation, _debugName.c_str());
        setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(_image), _debugName);
        if (_imageView != VK_NULL_HANDLE)
        {
            setObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(_imageView), _debugName + "_view");
        }
    }
}

void Image::buildTargetImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format,
        .usage = ImageUsage::ColorAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst,
        .debugName = debugName});
}
void Image::buildDepthImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format,
        .usage = ImageUsage::DepthStencil | ImageUsage::Sampled, .debugName = debugName});
}
void Image::buildSampledImage(vk::Device* device, const Size2D& size, PixelFormat format, VkImageUsageFlags usage, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format, .usage = genericUsage(usage), .debugName = debugName});
}
void Image::buildCubemapImage(vk::Device* device, const Size2D& size, PixelFormat format,
                             VkImageUsageFlags usage, uint32_t levels, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format, .usage = genericUsage(usage),
        .type = ImageType::Cubemap, .mipLevels = levels, .debugName = debugName});
}

ImageUsage Image::usage() const
{
    return genericUsage(_usage) | (!_ownsImage ? ImageUsage::Present : ImageUsage::None);
}

void Image::createView()
{
    auto info = Info::imageViewCreateInfo(_vkFormat, _image, _aspect);
    info.viewType = isCubemap() ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    info.subresourceRange.levelCount = _mipLevels;
    info.subresourceRange.layerCount = arrayLayers();
    VK_ASSERT(vkCreateImageView(_device->handle(), &info, nullptr, &_imageView));
}

VkImageView Image::subresourceView(const ImageSubresourceRange& range, bool attachment) const
{
    validateRange(range);
    if (_imageView == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("Transfer-only images do not expose native image views");
    }
    if (range.layerCount != 1 && !(isCubemap() && range.baseArrayLayer == 0 && range.layerCount == 6))
    {
        throw std::invalid_argument("Vulkan view must select one layer or all cube faces");
    }
    if (range == fullRange() && (attachment || !hasStencil(_pixelFormat)))
    {
        return _imageView;
    }
    const std::array<uint32_t, 5> key = {range.baseMipLevel, range.mipCount,
        range.baseArrayLayer, range.layerCount, attachment ? 1u : 0u};
    const auto found = _subresourceViews.find(key);
    if (found != _subresourceViews.end())
    {
        return found->second;
    }
    auto info = Info::imageViewCreateInfo(_vkFormat, _image, _aspect);
    info.viewType = range.layerCount == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    info.subresourceRange = {_aspect, range.baseMipLevel, range.mipCount, range.baseArrayLayer, range.layerCount};
    if (!attachment && isDepthFormat(_pixelFormat))
    {
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    VkImageView result = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateImageView(_device->handle(), &info, nullptr, &result));
    _subresourceViews.emplace(key, result);
    return result;
}

VkImageView Image::cubemapFaceMipView(uint32_t face, uint32_t mipLevel) const
{
    if (!isCubemap() || face >= 6 || mipLevel >= _mipLevels)
    {
        return VK_NULL_HANDLE;
    }
    return subresourceView({mipLevel, 1, face, 1}, true);
}

void Image::initFromSwapchainImage(vk::Device* device, VkImage image, PixelFormat format, const Size2D& size, VkImageUsageFlags usage)
{
    cleanup();
    _device = device;
    DeviceResource::_device = device;
    _image = image;
    _size = size;
    _pixelFormat = format;
    _imageType = ImageType::Image2D;
    _mipLevels = 1;
    _vkFormat = toVkFormat(format);
    _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _allocation = VK_NULL_HANDLE;
    _usage = usage;
    _ownsImage = false;
    resetLayoutTracking();
    createView();
}

void Image::resize(const Size2D& size)
{
    if (!_ownsImage)
    {
        throw std::logic_error("Cannot resize a wrapped image");
    }
    ImageDescription description{.size = size, .format = _pixelFormat, .usage = usage(), .type = _imageType,
        .mipLevels = std::min(_mipLevels, mipLevelsForMinSize(std::max(size.width, size.height), 1)), .debugName = _debugName};
    buildImage(_device, description);
}

void Image::cleanup()
{
    for (const auto& entry : _subresourceViews)
    {
        vkDestroyImageView(_device->handle(), entry.second, nullptr);
    }
    _subresourceViews.clear();
    if (_imageView != VK_NULL_HANDLE && _device)
    {
        vkDestroyImageView(_device->handle(), _imageView, nullptr);
        _imageView = VK_NULL_HANDLE;
    }
    if (_ownsImage && _image != VK_NULL_HANDLE && _device)
    {
        vmaDestroyImage(_device->allocator(), _image, _allocation);
    }
    _image = VK_NULL_HANDLE;
    _allocation = VK_NULL_HANDLE;
    resetLayoutTracking();
}

void Image::upload(const ImageUploadDescription& description, ImageLayout finalLayout)
{
    const auto sourceStride = validateUpload(description);
    validateLayout(finalLayout);
    _device->waitIdle();
    const auto extent = mipExtent(description.mipLevel);
    const size_t stride = size_t(extent.width) * pixelFormatBytes(_pixelFormat);
    const VkDeviceSize bytes = stride * extent.height;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = bytes;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo mapped{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &info, &allocationInfo, &buffer, &allocation, &mapped));
    for (uint32_t row = 0; row < extent.height; ++row)
    {
        std::memcpy(static_cast<uint8_t*>(mapped.pMappedData) + row * stride,
            static_cast<const uint8_t*>(description.data) + row * sourceStride, stride);
    }
    const ImageSubresourceRange range{description.mipLevel, 1, description.arrayLayer, 1};
    try
    {
        VK_ASSERT(vmaFlushAllocation(_device->allocator(), allocation, 0, VK_WHOLE_SIZE));
        _device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(this, ImageLayout::TransferDst, range);
            VkBufferImageCopy region{};
            region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, range.baseMipLevel, range.baseArrayLayer, 1};
            region.imageExtent = {extent.width, extent.height, 1};
            vkCmdCopyBufferToImage(static_cast<vk::CommandBuffer*>(command)->handle(), buffer,
                _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            command->transition(this, finalLayout, range);
        });
    }
    catch (...)
    {
        vmaDestroyBuffer(_device->allocator(), buffer, allocation);
        throw;
    }
    vmaDestroyBuffer(_device->allocator(), buffer, allocation);
}

void Image::readPixels(std::vector<uint8_t>& data, const ImageSubresourceRange& range, ImageLayout finalLayout)
{
    validateRange(range);
    validateLayout(finalLayout);
    if (range.mipCount != 1 || range.layerCount != 1 || isDepthFormat(_pixelFormat) ||
        !hasFlag(usage(), ImageUsage::TransferSrc) || currentLayout(range) == ImageLayout::Undefined)
    {
        throw std::invalid_argument("Readback requires one initialized color subresource with TransferSrc usage");
    }
    _device->waitIdle();
    const auto extent = mipExtent(range.baseMipLevel);
    const size_t bytes = size_t(extent.width) * extent.height * pixelFormatBytes(_pixelFormat);
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = bytes;
    info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo mapped{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &info, &allocationInfo, &buffer, &allocation, &mapped));
    try
    {
        _device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(this, ImageLayout::TransferSrc, range);
            VkBufferImageCopy region{};
            region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, range.baseMipLevel, range.baseArrayLayer, 1};
            region.imageExtent = {extent.width, extent.height, 1};
            vkCmdCopyImageToBuffer(static_cast<vk::CommandBuffer*>(command)->handle(), _image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
            command->transition(this, finalLayout, range);
        });
        VK_ASSERT(vmaInvalidateAllocation(_device->allocator(), allocation, 0, VK_WHOLE_SIZE));
        data.resize(bytes);
        std::memcpy(data.data(), mapped.pMappedData, bytes);
    }
    catch (...)
    {
        vmaDestroyBuffer(_device->allocator(), buffer, allocation);
        throw;
    }
    vmaDestroyBuffer(_device->allocator(), buffer, allocation);
}

void Image::readPixelsRGBA8(std::vector<uint8_t>& data, ImageLayout finalLayout)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM)
    {
        throw std::invalid_argument("readPixelsRGBA8 requires R8G8B8A8_UNORM");
    }
    readPixels(data, {}, finalLayout);
}
void Image::uploadRGBA8(const void* pixels, const Size2D& size)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM || size != _size)
    {
        throw std::invalid_argument("uploadRGBA8 requires matching RGBA8 dimensions");
    }
    upload({pixels, size_t(width()) * height() * 4});
}
void Image::uploadImage(const base::Image* image)
{
    gpu::Image::uploadImage(image);
}

}
}
}
