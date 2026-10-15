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

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputImage;
layout(set = 0, binding = 1, rgba8) uniform image2D outputImage;

float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (pos.x >= size.x || pos.y >= size.y)
    {
        return;
    }

    // Sample 3x3 neighborhood luminances
    float tl = luma(imageLoad(inputImage, pos + ivec2(-1, -1)).rgb);
    float t = luma(imageLoad(inputImage, pos + ivec2(0, -1)).rgb);
    float tr = luma(imageLoad(inputImage, pos + ivec2(1, -1)).rgb);
    float l = luma(imageLoad(inputImage, pos + ivec2(-1, 0)).rgb);
    float r = luma(imageLoad(inputImage, pos + ivec2(1, 0)).rgb);
    float bl = luma(imageLoad(inputImage, pos + ivec2(-1, 1)).rgb);
    float b = luma(imageLoad(inputImage, pos + ivec2(0, 1)).rgb);
    float br = luma(imageLoad(inputImage, pos + ivec2(1, 1)).rgb);

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0, 1.0);

    imageStore(outputImage, pos, vec4(vec3(edge), 1.0));
}
