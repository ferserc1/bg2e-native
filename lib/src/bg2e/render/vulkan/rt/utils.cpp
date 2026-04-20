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

#include <bg2e/render/vulkan/rt/utils.hpp>

namespace bg2e::render::vulkan::rt
{

VkTransformMatrixKHR mat4ToVkTransformMatrix(const glm::mat4 &m)
{
    VkTransformMatrixKHR out{};

    out.matrix[0][0] = m[0][0];
    out.matrix[0][1] = m[1][0];
    out.matrix[0][2] = m[2][0];
    out.matrix[0][3] = m[3][0];

    out.matrix[1][0] = m[0][1];
    out.matrix[1][1] = m[1][1];
    out.matrix[1][2] = m[2][1];
    out.matrix[1][3] = m[3][1];

    out.matrix[2][0] = m[0][2];
    out.matrix[2][1] = m[1][2];
    out.matrix[2][2] = m[2][2];
    out.matrix[2][3] = m[3][2];

    return out;
}

}