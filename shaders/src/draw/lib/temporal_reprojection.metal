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
    float4x4 previousView;
#if TEMPORAL_ADAPTIVE
    float4 adaptive;
#endif
#endif
};
#if !TEMPORAL_ADAPTIVE
static_assert(sizeof(TemporalUniforms) == 352, "Temporal v3 uniform size");
#endif
#if TEMPORAL_ADAPTIVE
static_assert(sizeof(TemporalUniforms) == 368, "Temporal v4 uniform size");
static_assert(__builtin_offsetof(TemporalUniforms, adaptive) == 352, "Adaptive offset");
#endif
static_assert(__builtin_offsetof(TemporalUniforms, previousView) == 288, "Previous view offset");
bool finite4(float4 value)
{
    return all(isfinite(value));
}
// Geometric validation for each positive-weight footprint candidate.
// Positive depth follows the RH camera convention; motion never widens tolerance.
bool validPreviousGeometry(float4 previousPosition, float4 previousNormal,
    float3 expectedPosition, float3 expectedDirection, float4 previousCameraPosition,
    float4x4 previousView, float threshold, float normalCosine)
{
    float4 candidateCameraPosition = previousView*float4(previousPosition.xyz,1.0);
    float expectedDepth = -previousCameraPosition.z;
    float candidateDepth = -candidateCameraPosition.z;
    return finite4(previousPosition) && finite4(previousNormal) &&
        finite4(previousCameraPosition) && finite4(candidateCameraPosition) &&
        isfinite(threshold) && threshold >= 0.0 &&
        previousPosition.w > 0.5 && previousNormal.w >= 1.0 &&
        expectedDepth > 0.0 && candidateDepth > 0.0 &&
        abs(candidateDepth-expectedDepth) <= threshold &&
        length(previousPosition.xyz-expectedPosition) <= threshold &&
        dot(expectedDirection,previousNormal.xyz) >= normalCosine;
}

#if TEMPORAL_MOMENTS
#include "temporal_moments.metal"
#endif

#if TEMPORAL_ADAPTIVE
#include "temporal_current_statistics.metal"
#include "temporal_history_response.metal"
#endif
#if TEMPORAL_MOMENTS
#include "temporal_moment_reactivity.metal"
#endif

#if TEMPORAL_DIAGNOSTIC
// Fixed precedence: lower reason wins only when no valid footprint survives.
float3 reasonColor(uint reason)
{
    if (reason == 0u)
    {
        return float3(0.25); // No predecessor.
    }
    if (reason == 1u)
    {
        return float3(0.5,0,1); // Unsupported correspondence.
    }
    if (reason == 2u)
    {
        return float3(0,0.25,1); // Off-screen/behind camera.
    }
    if (reason == 3u)
    {
        return float3(0); // Uncovered.
    }
    if (reason == 4u)
    {
        return float3(1,0,1); // Token mismatch.
    }
    if (reason == 5u)
    {
        return float3(1,0,0); // Position/depth mismatch.
    }
    if (reason == 6u)
    {
        return float3(1,1,0); // Normal mismatch.
    }
    if (reason == 8u)
    {
        return float3(0,1,1); // Accepted partial footprint.
    }
    if (reason == 9u)
    {
        return float3(0,1,0); // Accepted full footprint.
    }
    return float3(1); // Invalid signal (7), or defensive unknown reason.
}
#endif

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
    , texture2d<float> expectedPositionImage [[texture(15)]]
    , texture2d<float> expectedNormalImage [[texture(16)]]
#if TEMPORAL_METRICS
    , texture2d<float,access::write> outputMetrics [[texture(19)]]
#endif
#if TEMPORAL_MOMENTS
    , texture2d<float> previousMomentImage [[texture(17)]]
    , texture2d<float,access::write> outputMoments [[texture(18)]]
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
#if TEMPORAL_DIAGNOSTIC
    bool receiverCovered = covered && finite4(float4(position,1)) && finite4(float4(normal,1));
    uint rejectionReason = 10u;
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
    // Geometric support is independent of reflection hit confidence. Reserved for Phase 4.
    float geometricConfidence = 0.0;
#if TEMPORAL_METRICS
    float diagnosticAlpha = covered ? 1.0 : 0.0;
    float diagnosticReactivity = 0.0;
    float diagnosticVariance = 0.0; // Zero also means no statistics evaluated (frozen/rejected).
