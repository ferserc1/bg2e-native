/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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
#include <cstdint>
#include <functional>
#include <vector>

namespace bg2e {
namespace gpu {

class Device;

struct BG2E_API ImageTransitionInfo {
    uint32_t aspectFlags = 0;
    uint32_t mipLevel = 0;
    uint32_t mipLevelsCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
    uint64_t srcStageMask = 0;
    uint64_t dstStageMask = 0;
    uint32_t srcAccessMask = 0;
    uint32_t dstAccessMask = 0;
};

class BG2E_API Image {
public:
    virtual ~Image() = default;

    static void cmdTransitionImage(void* commandBuffer, void* image, uint32_t oldLayout, uint32_t newLayout, const ImageTransitionInfo& info);
    static void transitionImage(Device* device, void* image, uint32_t oldLayout, uint32_t newLayout, const ImageTransitionInfo& info);
    static void cmdCopy(void* commandBuffer, void* srcImage, uint32_t srcWidth, uint32_t srcHeight, void* dstImage, uint32_t dstWidth, uint32_t dstHeight);

    virtual void cleanup() = 0;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
};

}
}
