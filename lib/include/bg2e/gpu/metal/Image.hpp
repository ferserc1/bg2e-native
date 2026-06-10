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
#include <bg2e/gpu/metal/common.hpp>
#include <vector>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;

class Image : public gpu::Image {
public:
    Image() = default;
    ~Image() override { cleanup(); }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void buildTargetImage(metal::Device* device, const Size2D& size, PixelFormat format);
    void buildDepthImage(metal::Device* device, const Size2D& size, PixelFormat format);

    void resize(const Size2D& size);

    void cleanup() override;
    bool isValid() const override;

    // Backend dependent functions
    void initFromDrawableTexture(
        metal::Device* device,
        TextureHandle texture,
        PixelFormat format,
        const Size2D& size
    );
    TextureHandle texture() const { return _texture; }

    void readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout currentLayout = ImageLayout::ColorAttachment) override;

private:
    metal::Device* _device      = nullptr;
    TextureHandle  _texture     = nullptr;
    bool           _isDepth     = false;
    bool           _ownsTexture = true;
};

}
}
}
