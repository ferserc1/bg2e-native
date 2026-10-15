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

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform image2D outImage;

void main()
{
    ivec2 size = imageSize(outImage);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= size.x || pos.y >= size.y)
    {
        return;
    }
    vec2 uv = vec2(float(pos.x) / float(size.x), float(pos.y) / float(size.y));
    imageStore(outImage, pos, vec4(uv.x, uv.y, 0.5, 1.0));
}
