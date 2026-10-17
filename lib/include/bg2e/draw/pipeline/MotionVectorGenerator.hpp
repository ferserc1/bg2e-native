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
#include <bg2e/math/base.hpp>
#include <bg2e/gpu/Common.hpp>

namespace bg2e::draw::pipeline {
struct TemporalView {
    glm::mat4 view{1}, projection{1}, jitteredProjection{1};
    bool reproject = true; // false: stationary explicit-surface correspondence at the same UV.
};
struct MotionVectorTransforms {
    glm::mat4 currentFromJittered{1}, previousFromJittered{1};
};
// Camera-only motion reference, shared coordinate convention with the GPU
// temporal projection and FSR. Moving geometry requires history invalidation.
class BG2E_API MotionVectorGenerator {
  public:
    static void validate(const TemporalView& view);
    static MotionVectorTransforms camera(const TemporalView& current, const TemporalView& previous);
    // Previous minus current unjittered framebuffer pixels; Y opposes clip Y.
    static glm::vec2 motionPixels(const MotionVectorTransforms& transforms,
        const glm::vec4& currentJitteredClip, gpu::Size2D extent);
};
}
