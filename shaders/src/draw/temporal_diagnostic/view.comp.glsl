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
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
layout(set=0,binding=0,std140) uniform DiagnosticUniforms { uvec4 options; } dataBlock;
layout(set=0,binding=1) uniform utexture2D tokenImage;
layout(set=0,binding=2) uniform texture2D acceptanceImage;
layout(set=0,binding=3) uniform sampler nearestSampler;
layout(set=0,binding=4,rgba16f) uniform image2D outputImage;
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.options.xy)))
    {
        return;
    }
    uint token = texelFetch(usampler2D(tokenImage,nearestSampler),ivec2(pixel),0).r;
    vec3 color;
    if (dataBlock.options.z == 1u)
    {
        uint hash = token*1664525u+1013904223u;
        hash ^= hash >> 16;
        color = token == 0u ? vec3(0) : vec3(hash & 255u,(hash >> 8) & 255u,(hash >> 16) & 255u)/255.0;
    }
    else if (dataBlock.options.w == 0u)
    {
        color = vec3(1,0.5,0);
    }
    else
    {
        vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.options.xy);
        ivec2 size = textureSize(sampler2D(acceptanceImage,nearestSampler),0);
        ivec2 source = min(ivec2(uv*vec2(size)),size-1);
        color = texelFetch(sampler2D(acceptanceImage,nearestSampler),source,0).rgb;
    }
    imageStore(outputImage,ivec2(pixel),vec4(color,1));
}
