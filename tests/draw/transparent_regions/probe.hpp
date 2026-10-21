/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/draw/pipeline/ProcessingRegion.hpp>
#include <limits>
#include <stdexcept>

namespace bg2e::draw::tests {
// CPU-only source probe; no device, shaders or command submission required.
inline void runTransparentRegionProbe()
{
    const auto require = [](bool condition)
    {
        if (!condition)
        {
            throw std::runtime_error("Conservative transparent region probe failed");
        }
    };
    const gpu::Size2D extent{100, 80};
    const glm::mat4 identity{1};
    auto region = pipeline::projectProcessingRegion({-0.1f,-0.1f,0.4f}, {0.1f,0.1f,0.6f}, identity, extent);
    require(region && region->x <= 45 && region->y <= 36 &&
        region->x+region->width >= 55 && region->y+region->height >= 44 &&
        region->width < extent.width && region->height < extent.height);
    region = pipeline::projectProcessingRegion({2,0,0.4f}, {3,1,0.6f}, identity, extent);
    require(region && region->width == 0);
    region = pipeline::projectProcessingRegion({-1.1f,0,0.4f}, {-0.9f,0.1f,0.6f}, identity, extent);
    require(region && region->x == 0 && region->width > 0);
    require(!pipeline::projectProcessingRegion({0,0,-0.1f}, {1,1,0.2f}, identity, extent));
    require(!pipeline::projectProcessingRegion({0,0,0.9f}, {1,1,1.1f}, identity, extent));
    require(!pipeline::projectProcessingRegion({1,0,0.4f}, {-1,1,0.6f}, identity, extent));
    auto invalid = identity;
    invalid[0][0] = std::numeric_limits<float>::quiet_NaN();
    require(!pipeline::projectProcessingRegion({0,0,0.4f}, {1,1,0.6f}, invalid, extent));
    auto behindEye = identity;
    behindEye[3][3] = -1;
    require(!pipeline::projectProcessingRegion({0,0,0.4f}, {1,1,0.6f}, behindEye, extent));
    require(!pipeline::projectProcessingRegion({0,0,0.4f}, {1,1,0.6f}, identity, {0,0}));
}
}
