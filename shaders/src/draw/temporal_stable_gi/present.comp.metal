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

#include <metal_stdlib>
using namespace metal;
struct PresentUniforms {
    float4x4 inverseViewProjection, stableViewProjection;
    uint4 extent;
    float4 rejection, options;
};
static_assert(sizeof(PresentUniforms) == 176, "Stable GI presentation uniforms");
kernel void compMain(uint2 pixel [[thread_position_in_grid]], constant PresentUniforms& dataBlock [[buffer(1)]],
    texture2d<float> stableSignal [[texture(0)]], texture2d<float> stablePosition [[texture(1)]],
    texture2d<float> stableNormal [[texture(2)]], texture2d<float> normalImage [[texture(3)]],
    depth2d<float> depthImage [[texture(4)]], texture2d<float> currentSignal [[texture(5)]],
    sampler nearestSampler [[sampler(0)]], texture2d<float,access::write> outputImage [[texture(6)]])
{
    if (any(pixel >= dataBlock.extent.zw))
    {
        return;
    }
    float2 uv = (float2(pixel)+0.5)/float2(dataBlock.extent.zw);
    int2 receiver = min(int2(uv*float2(dataBlock.extent.xy)),int2(dataBlock.extent.xy)-1);
    float2 receiverUV = (float2(receiver)+0.5)/float2(dataBlock.extent.xy);
    float depth = float4(depthImage.read(uint2(receiver))).r;
    float3 encoded = normalImage.read(uint2(receiver)).xyz;
    float3 decoded = encoded*2.0-1.0;
    float4 world = dataBlock.inverseViewProjection*float4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,depth,1.0);
    bool covered = all(isfinite(world)) && all(isfinite(float4(decoded,depth))) &&
        world.w > 1e-8 && depth >= 0.0 && depth < 1.0 && dot(encoded,encoded) > 1e-8 && dot(decoded,decoded) > 1e-8;
    float4 result = float4(0);
    if (covered)
    {
        float3 position = world.xyz/world.w;
        float3 normal = normalize(decoded);
        float4 clip = dataBlock.stableViewProjection*float4(position,1);
        if (!all(isfinite(clip)) || clip.w <= 1e-8)
        {
            outputImage.write(float4(0),pixel);
            return;
        }
        float2 stableUV = uv+(clip.xy/clip.w*float2(0.5,-0.5)+0.5)-receiverUV;
        float2 q = stableUV*float2(dataBlock.extent.zw)-0.5;
        int2 base = int2(floor(q));
        float2 fraction = q-float2(base);
        bool nearest = dataBlock.rejection.w > 0.5;
        if (nearest)
        {
            base = int2(floor(q+0.5));
        }
        float tolerance = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.options.xyz);
        float4 sum = float4(0);
        float validWeight = 0.0;
        for (int tap = 0; tap < 4; ++tap)
        {
            if (nearest && tap != 0)
            {
                break;
            }
            int2 offset = int2(tap & 1,tap >> 1);
            int2 p = base+offset;
            float weight = nearest ? 1.0 : (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
                (offset.y == 0 ? 1.0-fraction.y : fraction.y);
            if (weight <= 0.0 || any(p < int2(0)) || any(p >= int2(dataBlock.extent.zw)))
            {
                continue;
            }
            float4 point = stablePosition.read(uint2(p));
            float4 direction = stableNormal.read(uint2(p));
            float4 value = stableSignal.read(uint2(p));
            if (!all(isfinite(point)) || !all(isfinite(direction)) || !all(isfinite(value)) ||
                point.w <= 0.5 || direction.w < 1.0 ||
                length(point.xyz-position) > tolerance || dot(direction.xyz,normal) < dataBlock.rejection.z ||
                (dataBlock.options.w < 0.5 && value.a <= 0.5))
            {
                continue;
            }
            sum += value*weight;
            validWeight += weight;
        }
        if (validWeight > 1e-6)
        {
            result = sum/validWeight;
        }
        else
        {
            // A presentation hole may use this frame's raw GI, never its history.
            result = dataBlock.options.w < 0.5 ? currentSignal.read(pixel) : float4(0.25);
        }
    }
    if (!all(isfinite(result)))
    {
        result = float4(0);
    }
    outputImage.write(result,pixel);
}
