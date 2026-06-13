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
namespace gpu {

namespace vk    { class CommandBuffer; }
namespace metal { class CommandBuffer; }

class BG2E_API Image : public DeviceResource {
public:
    explicit Image(Device* device) : DeviceResource(device) {}
    virtual ~Image() = default;

    PixelFormat   pixelFormat() const { return _pixelFormat; }
    const Size2D& size()        const { return _size; }
    uint32_t      width()       const { return _size.width;  }
    uint32_t      height()      const { return _size.height; }

    ImageLayout   currentLayout() const { return _currentLayout; }

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

protected:
    // _currentLayout must always mirror the real backend layout of the image.
    // Assign it directly only right after the API call that actually changes the
    // layout (e.g. a pipeline barrier), so the tracked value stays in sync.
    PixelFormat   _pixelFormat   = PixelFormat::Undefined;
    Size2D        _size;
    ImageLayout   _currentLayout = ImageLayout::Undefined;

    friend class vk::CommandBuffer;
    friend class metal::CommandBuffer;
};

}
}