#endif
#if TEMPORAL_MOMENTS
    float2 currentMoments = float2(0);
    if (covered)
    {
#if TEMPORAL_SCALAR
        float momentStatistic = boundedMomentStatistic(current.r);
#else
        // A reflection miss has no radiance sample; arbitrary miss RGB cannot enter moments.
        float luminance = dataBlock.counts.x == 2u && current.a <= CURRENT_STATISTICS_EPSILON ? 0.0 :
            max(dot(current.rgb,float3(0.2126,0.7152,0.0722)),0.0);
        float momentStatistic = boundedMomentStatistic(luminance);
#endif
        currentMoments = float2(momentStatistic,momentStatistic*momentStatistic);
    }
    float2 resolvedMoments = currentMoments;
#endif
    float4 result = current;
    float count = 1.0;
    float4 expectedPosition = expectedPositionImage.read(uint2(sourcePixel));
    float4 expectedNormal = expectedNormalImage.read(uint2(sourcePixel));
    float expectedNormalLength = length(expectedNormal.xyz);
    bool expectedValid = finite4(expectedPosition) && finite4(expectedNormal) &&
        expectedPosition.w > 0.5 && expectedNormal.w > 0.5 && expectedNormalLength > 1e-8 &&
        isfinite(expectedNormalLength);
    float3 expectedDirection = expectedNormal.xyz / (expectedNormalLength > 1e-8 ? expectedNormalLength : 1.0);
    float2 previousUV = uv;
    bool accepted = covered && expectedValid && dataBlock.rejection.w > 0.5 &&
        dataBlock.flags.y != 0u && dataBlock.flags.z != 0u;
    if (dataBlock.flags.x != 0u)
    {
        float4 clip = dataBlock.previousViewProjection*float4(expectedPosition.xyz,1.0);
        accepted = accepted && finite4(clip) && clip.w > 1e-8 && clip.z >= 0.0 && clip.z <= clip.w;
        previousUV = clip.xy/(abs(clip.w) > 1e-8 ? clip.w : 1.0)*float2(0.5,-0.5)+0.5;
    }
    accepted = accepted && all(previousUV >= float2(0)) && all(previousUV < float2(1));
#if TEMPORAL_DIAGNOSTIC
    if (dataBlock.rejection.w <= 0.5)
    {
        rejectionReason = min(rejectionReason,0u);
    }
    if (!expectedValid || dataBlock.flags.y == 0u || dataBlock.flags.z == 0u)
    {
        rejectionReason = min(rejectionReason,1u);
    }
    float4 diagnosticClip = dataBlock.previousViewProjection*float4(expectedPosition.xyz,1);
    if (!(all(previousUV >= float2(0)) && all(previousUV < float2(1))) || (dataBlock.flags.x != 0u &&
        (!finite4(diagnosticClip) || diagnosticClip.w <= 1e-8 || diagnosticClip.z < 0.0 || diagnosticClip.z > diagnosticClip.w)))
    {
        rejectionReason = min(rejectionReason,2u);
    }
    if (!receiverCovered)
    {
        rejectionReason = min(rejectionReason,3u);
    }
    if (receiverCovered && !covered)
    {
        rejectionReason = min(rejectionReason,7u);
    }
#endif
#if TEMPORAL_METADATA
    uint token = tokenImage.read(uint2(sourcePixel)).r;
#if TEMPORAL_DIAGNOSTIC
    if (token == 0u)
    {
        rejectionReason = min(rejectionReason,4u);
    }
#endif
    if (dataBlock.flags.y != 0u)
    {
        accepted = accepted && dataBlock.flags.z != 0u;
        if (accepted)
        {
            float metadataDepth = metadataDepthImage.read(uint2(sourcePixel));
#if TEMPORAL_WORLD
            float4 currentClip = dataBlock.currentViewProjection*float4(position,1.0);
            float receiverDepth = currentClip.z/(abs(currentClip.w) > 1e-8 ? currentClip.w : 1.0);
            accepted = accepted && currentClip.w > 1e-8 && receiverDepth >= 0.0 && receiverDepth <= 1.0;
#else
            float receiverDepth = depth;
#endif
            accepted = accepted && token != 0u &&
                abs(metadataDepth-receiverDepth) <= dataBlock.metadataRejection.x;
#if TEMPORAL_DIAGNOSTIC
            if (!accepted)
            {
                rejectionReason = min(rejectionReason,5u);
            }
#endif
        }
    }
