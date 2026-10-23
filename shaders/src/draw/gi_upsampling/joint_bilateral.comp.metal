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

struct GIUpsamplingUniforms {
    float4x4 inverseViewProjection;
    float4x4 worldToView;
    uint4 extent;
    float4 thresholds;
    float4 parameters;
};

// Bit checks remain valid with backend fast-math compilation settings.
bool finiteValue(float value)
{
    return (as_type<uint>(value) & 0x7f800000u) != 0x7f800000u;
}

bool finiteVector(float3 value)
{
    return finiteValue(value.x) && finiteValue(value.y) && finiteValue(value.z);
}

bool receiver(int2 pixel, constant GIUpsamplingUniforms& dataBlock, texture2d<float> normalImage, depth2d<float> depthImage, thread float3& normal, thread float& viewDepth)
{
    float3 encoded = normalImage.read(uint2(pixel)).rgb;
    float depth = depthImage.read(uint2(pixel));
    if (!finiteVector(encoded) || dot(encoded,encoded) <= 1e-8 ||
        !finiteValue(depth) || depth < 0.0 || depth >= 1.0)
    {
        return false;
    }
    float3 decoded = encoded*2.0-1.0;
    float normalLength = dot(decoded,decoded);
    if (!finiteVector(decoded) || !finiteValue(normalLength) || normalLength <= 0.0)
    {
        return false;
    }
    normal = decoded/sqrt(normalLength);
    float2 uv = (float2(pixel)+0.5)/float2(dataBlock.extent.xy);
    float4 world = dataBlock.inverseViewProjection*float4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    if (!finiteValue(world.w) || abs(world.w) <= 1e-8)
    {
        return false;
    }
    float3 position = world.xyz/world.w;
    viewDepth = (dataBlock.worldToView*float4(position,1.0)).z;
    return finiteVector(normal) && finiteVector(position) && finiteValue(viewDepth);
}

kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant GIUpsamplingUniforms& dataBlock [[buffer(1)]],
    texture2d<float> signalImage [[texture(0)]],
    texture2d<float> normalImage [[texture(1)]],
    depth2d<float> depthImage [[texture(2)]],
    texture2d<float,access::write> outputImage [[texture(3)]])
{
    if (any(pixel >= dataBlock.extent.xy))
    {
        return;
    }
    int2 p = int2(pixel);
    float3 centerNormal;
    float centerDepth;
    float4 result = float4(0.0);
    if (!receiver(p,dataBlock,normalImage,depthImage,centerNormal,centerDepth))
    {
        outputImage.write(result,pixel);
        return;
    }
    float2 source = (float2(p)+0.5)/float2(dataBlock.extent.xy)*float2(dataBlock.extent.zw)-0.5;
    int2 base = int2(floor(source));
    float3 sum = float3(0.0);
    float weightSum = 0.0;
    float nearestDistance = 0.0;
    float3 nearestValue = float3(0.0);
    bool found = false;
    for (int j = -1; j <= 2; ++j)
    {
        for (int i = -1; i <= 2; ++i)
        {
            int2 q = base+int2(i,j);
            if (any(q < int2(0)) || any(q >= int2(dataBlock.extent.zw)))
            {
                continue;
            }
            float4 value = signalImage.read(uint2(q));
            if (!finiteVector(value.rgb) || !finiteValue(value.a) || value.a <= 0.5)
            {
                continue;
            }
            // Preserve the RTGI producer's exact receiver association and operation order.
            int2 h = min(int2((float2(q)+0.5)/float2(dataBlock.extent.zw)*float2(dataBlock.extent.xy)),int2(dataBlock.extent.xy)-1);
            float3 normal;
            float depth;
            if (!receiver(h,dataBlock,normalImage,depthImage,normal,depth))
            {
                continue;
            }
            float dz = abs(depth-centerDepth);
            float cosine = clamp(dot(normal,centerNormal),-1.0,1.0);
            float tolerance = max(dataBlock.thresholds.x,dataBlock.thresholds.y*max(abs(depth),abs(centerDepth)));
            if (!finiteValue(dz) || !finiteValue(cosine) || dz > tolerance || cosine < dataBlock.thresholds.z)
            {
                continue;
            }
            float2 delta = (float2(h)-float2(p))*float2(dataBlock.extent.zw)/float2(dataBlock.extent.xy);
            float distanceSquared = dot(delta,delta);
            float3 radiance = max(value.rgb,float3(0.0));
            // Stable iteration order breaks ties, and fallback never relaxes geometry gates.
            if (!found || distanceSquared < nearestDistance)
            {
                nearestDistance = distanceSquared;
                nearestValue = radiance;
                found = true;
            }
            float depthTerm = dz/(dataBlock.parameters.x*tolerance);
            float normalTerm = (1.0-cosine)/dataBlock.parameters.y;
            float spatialTerm = distanceSquared/(dataBlock.thresholds.w*dataBlock.thresholds.w);
            float weight = exp(-0.5*(spatialTerm+depthTerm*depthTerm+normalTerm*normalTerm));
            if (finiteValue(weight))
            {
                sum += radiance*weight;
                weightSum += weight;
            }
        }
    }
    if (found)
    {
        float3 radiance = nearestValue;
        if (finiteValue(weightSum) && weightSum > dataBlock.parameters.z && finiteVector(sum))
        {
            float3 filtered = sum/weightSum;
            if (finiteVector(filtered))
            {
                radiance = filtered;
            }
        }
        result = float4(clamp(radiance,float3(0.0),float3(65504.0)),1.0);
    }
    outputImage.write(result,pixel);
}
