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
struct TemporalUniforms {
    float4x4 inverseViewProjection;
    float4x4 previousViewProjection;
    float4 rejection;
    uint4 counts;
    uint4 extent;
    float4 cameraRoughness;
    uint4 flags;
#if TEMPORAL_METADATA
    float4x4 currentViewProjection;
    float4 metadataRejection;
#endif
};
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant TemporalUniforms& dataBlock [[buffer(1)]],
    texture2d<float> normalImage [[texture(0)]],
#if TEMPORAL_WORLD
    texture2d<float> positionImage [[texture(1)]],
#else
    depth2d<float> positionImage [[texture(1)]],
#endif
    texture2d<float> coverageImage [[texture(2)]],
    texture2d<float> currentImage [[texture(3)]],
    texture2d<float> previousImage [[texture(4)]],
    texture2d<float> previousPositionImage [[texture(5)]],
    texture2d<float> previousNormalImage [[texture(6)]],
    texture2d<float> materialImage [[texture(7)]],
    texture2d<float,access::write> outputImage [[texture(8)]],
    texture2d<float,access::write> outputPosition [[texture(9)]],
    texture2d<float,access::write> outputNormal [[texture(10)]]
#if TEMPORAL_METADATA
    , texture2d<uint> tokenImage [[texture(11)]]
    , depth2d<float> metadataDepthImage [[texture(12)]]
    , texture2d<uint> previousTokenImage [[texture(13)]]
#endif
#if TEMPORAL_DIAGNOSTIC
    , texture2d<float,access::write> outputAcceptance [[texture(14)]]
