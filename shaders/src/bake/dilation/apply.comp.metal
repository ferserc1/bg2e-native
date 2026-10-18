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

#include <metal_stdlib>
using namespace metal;
#define vec3 float3
#define vec4 float4
#define uvec2 uint2
#define uvec4 uint4
#define ivec2 int2
#define NAN_VALUE as_type<float>(0x7fc00000u)
#define READ0(p) input0.read(uint2(p))
#define READ1(p) input1.read(uint2(p))
#define READ2(p) input2.read(uint2(p))
#define WRITE0(v) output0.write(v,pixel)
bool finite3(float3 v)
{
    return all(isfinite(v));
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant uint4& parameters [[buffer(1)]],
    sampler nearestSampler [[sampler(0)]],
    texture2d<float,access::read> input0 [[texture(0)]],
    texture2d<uint,access::read> input1 [[texture(1)]],
    texture2d<uint,access::read> input2 [[texture(2)]],
    texture2d<float,access::write> output0 [[texture(3)]])
{
    int2 p = int2(pixel);
    int width = int(output0.get_width()), height = int(output0.get_height());
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
