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

#include <bg2e/draw/pipeline/MotionVectorGenerator.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::draw::pipeline {
void MotionVectorGenerator::validate(const TemporalView& view)
{
    for (const auto& matrix : {view.view,view.projection,view.jitteredProjection})
    {
        for (uint32_t c = 0; c < 4; ++c)
        {
            for (uint32_t r = 0; r < 4; ++r)
            {
                if (!std::isfinite(matrix[c][r]))
                {
                    throw std::invalid_argument("Temporal matrices must be finite");
                }
            }
        }
        const float determinant = glm::determinant(matrix);
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f)
        {
            throw std::invalid_argument("Temporal matrices must be invertible");
        }
    }
}

MotionVectorTransforms MotionVectorGenerator::camera(const TemporalView& current, const TemporalView& previous)
{
    validate(current);
    validate(previous);
    const auto inverse = glm::inverse(current.jitteredProjection*current.view);
    return {current.projection*current.view*inverse,previous.projection*previous.view*inverse};
}

glm::vec2 MotionVectorGenerator::motionPixels(const MotionVectorTransforms& transforms,
    const glm::vec4& clip, gpu::Size2D extent)
{
    if (extent.isZero())
    {
        throw std::invalid_argument("Motion output extent must be nonzero");
    }
    const auto current = transforms.currentFromJittered*clip, previous = transforms.previousFromJittered*clip;
    if (std::abs(current.w) <= 1e-6f || previous.w <= 1e-6f)
    {
        return glm::vec2(0);
    }
    const auto motion = (glm::vec2(previous)/previous.w-glm::vec2(current)/current.w)*
        glm::vec2(0.5f,-0.5f)*glm::vec2(extent.width,extent.height);
    return std::isfinite(motion.x) && std::isfinite(motion.y) ? motion : glm::vec2(0);
}
}
