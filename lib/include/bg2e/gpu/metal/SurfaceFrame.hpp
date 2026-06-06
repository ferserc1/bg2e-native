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

#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <memory>

namespace bg2e {
namespace gpu {
namespace metal {

class Image;

class SurfaceFrame : public gpu::SurfaceFrame {
public:
    gpu::Image* colorImage() const override;
    gpu::Image* depthImage() const override { return _depthImage; }
    bool        isValid()    const override;

#if BG2E_IS_MAC
    void setDrawable(CA::MetalDrawable* d)            { _drawable = d; }
    CA::MetalDrawable* drawable() const               { return _drawable; }
#endif

    void setColorImage(std::unique_ptr<metal::Image> img);
    void setColorImageRef(metal::Image* img)     { _colorImageRaw = img; }
    void setDepthImage(gpu::Image* img) { _depthImage = img; }

private:
    std::unique_ptr<metal::Image> _colorImage;
    metal::Image*                 _colorImageRaw = nullptr;
    gpu::Image*                   _depthImage = nullptr;
#if BG2E_IS_MAC
    CA::MetalDrawable*            _drawable   = nullptr;
#endif
};

}
}
}