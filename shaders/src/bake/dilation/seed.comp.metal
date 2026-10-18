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
#define WRITE0(v) output0.write(v,pixel)
bool finite3(float3 v)
{
    return all(isfinite(v));
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant uint4& parameters [[buffer(1)]],
    sampler nearestSampler [[sampler(0)]],
    texture2d<uint,access::read> input0 [[texture(0)]],
    texture2d<uint,access::write> output0 [[texture(1)]])
{
    int2 p = int2(pixel);
    int width = int(output0.get_width()), height = int(output0.get_height());
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
