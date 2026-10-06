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

layout(set = 0, binding = 0) uniform sampler2D rtaoVisibility;
layout(set = 0, binding = 1) uniform sampler2D normalizedRTGI;
layout(set = 0, binding = 2) uniform sampler2D directShadowVisibility;
layout(set = 0, binding = 3) uniform usampler2D validTexelMask;
layout(set = 0, binding = 4, rgba16f) uniform image2D lightmapOutput;

layout(push_constant) uniform PushConstant {
    uint mode;       // 0 = RTAO, 1 = RTGI
    uint useShadows;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 extent = imageSize(lightmapOutput);
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }

    if (texelFetch(validTexelMask, pixel, 0).r == 0u)
    {
        imageStore(lightmapOutput, pixel, vec4(1.0));
        return;
    }

    vec3 lightMultiplier = pc.mode == 0u
        ? vec3(texelFetch(rtaoVisibility, pixel, 0).r)
        : texelFetch(normalizedRTGI, pixel, 0).rgb;

    // The direct shadow factor is an independent term and is applied once to
    // either indirect mode. The result remains a linear RGB light multiplier.
    if (pc.useShadows != 0u)
    {
        lightMultiplier *= texelFetch(directShadowVisibility, pixel, 0).r;
    }

    imageStore(lightmapOutput, pixel, vec4(lightMultiplier, 1.0));
}