#endif
    )
{
    if (any(pixel >= dataBlock.extent.zw))
    {
        return;
    }
    float2 uv = (float2(pixel)+0.5)/float2(dataBlock.extent.zw);
    int2 sourcePixel = min(int2(uv*float2(dataBlock.extent.xy)),int2(dataBlock.extent.xy)-1);
    float3 encoded = normalImage.read(uint2(sourcePixel)).xyz;
    float3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if TEMPORAL_WORLD
    covered = covered && coverageImage.read(uint2(sourcePixel)).r > 0.0;
    position = positionImage.read(uint2(sourcePixel)).xyz;
    normal = normalize(encoded);
#else
    float depth = positionImage.read(uint2(sourcePixel));
    covered = covered && depth < 1.0;
    float2 sourceUV = (float2(sourcePixel)+0.5)/float2(dataBlock.extent.xy);
    float4 world = dataBlock.inverseViewProjection*float4(sourceUV.x*2.0-1.0,1.0-sourceUV.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
#endif
    float4 current = currentImage.read(uint2(int2(pixel)));
#if TEMPORAL_SCALAR
    covered = covered && ((as_type<uint>(current.r) & 0x7f800000u) != 0x7f800000u);
    current = float4(clamp(current.r,0.0,1.0));
#else
    covered = covered && (!any((as_type<uint4>(current) & uint4(0x7f800000u)) == uint4(0x7f800000u)));
    if (dataBlock.counts.x == 1u)
    {
        covered = covered && current.a > 0.5;
        current.a = 1.0;
    }
    else
    {
        float roughness = materialImage.read(uint2(sourcePixel)).g;
        covered = covered && ((as_type<uint>(roughness) & 0x7f800000u) != 0x7f800000u) && roughness <= dataBlock.cameraRoughness.w;
        current.a = clamp(current.a,0.0,1.0);
    }
#endif
    covered = covered && (!any((as_type<uint3>(position) & uint3(0x7f800000u)) == uint3(0x7f800000u))) && (!any((as_type<uint3>(normal) & uint3(0x7f800000u)) == uint3(0x7f800000u)));
    float4 result = current;
    float count = 1.0;
    float2 previousUV = uv;
    bool accepted = covered && dataBlock.rejection.w > 0.5;
    if (dataBlock.flags.x != 0u)
    {
        float4 clip = dataBlock.previousViewProjection*float4(position,1.0);
        accepted = accepted && clip.w > 1e-8 && clip.z >= 0.0 && clip.z <= clip.w;
        previousUV = clip.xy/(abs(clip.w) > 1e-8 ? clip.w : 1.0)*float2(0.5,-0.5)+0.5;
    }
    accepted = accepted && all(previousUV >= float2(0)) && all(previousUV < float2(1));
#if TEMPORAL_METADATA
    if (dataBlock.flags.y != 0u)
    {
        accepted = accepted && dataBlock.flags.z != 0u;
        if (accepted)
        {
            // UV is checked before conversion: an out-of-bounds predecessor never clamps.
            uint2 metadataPixel = uint2(previousUV*float2(dataBlock.extent.xy));
            uint token = tokenImage.read(uint2(sourcePixel)).r;
            uint previousToken = previousTokenImage.read(metadataPixel).r;
            float metadataDepth = metadataDepthImage.read(uint2(sourcePixel));
#if TEMPORAL_WORLD
            float4 currentClip = dataBlock.currentViewProjection*float4(position,1.0);
            float receiverDepth = currentClip.z/(abs(currentClip.w) > 1e-8 ? currentClip.w : 1.0);
            accepted = accepted && currentClip.w > 1e-8 && receiverDepth >= 0.0 && receiverDepth <= 1.0;
#else
            float receiverDepth = depth;
#endif
            accepted = accepted && token != 0u && token == previousToken &&
                abs(metadataDepth-receiverDepth) <= dataBlock.metadataRejection.x;
        }
    }
#endif
    if (accepted)
    {
        int2 previousPixel = int2(previousUV*float2(dataBlock.extent.zw));
        float4 previousPosition = previousPositionImage.read(uint2(previousPixel));
        float4 previousNormal = previousNormalImage.read(uint2(previousPixel));
        float4 previous = previousImage.read(uint2(previousPixel));
        float threshold = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.cameraRoughness.xyz);
        accepted = previousPosition.w > 0.5 && previousNormal.w >= 1.0 &&
            length(previousPosition.xyz-position) <= threshold && dot(normal,previousNormal.xyz) >= dataBlock.rejection.z;
        if (accepted)
        {
            float oldCount = min(previousNormal.w,float(dataBlock.counts.y));
            float oldWeight = dataBlock.counts.z != 0u && oldCount >= float(dataBlock.counts.y) ? 1.0 :
                1.0-1.0/min(oldCount+1.0,float(dataBlock.counts.y));
#if TEMPORAL_SCALAR
            result = mix(current,previous,oldWeight);
#else
            if (dataBlock.counts.x == 2u)
            {
                // Confidence-weighted RGB, with confidence accumulated separately.
                // A zero-confidence miss is distinct from roughness rejection.
                float confidence = mix(current.a,previous.a,oldWeight);
                float3 weighted = current.rgb*current.a*(1.0-oldWeight)+previous.rgb*previous.a*oldWeight;
                result = float4(confidence > 1e-8 ? weighted/confidence : float3(0),confidence);
            }
            else
            {
                result = float4(mix(current.rgb,previous.rgb,oldWeight),1.0);
            }
#endif
            count = min(oldCount+1.0,float(dataBlock.counts.y));
        }
    }
    if (!covered)
    {
#if TEMPORAL_SCALAR
        result = float4(1);
#else
        result = float4(0);
#endif
        position = float3(0);
        normal = float3(0);
        count = 0.0;
    }
    outputImage.write(result,pixel);
    outputPosition.write(float4(position,covered ? 1.0 : 0.0),pixel);
    outputNormal.write(float4(normal,count),pixel);
#if TEMPORAL_DIAGNOSTIC
    outputAcceptance.write(float4(covered ? (accepted ? float3(0,1,0) : float3(1,0,0)) : float3(0),1),pixel);
#endif
}
