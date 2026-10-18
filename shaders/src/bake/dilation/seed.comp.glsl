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
layout(set=0,binding=2) uniform utexture2D input0;
#define READ0(p) texelFetch(usampler2D(input0,nearestSampler),p,0)
layout(set=0,binding=3,rg32ui) uniform writeonly uimage2D output0;
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
    uint bestChart = 0xffffffffu;
    int bestDistance = 0x7fffffff;
    uvec2 best = uvec2(0xffffffffu);
    int radius = int(parameters.z);
    int first = max(0,p.x-radius), last = min(width-1,p.x+radius);
    for (int x = first; x <= last; ++x)
    {
        uint chart = READ0(ivec2(x,p.y)).r;
        int distance = (x-p.x)*(x-p.x);
        if (chart != 0u && (distance < bestDistance ||
            (distance == bestDistance && (chart < bestChart || (chart == bestChart && uint(x) < best.x)))))
        {
            best = uvec2(uint(x),uint(p.y));
            bestChart = chart;
            bestDistance = distance;
        }
    }
    WRITE0(uvec4(best,0u,0u));
}
