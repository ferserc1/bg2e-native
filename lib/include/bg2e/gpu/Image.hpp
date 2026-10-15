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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <stdexcept>
#include <vector>

namespace bg2e {
namespace base {
class Image;
}
namespace gpu {

namespace vk {
class CommandBuffer;
}
namespace metal {
class CommandBuffer;
}

class BG2E_API Image : public DeviceResource {
  public:
    explicit Image(Device* device) : DeviceResource(device)
    {
    }
    virtual ~Image() = default;

    virtual ImageUsage usage() const
    {
        return ImageUsage::None;
    }
    PixelFormat pixelFormat() const
    {
        return _pixelFormat;
    }
    const Size2D& size() const
    {
        return _size;
    }
    uint32_t width() const
    {
        return _size.width;
    }
    uint32_t height() const
    {
        return _size.height;
    }

    ImageType imageType() const
    {
        return _imageType;
    }
    uint32_t mipLevels() const
    {
        return _mipLevels;
    }
    bool isCubemap() const
    {
        return _imageType == ImageType::Cubemap;
    }

    uint32_t arrayLayers() const
    {
        return isCubemap() ? 6u : 1u;
    }
    ImageSubresourceRange fullRange() const
    {
        return {0, _mipLevels, 0, arrayLayers()};
    }
    Size2D mipExtent(uint32_t mipLevel) const;
    void validateRange(const ImageSubresourceRange& range) const;
    ImageView view(const ImageSubresourceRange& range);
    ImageView fullView();
    // Throws for a mixed-layout range; never reports Undefined for valid data
    // merely because another mip/layer has a different layout.
    ImageLayout currentLayout() const;
    ImageLayout currentLayout(const ImageSubresourceRange& range) const;
    void validateLayout(ImageLayout layout) const;
    size_t validateUpload(const ImageUploadDescription& description) const;

    // Synchronous rare-edit conveniences: callers stop producers. Implementations
    // wait for outstanding users and complete staging work before returning.
    virtual void upload(const ImageUploadDescription& description, ImageLayout finalLayout = ImageLayout::ShaderReadOnly);
    virtual void readPixels(std::vector<uint8_t>& data, const ImageSubresourceRange& range,
                            ImageLayout finalLayout = ImageLayout::ShaderReadOnly);
    void generateMipmaps(ImageLayout finalLayout = ImageLayout::ShaderReadOnly);
    static void validateDescription(const ImageDescription& description);

    virtual void readPixelsRGBA8(
        std::vector<uint8_t>& outData,
        ImageLayout currentLayout = ImageLayout::ColorAttachment)
    {
        throw std::runtime_error("Image::readPixelsRGBA8 not implemented");
    }

    virtual void uploadRGBA8(const void* pixels, const Size2D& size)
    {
        throw std::runtime_error("Image::uploadRGBA8 not implemented");
    }

    virtual void uploadImage(const base::Image* image);

  protected:
    void setLayout(const ImageSubresourceRange& range, ImageLayout layout);
    void resetLayoutTracking();
    std::vector<ImageLayout> _subresourceLayouts;
    PixelFormat _pixelFormat = PixelFormat::Undefined;
    Size2D _size;
    // Uniform initial layout before the per-mip/layer table is materialized.
    // Native commands update the table; builders reset it on allocation.
    ImageLayout _currentLayout = ImageLayout::Undefined;
    ImageType _imageType = ImageType::Image2D;
    uint32_t _mipLevels = 1;

    friend class vk::CommandBuffer;
    friend class metal::CommandBuffer;
};

}
}