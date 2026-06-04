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

namespace bg2e {
namespace gpu {

enum class BackendType
{
    Vulkan,
    Metal
};

struct Size2D {
    uint32_t width  = 0;
    uint32_t height = 0;

    Size2D() = default;
    Size2D(uint32_t w, uint32_t h) : width(w), height(h) {}

    bool operator==(const Size2D& o) const { return width == o.width && height == o.height; }
    bool operator!=(const Size2D& o) const { return !(*this == o); }
    bool isZero() const { return width == 0 || height == 0; }
};

struct Size3D {
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t depth  = 1;

    Size3D() = default;
    Size3D(uint32_t w, uint32_t h, uint32_t d = 1) : width(w), height(h), depth(d) {}
    explicit Size3D(const Size2D& s, uint32_t d = 1) : width(s.width), height(s.height), depth(d) {}

    Size2D toSize2D() const { return Size2D{ width, height }; }

    bool operator==(const Size3D& o) const { return width == o.width && height == o.height && depth == o.depth; }
    bool operator!=(const Size3D& o) const { return !(*this == o); }
};

enum class PixelFormat {
    Undefined = 0,

    // --- Color ---
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_SFLOAT,
    R32G32B32A32_SFLOAT,

    // --- Depth / stencil ---
    D16_UNORM,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT
};

constexpr bool isDepthFormat(PixelFormat f)
{
    return f == PixelFormat::D16_UNORM
        || f == PixelFormat::D32_SFLOAT
        || f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

constexpr bool hasStencil(PixelFormat f)
{
    return f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

}
}
