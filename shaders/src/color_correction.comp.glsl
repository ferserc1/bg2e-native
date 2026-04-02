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
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler2D inputImage;
layout (rgba16f, set = 0, binding = 1) uniform image2D outputImage;

void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    vec2 inTexCoord = vec2(gl_GlobalInvocationID.x / size.x, gl_GlobalInvocationID.y);
    
    float gamma = 2.2; // Gamma value for conversion
    vec3 sampleColor = lineal2SRGB(texture(inputImage, inTexCoord).rgb, gamma);
    imageStore(outputImage, texelCoord, vec4(sampleColor, 1.0));
    
}


