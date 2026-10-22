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
layout(set=0,binding=2) uniform texture2D surfaceImage;
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
    vec4 value = texelFetch(sampler2D(surfaceImage,nearestSampler),ivec2(pixel),0);
    vec3 color = vec3(0);
    if (token != 0u)
    {
        if (dataBlock.options.z == 3u)
        {
            if (!any(isnan(value.xy)) && !any(isinf(value.xy)))
            {
                color = vec3(clamp(vec2(0.5)+value.xy/64.0,vec2(0),vec2(1)),0.5);
            }
        }
        else if (value.w > 0.5 && !any(isnan(value)) && !any(isinf(value)))
        {
            color = dataBlock.options.z == 4u ?
                0.5+0.5*value.xyz/(vec3(1)+abs(value.xyz)) :
                clamp(0.5+0.5*value.xyz,vec3(0),vec3(1));
        }
    }
    imageStore(outputImage,ivec2(pixel),vec4(color,1));
}
