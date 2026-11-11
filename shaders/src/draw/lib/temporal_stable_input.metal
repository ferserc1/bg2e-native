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

// New samples are filtered once into the stable grid. This helper never reads history.
float4 stableGICurrent(int2 pixel, float3 centerPosition, float3 centerNormal,
    constant TemporalUniforms& dataBlock, texture2d<float> currentImage,
    texture2d<float> normalImage, depth2d<float> positionImage
#if TEMPORAL_METADATA
    , texture2d<uint> tokenImage
#endif
    )
{
    if (all(dataBlock.stableJitter.xy == float2(0)))
    {
        return currentImage.read(uint2(pixel));
    }
    float2 currentUV = (float2(pixel)+0.5)/float2(dataBlock.extent.zw)+dataBlock.stableJitter.xy;
    int2 centerReceiver = clamp(int2(currentUV*float2(dataBlock.extent.xy)),int2(0),int2(dataBlock.extent.xy)-1);
#if TEMPORAL_METADATA
    uint centerToken = tokenImage.read(uint2(centerReceiver)).r;
#endif
    float2 q = currentUV*float2(dataBlock.extent.zw)-0.5;
    int2 base = int2(floor(q));
    float2 fraction = q-float2(base);
    float4 sum = float4(0);
    float weightSum = 0.0;
    float tolerance = dataBlock.rejection.x+dataBlock.rejection.y*length(centerPosition-dataBlock.cameraRoughness.xyz);
    for (int tap = 0; tap < 4; ++tap)
    {
        int2 offset = int2(tap & 1,tap >> 1);
        int2 candidate = base+offset;
        float weight = (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
            (offset.y == 0 ? 1.0-fraction.y : fraction.y);
        if (weight <= 0.0 || any(candidate < int2(0)) || any(candidate >= int2(dataBlock.extent.zw)))
        {
            continue;
        }
        float2 uv = (float2(candidate)+0.5)/float2(dataBlock.extent.zw);
        int2 receiver = min(int2(uv*float2(dataBlock.extent.xy)),int2(dataBlock.extent.xy)-1);
#if TEMPORAL_METADATA
        if (dataBlock.flags.y != 0u && (centerToken == 0u || tokenImage.read(uint2(receiver)).r != centerToken))
        {
            continue;
        }
#endif
        float depth = positionImage.read(uint2(receiver));
        float3 encoded = normalImage.read(uint2(receiver)).xyz;
        float3 decoded = encoded*2.0-1.0;
        float normalLength = dot(decoded,decoded);
        float2 receiverUV = (float2(receiver)+0.5)/float2(dataBlock.extent.xy);
        float4 world = dataBlock.inverseViewProjection*float4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,depth,1.0);
        float4 value = currentImage.read(uint2(candidate));
        if (!all(isfinite(value)) || !all(isfinite(world)) || !all(isfinite(float4(decoded,depth))) ||
            depth < 0.0 || depth >= 1.0 || dot(encoded,encoded) <= 1e-8 ||
            normalLength <= 1e-8 || world.w <= 1e-8 || value.a <= 0.5)
        {
            continue;
        }
        float3 position = world.xyz/world.w;
        if (length(position-centerPosition) > tolerance ||
            dot(decoded/sqrt(normalLength),centerNormal) < dataBlock.rejection.z)
        {
            continue;
        }
        sum += value*weight;
        weightSum += weight;
    }
    return weightSum > 1e-6 ? float4(sum.rgb/weightSum,1) : float4(0);
}
