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
#define READ3(p) input3.read(uint2(p))
#define READ4(p) input4.read(uint2(p))
#define READ5(p) input5.read(uint2(p))
#define WRITE0(v) output0.write(v,pixel)
#define WRITE1(v) output1.write(v,pixel)
bool finite3(float3 v)
{
    return all(isfinite(v));
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant uint4& parameters [[buffer(1)]],
    sampler nearestSampler [[sampler(0)]],
    texture2d<float,access::read> input0 [[texture(0)]],
    texture2d<float,access::read> input1 [[texture(1)]],
    texture2d<uint,access::read> input2 [[texture(2)]],
    texture2d<float,access::read> input3 [[texture(3)]],
    texture2d<uint,access::read> input4 [[texture(4)]],
    texture2d<uint,access::read> input5 [[texture(5)]],
    texture2d<float,access::write> output0 [[texture(6)]],
    texture2d<uint,access::write> output1 [[texture(7)]])
{
    int2 p = int2(pixel);
    int width = int(output0.get_width()), height = int(output0.get_height());
    if (p.x >= width || p.y >= height)
    {
        return;
    }
    vec4 oldSum = parameters.y != 0u ? READ1(p) : vec4(0.0);
    uint oldCount = parameters.y != 0u ? READ2(p).r : 0u;
    uint chart = READ4(p).r;
    bool covered = READ3(p).r > 0.5;
    float error = oldSum.a;
    if (covered != (chart != 0u) || (parameters.y != 0u && READ5(p).r != chart))
    {
        error = 1.0;
    }
    vec3 sum = oldSum.rgb;
    uint count = oldCount;
    if (covered)
    {
        vec4 raw = READ0(p);
        bool valid = finite3(raw.rgb);
        vec3 value = parameters.x == 0u ? vec3(raw.r) : raw.rgb;
        if (parameters.x == 0u)
        {
            valid = valid && raw.r >= 0.0 && raw.r <= 1.0;
        }
        else
        {
            // Public GI uses a half-float target and saturates at 65504.
            // Conservatively reject the saturation boundary, never accept clipped HDR.
            valid = valid && raw.a == 1.0 && min(value.x,min(value.y,value.z)) >= 0.0 &&
                max(value.x,max(value.y,value.z)) < 65504.0;
        }
        if (valid)
        {
            sum += value;
            count += 1u;
        }
        else
        {
            error = 1.0;
        }
        if (!finite3(sum))
        {
            error = 1.0;
        }
    }
    else
    {
        if (oldCount != 0u)
        {
            error = 1.0;
        }
        sum = vec3(0.0);
        count = 0u;
    }
    WRITE0(vec4(sum,error));
    WRITE1(uvec4(count,0u,0u,0u));
}
