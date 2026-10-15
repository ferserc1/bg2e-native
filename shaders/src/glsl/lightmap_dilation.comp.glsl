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

#version 460

layout(set = 0, binding = 0) uniform sampler2D sourceImage;
layout(set = 0, binding = 1, rgba32f) uniform image2D outputImage;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 extent = imageSize(outputImage);
    if (any(greaterThanEqual(pixel, extent)))
    {
        return;
    }

    vec4 current = texelFetch(sourceImage, pixel, 0);
    if (current.a > 0.5)
    {
        imageStore(outputImage, pixel, current);
        return;
    }

    // Each pass grows covered texels by one pixel. Read only the previous
    // image, so workgroup execution order cannot change the result.
    vec4 selected = current;
    int bestDistance = 3;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 neighbor = pixel + ivec2(x, y);
            if (any(lessThan(neighbor, ivec2(0))) ||
                any(greaterThanEqual(neighbor, extent)))
            {
                continue;
            }

            vec4 candidate = texelFetch(sourceImage, neighbor, 0);
            int distance = abs(x) + abs(y);
            if (candidate.a > 0.5 && distance < bestDistance)
            {
                selected = candidate;
                bestDistance = distance;
            }
        }
    }
    imageStore(outputImage, pixel, selected);
}
