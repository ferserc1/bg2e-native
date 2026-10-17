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

#version 460
#extension GL_GOOGLE_include_directive : require
#include "../lib/blue_noise.glsl"
layout(local_size_x=16, local_size_y=16) in;
layout(set=0,binding=0,std140) uniform NoiseProbeData { uvec4 metadata; uvec4 sequence; } dataBlock;
layout(set=0,binding=1,rgba8) uniform image2D outputImage;
void main()
{
    ivec2 outputPixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(outputPixel, imageSize(outputImage))))
    {
        return;
    }
    ivec2 pixel = outputPixel - ivec2(3,2);
    uint frame = dataBlock.sequence.x + uint(outputPixel.y) / dataBlock.metadata.y;
    vec2 value = bg2eHashNoise2(pixel, frame, dataBlock.sequence.y);
    imageStore(outputImage, outputPixel, vec4(value,0,1));
}
