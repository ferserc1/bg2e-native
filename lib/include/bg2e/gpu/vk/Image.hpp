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

#pragma once

#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <vector>
#include <map>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;

class Image : public gpu::Image {
  public:
    explicit Image(gpu::Device* device = nullptr) : gpu::Image(device)
    {
    }
    ~Image() override
    {
        cleanup();
    }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void buildTargetImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName = {});
    void buildDepthImage(vk::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName = {});
    void buildSampledImage(vk::Device* device, const Size2D& size, PixelFormat format, VkImageUsageFlags usage, const std::string& debugName = {});
    void buildImage(vk::Device* device, const ImageDescription& description);
    void upload(const ImageUploadDescription& description, ImageLayout finalLayout = ImageLayout::ShaderReadOnly) override;
    void readPixels(std::vector<uint8_t>& data, const ImageSubresourceRange& range,
                    ImageLayout finalLayout = ImageLayout::ShaderReadOnly) override;
    ImageUsage usage() const override;
    void resize(const Size2D& size);

    void cleanup() override;
    bool isValid() const override
    {
        return _image != VK_NULL_HANDLE;
    }

    // Backend dependent functions
    void initFromSwapchainImage(vk::Device* device, VkImage image, PixelFormat format, const Size2D& size,
                                VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    VkImage handle() const
    {
        return _image;
    }
    VkImageView imageView() const
    {
        return _imageView;
    }
    VkFormat vkFormat() const
    {
        return _vkFormat;
    }
    VkImageAspectFlags aspect() const
    {
        return _aspect;
    }

    VkImageView subresourceView(const ImageSubresourceRange& range, bool attachment = false) const;

    VkImageView cubemapFaceMipView(uint32_t face, uint32_t mipLevel) const;

    void readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout currentLayout = ImageLayout::ColorAttachment) override;
    void readPixelsDepth32(std::vector<float>& outData, ImageLayout currentLayout = ImageLayout::TransferSrc) override;
    void uploadRGBA8(const void* pixels, const Size2D& size) override;
    void uploadImage(const base::Image* image) override;

    void buildCubemapImage(
        vk::Device* device,
        const Size2D& size,
        PixelFormat format,
        VkImageUsageFlags usage,
        uint32_t mipLevels,
        const std::string& debugName = {});

  private:
    void createView();


    vk::Device* _device = nullptr;
    VkImage _image = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VkFormat _vkFormat = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    bool _ownsImage = false;
    VkImageUsageFlags _usage = 0;
    std::string _debugName;
    mutable std::map<std::array<uint32_t, 5>, VkImageView> _subresourceViews;
};

}
}
}
