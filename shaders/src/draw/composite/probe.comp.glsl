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

#version 450
#extension GL_GOOGLE_include_directive : require
#include "lib/color.glsl"
layout(local_size_x = 16, local_size_y = 16) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D outputImage;
void main()
{
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(position, imageSize(outputImage))))
    {
        return;
    }
    imageStore(outputImage, position, vec4(drawColorCorrection(vec3(0.5), 1.0, 2.2, 0.0, 1.0), 1.0));
}
