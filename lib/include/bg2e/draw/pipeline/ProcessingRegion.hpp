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

#include <bg2e/gpu/Common.hpp>
#include <bg2e/math/base.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

namespace bg2e::draw::pipeline {
// Null means full extent; an empty rectangle means proven offscreen. This is
// only valid for rigid geometry with the standard, undeformed G-buffer shader.
inline std::optional<gpu::ScissorRegion> projectProcessingRegion(const glm::vec3& minimum,
    const glm::vec3& maximum, const glm::mat4& clipFromLocal, gpu::Size2D extent)
{
    if (extent.isZero())
    {
        return std::nullopt;
    }
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!std::isfinite(minimum[axis]) || !std::isfinite(maximum[axis]) || minimum[axis] > maximum[axis])
        {
            return std::nullopt;
        }
    }
    double left = 1e30, top = 1e30, right = -1e30, bottom = -1e30;
    for (int corner = 0; corner < 8; ++corner)
    {
        const glm::vec3 position{corner & 1 ? maximum.x : minimum.x,
            corner & 2 ? maximum.y : minimum.y, corner & 4 ? maximum.z : minimum.z};
        const auto clip = clipFromLocal * glm::vec4(position, 1);
        for (int component = 0; component < 4; ++component)
        {
            if (!std::isfinite(clip[component]))
            {
                return std::nullopt;
            }
        }
        // Do not divide bounds crossing either depth plane or the eye plane.
        if (clip.w <= 1e-5f || clip.z <= 1e-5f || clip.z >= clip.w-1e-5f)
        {
            return std::nullopt;
        }
        const double x = (double(clip.x)/clip.w*0.5+0.5)*extent.width;
        const double y = (0.5-double(clip.y)/clip.w*0.5)*extent.height;
        left = std::min(left, x);
        right = std::max(right, x);
        top = std::min(top, y);
        bottom = std::max(bottom, y);
    }
    // Two pixels cover raster edge precision and nearest G-buffer sampling.
    // Temporal, denoise and scaled receiver kernels retain their full domain.
    left = std::clamp(std::floor(left)-2, 0.0, double(extent.width));
    top = std::clamp(std::floor(top)-2, 0.0, double(extent.height));
    right = std::clamp(std::ceil(right)+2, 0.0, double(extent.width));
    bottom = std::clamp(std::ceil(bottom)+2, 0.0, double(extent.height));
    if (left == 0 && top == 0 && right == extent.width && bottom == extent.height)
    {
        return std::nullopt;
    }
    return gpu::ScissorRegion{uint32_t(left), uint32_t(top), uint32_t(right-left), uint32_t(bottom-top)};
}
}