#endif
    if (accepted)
    {
        float2 q = previousUV*float2(dataBlock.extent.zw)-0.5;
        int2 base = int2(floor(q));
        float2 fraction = q-float2(base);
        int2 metadataExtent = int2(previousTokenImage.get_width(),previousTokenImage.get_height());
        float4 previousCameraPosition = dataBlock.previousView*float4(expectedPosition.xyz,1.0);
        float threshold = dataBlock.rejection.x+dataBlock.rejection.y*length(previousCameraPosition.xyz);
        float4 historySum = float4(0);
        float validWeight = 0.0;
#if TEMPORAL_MOMENTS
        float2 momentSum = float2(0);
#endif
        float minimumCount = float(dataBlock.counts.y);
        for (int tap = 0; tap < 4; ++tap)
        {
            int2 offset = int2(tap & 1,tap >> 1);
            int2 previousPixel = base+offset;
            float weight = (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
                (offset.y == 0 ? 1.0-fraction.y : fraction.y);
            if (weight <= 0.0 || any(previousPixel < int2(0)) || any(previousPixel >= int2(dataBlock.extent.zw)))
            {
                continue;
            }
            // Map this signal texel center into the retained integer metadata image.
            float2 candidateUV = (float2(previousPixel)+0.5)/float2(dataBlock.extent.zw);
            int2 metadataPixel = int2(floor(candidateUV*float2(metadataExtent)));
            uint previousToken = previousTokenImage.read(uint2(metadataPixel)).r;
            if (previousToken == 0u || previousToken != token)
            {
#if TEMPORAL_DIAGNOSTIC
                // Coverage precedes identity even when both predicates fail.
                float4 rejectedPosition = previousPositionImage.read(uint2(previousPixel));
                float4 rejectedNormal = previousNormalImage.read(uint2(previousPixel));
                uint tokenReason = rejectedPosition.w <= 0.5 || rejectedNormal.w < 1.0 ||
                    !finite4(float4(rejectedNormal.w)) ? 3u : 4u;
                rejectionReason = min(rejectionReason,tokenReason);
#endif
                continue;
            }
            float4 previousPosition = previousPositionImage.read(uint2(previousPixel));
            float4 previousNormal = previousNormalImage.read(uint2(previousPixel));
            float4 candidate = previousImage.read(uint2(previousPixel));
            bool candidateValid = finite4(candidate) && validPreviousGeometry(previousPosition,previousNormal,
                expectedPosition.xyz,expectedDirection,previousCameraPosition,dataBlock.previousView,
                threshold,dataBlock.rejection.z);
#if !TEMPORAL_SCALAR
            // GI valid black survives; missing GI coverage does not contribute.
            candidateValid = candidateValid && (dataBlock.counts.x != 1u || candidate.a > 0.5);
#endif
#if TEMPORAL_MOMENTS
            float2 candidateMoments = float2(0);
            if (candidateValid)
            {
                candidateMoments = previousMomentImage.read(uint2(previousPixel)).rg;
                candidateValid = validMoments(candidateMoments);
            }
#endif
            if (!candidateValid)
            {
#if TEMPORAL_DIAGNOSTIC
                float4 candidateCamera = dataBlock.previousView*float4(previousPosition.xyz,1);
                uint candidateReason = 7u;
                if (!finite4(previousNormal) || dot(expectedDirection,previousNormal.xyz) < dataBlock.rejection.z)
                {
                    candidateReason = min(candidateReason,6u);
                }
                if (!finite4(previousPosition) || !finite4(candidateCamera) || !finite4(previousCameraPosition) ||
                    !finite4(float4(threshold)) || threshold < 0.0 ||
                    -candidateCamera.z <= 0.0 || -previousCameraPosition.z <= 0.0 ||
                    abs(candidateCamera.z-previousCameraPosition.z) > threshold ||
                    length(previousPosition.xyz-expectedPosition.xyz) > threshold)
                {
                    candidateReason = min(candidateReason,5u);
                }
                if (previousPosition.w <= 0.5 || previousNormal.w < 1.0 ||
                    !finite4(float4(previousNormal.w)))
                {
                    candidateReason = min(candidateReason,3u);
                }
                rejectionReason = min(rejectionReason,candidateReason);
#endif
                continue;
            }
#if !TEMPORAL_SCALAR
            if (dataBlock.counts.x == 2u)
            {
                candidate.a = clamp(candidate.a,0.0,1.0);
                candidate.rgb *= candidate.a;
            }
#endif
            historySum += candidate*weight;
#if TEMPORAL_MOMENTS
            momentSum += candidateMoments*weight;
#endif
            validWeight += weight;
            minimumCount = min(minimumCount,previousNormal.w);
        }
        accepted = validWeight > 1e-6 && finite4(historySum);
#if TEMPORAL_DIAGNOSTIC
        if (!accepted && rejectionReason == 10u)
        {
            rejectionReason = 7u;
        }
#endif
#if TEMPORAL_MOMENTS
        float2 previousMoments = float2(0);
        if (accepted)
        {
            previousMoments = momentSum/validWeight;
            accepted = validMoments(previousMoments);
        }
#endif
        float4 previous = float4(0);
        if (accepted)
        {
            previous = historySum/validWeight;
#if !TEMPORAL_SCALAR
            if (dataBlock.counts.x == 2u)
            {
                previous.rgb = previous.a > 1e-8 ? previous.rgb/previous.a : float3(0);
            }
#endif
            accepted = finite4(previous);
            geometricConfidence = accepted ? clamp(validWeight,0.0,1.0) : 0.0;
        }
        if (accepted)
        {
            float oldCount = minimumCount;
            float oldWeight = dataBlock.counts.z != 0u && oldCount >= float(dataBlock.counts.y) ? 1.0 :
                1.0-1.0/min(oldCount+1.0,float(dataBlock.counts.y));
#if TEMPORAL_ADAPTIVE
            // Freeze is literal outside the motion window; response never modifies a frozen pixel.
            bool frozen = dataBlock.counts.z != 0u && oldCount >= float(dataBlock.counts.y);
            if (dataBlock.adaptive.w > 0.5 && !frozen)
            {
                // Reactivity is measured against unclipped history, independently of confidence.
                CurrentStatistics currentStatistics = gatherCurrentStatistics(int2(pixel),token,position,normal,
                    dataBlock,normalImage,positionImage,currentImage,materialImage,tokenImage,metadataDepthImage);
                float alpha0 = 1.0/min(oldCount+1.0,float(dataBlock.counts.y));
                HistoryResponse response = respondToCurrentStatistics(previous,currentStatistics,dataBlock.adaptive.z);
                previous = response.history;
                float reactivity = response.reactivity;
#if TEMPORAL_MOMENTS
                reactivity = momentReactivity(currentStatistics,previousMoments,oldCount,dataBlock.adaptive.z);
#endif
#if TEMPORAL_METRICS
                diagnosticReactivity = reactivity;
                if (currentStatistics.varianceValid)
                {
#if TEMPORAL_MOMENTS
                    diagnosticVariance = currentStatistics.momentVariance;
                    if (oldCount >= 4.0)
                    {
                        diagnosticVariance = max(diagnosticVariance,
                            max(previousMoments.y-previousMoments.x*previousMoments.x,0.0));
                    }
#else
                    diagnosticVariance = currentStatistics.variance;
#endif
                }
#endif
                float alpha = clamp(max(max(alpha0,dataBlock.adaptive.x),
                    max((1.0-geometricConfidence)*dataBlock.adaptive.y,reactivity*dataBlock.adaptive.y)),0.0,1.0);
                if (alpha > alpha0)
                {
                    // Stored count now represents effective trust, not elapsed rendered frames.
                    float trustCap = max(1.0,1.0/max(alpha,CURRENT_STATISTICS_EPSILON)-1.0);
                    oldCount = min(oldCount,trustCap);
                }
                oldWeight = 1.0-alpha;
            }
#endif
#if TEMPORAL_METRICS
            diagnosticAlpha = 1.0-oldWeight;
#endif
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
#if TEMPORAL_MOMENTS
            // Same accepted taps and effective color alpha; oldWeight=1 keeps frozen moments.
            resolvedMoments = mix(currentMoments,previousMoments,oldWeight);
#endif
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
#if TEMPORAL_MOMENTS
        resolvedMoments = float2(0);
#endif
    }
    outputImage.write(result,pixel);
    outputPosition.write(float4(position,covered ? 1.0 : 0.0),pixel);
    outputNormal.write(float4(normal,count),pixel);
#if TEMPORAL_MOMENTS
    outputMoments.write(float4(resolvedMoments,0,1),pixel);
#endif
#if TEMPORAL_METRICS
    // Bounded visual encoding: alpha, reactivity, variance/(1+variance), count/maxFrames.
    float4 metrics = covered ? float4(clamp(diagnosticAlpha,0.0,1.0),clamp(diagnosticReactivity,0.0,1.0),
        diagnosticVariance/(1.0+diagnosticVariance),clamp(count/float(dataBlock.counts.y),0.0,1.0)) : float4(0);
    outputMetrics.write(metrics,pixel);
#endif
#if TEMPORAL_DIAGNOSTIC
    uint reason = accepted ? (geometricConfidence >= 1.0-1e-6 ? 9u : 8u) : rejectionReason;
    outputAcceptance.write(float4(reasonColor(reason),1),pixel);
#endif
}
