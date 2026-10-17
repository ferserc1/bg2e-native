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
#if DENOISE_WORLD
#define DENOISE_POSITION texture2d<float>
#else
#define DENOISE_POSITION depth2d<float>
#endif
struct DenoiseUniforms {
    float4x4 inverseViewProjection;
    float4x4 worldToView;
    float4 thresholds;
    float4 parameters;
    uint4 extent;
    uint4 counts;
};
bool denoiseFinite(float value)
{
    return (as_type<uint>(value) & 0x7f800000u) != 0x7f800000u;
}
bool denoiseFinite3(float3 value)
{
    return !any((as_type<uint3>(value) & uint3(0x7f800000u)) == uint3(0x7f800000u));
}
int2 denoiseSource(int2 pixel, constant DenoiseUniforms& dataBlock)
{
    float2 uv = (float2(pixel)+0.5)/float2(dataBlock.extent.zw);
    return min(int2(uv*float2(dataBlock.extent.xy)),int2(dataBlock.extent.xy)-1);
}
bool denoiseSurface(int2 pixel, constant DenoiseUniforms& dataBlock, texture2d<float> normalImage, DENOISE_POSITION positionImage, texture2d<float> coverageImage, thread float3& position, thread float3& normal)
{
    int2 source = denoiseSource(pixel,dataBlock);
    float3 encoded = normalImage.read(uint2(source)).xyz;
    bool covered = dot(encoded,encoded) > 1e-8;
#if DENOISE_WORLD
    covered = covered && coverageImage.read(uint2(source)).r > 0.0;
    position = positionImage.read(uint2(source)).xyz;
    normal = normalize(encoded);
#else
    float depth = positionImage.read(uint2(source));
    covered = covered && depth < 1.0;
    float2 uv = (float2(source)+0.5)/float2(dataBlock.extent.xy);
    float4 world = dataBlock.inverseViewProjection*float4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
    covered = covered && denoiseFinite((dataBlock.worldToView*float4(position,1)).z);
#endif
    return covered && denoiseFinite3(position) && denoiseFinite3(normal) && dot(normal,normal) > 1e-8;
}
bool denoiseSignal(int2 pixel, constant DenoiseUniforms& dataBlock, texture2d<float> signalImage, texture2d<float> materialImage, thread float4& value)
{
    value = signalImage.read(uint2(pixel));
#if DENOISE_SCALAR
    bool valid = denoiseFinite(value.r);
    value = float4(clamp(value.r,0.0,1.0));
    return valid;
#else
    if (!denoiseFinite3(value.rgb) || !denoiseFinite(value.a))
    {
        return false;
    }
    value.rgb = max(value.rgb,float3(0));
    if (dataBlock.counts.x == 1u)
    {
        bool valid = value.a > 0.5;
        value.a = 1.0;
        return valid;
    }
    int2 source = denoiseSource(pixel,dataBlock);
    float roughness = materialImage.read(uint2(source)).g;
    value.a = clamp(value.a,0.0,1.0);
    return denoiseFinite(roughness) && roughness <= dataBlock.parameters.y;
#endif
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant DenoiseUniforms& dataBlock [[buffer(1)]],
    texture2d<float> normalImage [[texture(0)]],
    DENOISE_POSITION positionImage [[texture(1)]],
    texture2d<float> coverageImage [[texture(2)]],
    texture2d<float> signalImage [[texture(3)]],
    texture2d<float> materialImage [[texture(4)]],
    texture2d<float,access::write> outputImage [[texture(5)]])
{
    if (any(pixel >= dataBlock.extent.zw))
    {
        return;
    }
    float3 centerPosition, centerNormal;
    float4 center;
    if (!denoiseSurface(int2(pixel),dataBlock,normalImage,positionImage,coverageImage,centerPosition,centerNormal) || !denoiseSignal(int2(pixel),dataBlock,signalImage,materialImage,center))
    {
#if DENOISE_SCALAR
        outputImage.write(float4(1),pixel);
#else
        outputImage.write(float4(0),pixel);
#endif
        return;
    }
    float4 sum = float4(0);
    float weightSum = 0.0, confidenceSum = 0.0;
    int radius = int(dataBlock.counts.y);
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            int2 neighbor = int2(pixel)+int2(x,y);
            if (any(neighbor < int2(0)) || any(neighbor >= int2(dataBlock.extent.zw)))
            {
                continue;
            }
            float3 position, normal;
            float4 value;
            if (!denoiseSurface(neighbor,dataBlock,normalImage,positionImage,coverageImage,position,normal) || !denoiseSignal(neighbor,dataBlock,signalImage,materialImage,value))
            {
                continue;
            }
            float cosine = clamp(dot(centerNormal,normal),-1.0,1.0);
#if DENOISE_WORLD
            // Explicit UV surfaces reject distant world points, including coplanar chart seams.
            float depthDifference = length(position-centerPosition);
#else
            float depthDifference = abs((dataBlock.worldToView*float4(position-centerPosition,0)).z);
#endif
            if (!denoiseFinite(depthDifference) || depthDifference > dataBlock.thresholds.x || cosine < dataBlock.thresholds.y)
            {
                continue;
            }
            float depthTerm = depthDifference/dataBlock.thresholds.z;
            float normalTerm = (1.0-cosine)/dataBlock.thresholds.w;
            float spatialTerm = float(x*x+y*y)/(dataBlock.parameters.x*dataBlock.parameters.x);
            float weight = exp(-0.5*(spatialTerm+depthTerm*depthTerm+normalTerm*normalTerm));
            weightSum += weight;
#if DENOISE_SCALAR
            sum.r += value.r*weight;
#else
            if (dataBlock.counts.x == 2u)
            {
                sum.rgb += value.rgb*(value.a*weight);
                confidenceSum += value.a*weight;
            }
            else
            {
                sum.rgb += value.rgb*weight;
            }
#endif
        }
    }
    // The eligible center has weight one; retain a safe fallback for custom profiles.
    float4 result = center;
    if (weightSum > 1e-8)
    {
#if DENOISE_SCALAR
        result = float4(clamp(sum.r/weightSum,0.0,1.0));
#else
        if (dataBlock.counts.x == 2u)
        {
            result = float4(confidenceSum > 1e-8 ? sum.rgb/confidenceSum : float3(0),
                clamp(confidenceSum/weightSum,0.0,1.0));
        }
        else
        {
            result = float4(sum.rgb/weightSum,1.0);
        }
        result.rgb = min(result.rgb,float3(65504));
#endif
    }
    outputImage.write(result,pixel);
}
