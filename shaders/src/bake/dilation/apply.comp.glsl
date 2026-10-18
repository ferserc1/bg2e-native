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
layout(set=0,binding=0,std140) uniform Parameters { uvec4 parameters; };
layout(set=0,binding=1) uniform sampler nearestSampler;
layout(set=0,binding=2) uniform texture2D input0;
#define READ0(p) texelFetch(sampler2D(input0,nearestSampler),p,0)
layout(set=0,binding=3) uniform utexture2D input1;
#define READ1(p) texelFetch(usampler2D(input1,nearestSampler),p,0)
layout(set=0,binding=4) uniform utexture2D input2;
#define READ2(p) texelFetch(usampler2D(input2,nearestSampler),p,0)
layout(set=0,binding=5,rgba32f) uniform writeonly image2D output0;
#define WRITE0(v) imageStore(output0,p,v)
bool finite3(vec3 v)
{
    return !any(isnan(v)) && !any(isinf(v));
}
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
void main()
{
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    int width = imageSize(output0).x, height = imageSize(output0).y;
    if (p.x >= width || p.y >= height)
    {
        return;
    }
    vec3 result = READ0(p).rgb;
    if (READ1(p).r == 0u)
    {
        result = vec3(1.0);
        int radius = int(parameters.z);
        int bestDistance = radius*radius+1;
        uint bestChart = 0xffffffffu, bestIndex = 0xffffffffu;
        int first = max(0,p.y-radius), last = min(height-1,p.y+radius);
        for (int y = first; y <= last; ++y)
        {
            uvec2 seed = READ2(ivec2(p.x,y)).rg;
            if (seed.x == 0xffffffffu)
            {
                continue;
            }
            ivec2 q = ivec2(seed);
            ivec2 delta = q-p;
            int distance = delta.x*delta.x+delta.y*delta.y;
            uint chart = READ1(q).r;
            uint linearIndex = seed.y*uint(width)+seed.x;
            if (distance <= radius*radius && (distance < bestDistance ||
                (distance == bestDistance && (chart < bestChart || (chart == bestChart && linearIndex < bestIndex)))))
            {
                bestDistance = distance;
                bestChart = chart;
                bestIndex = linearIndex;
                result = READ0(q).rgb;
            }
        }
    }
    WRITE0(vec4(result,1.0));
}
