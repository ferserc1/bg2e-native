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
struct DebugPush {
    float4x4 inverseProjection;
    float4 display;
    float4 rangeMode;
};
fragment float4 fragMain(float4 position [[position]], depth2d<float> sourceImage [[texture(0)]],
    sampler sourceSampler [[sampler(0)]], constant DebugPush& pc [[buffer(0)]])
{
    float2 uv = (position.xy/pc.display.xy-0.5)/pc.display.w+0.5;
    int2 size = int2(sourceImage.get_width(),sourceImage.get_height());
    uint2 pixel = uint2(clamp(int2(uv*float2(size)),int2(0),size-1));
    float4 value = float4(sourceImage.read(pixel));
    uint mode = uint(pc.rangeMode.z);
    if (mode == 1u)
    {
        value.rgb = float3(value.r);
    }
    if (mode == 2u)
    {
        value.rgb = float3(value.g);
    }
    if (mode == 3u)
    {
        value.rgb = float3(value.b);
    }
    if (mode == 4u)
    {
        value.rgb = float3(value.a);
    }
    if (mode == 5u)
    {
        float2 center = (float2(pixel)+0.5)/float2(size);
        float4 view = pc.inverseProjection*float4(center.x*2.0-1.0,1.0-center.y*2.0,value.r,1);
        float depth = abs(view.z/max(abs(view.w),1e-8));
        value.rgb = float3(clamp((depth-pc.rangeMode.x)/(pc.rangeMode.y-pc.rangeMode.x),0.0,1.0));
    }
    if (pc.rangeMode.w >= 0.0)
    {
        value.rgb = clamp(value.rgb*exp2(pc.display.z),float3(0),float3(1));
    }
    if (pc.rangeMode.w > 0.5)
    {
        float3 lo = value.rgb/12.92;
        float3 hi = pow((value.rgb+0.055)/1.055,float3(2.4));
        value.rgb = select(hi,lo,value.rgb <= float3(0.04045));
    }
    return pc.rangeMode.w < 0.0 ? value : float4(value.rgb,1);
}
