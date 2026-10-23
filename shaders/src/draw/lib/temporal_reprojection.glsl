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

layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
layout(set=0,binding=0,std140) uniform TemporalUniforms
{
    mat4 inverseViewProjection;
    mat4 previousViewProjection;
    vec4 rejection;
    uvec4 counts;
    uvec4 extent;
    vec4 cameraRoughness;
    uvec4 flags;
#if TEMPORAL_METADATA
    mat4 currentViewProjection;
    vec4 metadataRejection;
    layout(offset = 288) mat4 previousView;
#if TEMPORAL_ADAPTIVE
    layout(offset = 352) vec4 adaptive;
#endif
#endif
} dataBlock;
layout(set=0,binding=1) uniform texture2D normalImage;
layout(set=0,binding=2) uniform texture2D positionImage;
layout(set=0,binding=3) uniform texture2D coverageImage;
layout(set=0,binding=4) uniform texture2D currentImage;
layout(set=0,binding=5) uniform texture2D previousImage;
layout(set=0,binding=6) uniform texture2D previousPositionImage;
layout(set=0,binding=7) uniform texture2D previousNormalImage;
layout(set=0,binding=8) uniform texture2D materialImage;
layout(set=0,binding=9) uniform sampler nearestSampler;
#if TEMPORAL_SCALAR
layout(set=0,binding=10,r16f) uniform image2D outputImage;
#else
layout(set=0,binding=10,rgba16f) uniform image2D outputImage;
#endif
layout(set=0,binding=11,rgba32f) uniform image2D outputPosition;
layout(set=0,binding=12,rgba16f) uniform image2D outputNormal;
#if TEMPORAL_METADATA
layout(set=0,binding=13) uniform utexture2D tokenImage;
layout(set=0,binding=14) uniform texture2D metadataDepthImage;
layout(set=0,binding=15) uniform utexture2D previousTokenImage;
#endif
layout(set=0,binding=17) uniform texture2D expectedPositionImage;
layout(set=0,binding=18) uniform texture2D expectedNormalImage;
#if TEMPORAL_DIAGNOSTIC
layout(set=0,binding=16,rgba16f) uniform image2D outputAcceptance;
#endif
#if TEMPORAL_MOMENTS
layout(set=0,binding=19) uniform texture2D previousMomentImage;
layout(set=0,binding=20,rg16f) uniform image2D outputMoments;
#endif
#if TEMPORAL_METRICS
layout(set=0,binding=21,rgba16f) uniform image2D outputMetrics;
#endif
bool finite4(vec4 value)
{
    return !any(isnan(value)) && !any(isinf(value));
}
// Geometric validation for each positive-weight footprint candidate.
// Positive depth follows the RH camera convention; motion never widens tolerance.
bool validPreviousGeometry(vec4 previousPosition, vec4 previousNormal,
    vec3 expectedPosition, vec3 expectedDirection, vec4 previousCameraPosition,
    mat4 previousView, float threshold, float normalCosine)
{
    vec4 candidateCameraPosition = previousView*vec4(previousPosition.xyz,1.0);
    float expectedDepth = -previousCameraPosition.z;
    float candidateDepth = -candidateCameraPosition.z;
    return finite4(previousPosition) && finite4(previousNormal) &&
        finite4(previousCameraPosition) && finite4(candidateCameraPosition) &&
        !isnan(threshold) && !isinf(threshold) && threshold >= 0.0 &&
        previousPosition.w > 0.5 && previousNormal.w >= 1.0 &&
        expectedDepth > 0.0 && candidateDepth > 0.0 &&
        abs(candidateDepth-expectedDepth) <= threshold &&
        length(previousPosition.xyz-expectedPosition) <= threshold &&
        dot(expectedDirection,previousNormal.xyz) >= normalCosine;
}

#if TEMPORAL_MOMENTS
#include "temporal_moments.glsl"
#endif

#if TEMPORAL_ADAPTIVE
#include "temporal_current_statistics.glsl"
#include "temporal_history_response.glsl"
#endif
#if TEMPORAL_MOMENTS
#include "temporal_moment_reactivity.glsl"
#endif

#if TEMPORAL_DIAGNOSTIC
// Fixed precedence: lower reason wins only when no valid footprint survives.
vec3 reasonColor(uint reason)
{
    if (reason == 0u)
    {
        return vec3(0.25); // No predecessor.
    }
    if (reason == 1u)
    {
        return vec3(0.5,0,1); // Unsupported correspondence.
    }
    if (reason == 2u)
    {
        return vec3(0,0.25,1); // Off-screen/behind camera.
    }
    if (reason == 3u)
    {
        return vec3(0); // Uncovered.
    }
    if (reason == 4u)
    {
        return vec3(1,0,1); // Token mismatch.
    }
    if (reason == 5u)
    {
        return vec3(1,0,0); // Position/depth mismatch.
    }
    if (reason == 6u)
    {
        return vec3(1,1,0); // Normal mismatch.
    }
    if (reason == 8u)
    {
        return vec3(0,1,1); // Accepted partial footprint.
    }
    if (reason == 9u)
    {
        return vec3(0,1,0); // Accepted full footprint.
    }
    return vec3(1); // Invalid signal (7), or defensive unknown reason.
}
#endif

void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.extent.zw);
    ivec2 sourcePixel = min(ivec2(uv*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),sourcePixel,0).xyz;
    vec3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if TEMPORAL_WORLD
    covered = covered && texelFetch(sampler2D(coverageImage,nearestSampler),sourcePixel,0).r > 0.0;
    position = texelFetch(sampler2D(positionImage,nearestSampler),sourcePixel,0).xyz;
    normal = normalize(encoded);
#else
    float depth = texelFetch(sampler2D(positionImage,nearestSampler),sourcePixel,0).r;
    covered = covered && depth < 1.0;
    vec2 sourceUV = (vec2(sourcePixel)+0.5)/vec2(dataBlock.extent.xy);
    vec4 world = dataBlock.inverseViewProjection*vec4(sourceUV.x*2.0-1.0,1.0-sourceUV.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
#endif
#if TEMPORAL_DIAGNOSTIC
    bool receiverCovered = covered && finite4(vec4(position,1)) && finite4(vec4(normal,1));
    uint rejectionReason = 10u;
#endif
    vec4 current = texelFetch(sampler2D(currentImage,nearestSampler),ivec2(pixel),0);
#if TEMPORAL_SCALAR
    covered = covered && (!isnan(current.r) && !isinf(current.r));
    current = vec4(clamp(current.r,0.0,1.0));
#else
    covered = covered && (!any(isnan(current)) && !any(isinf(current)));
    if (dataBlock.counts.x == 1u)
    {
        covered = covered && current.a > 0.5;
        current.a = 1.0;
    }
    else
    {
        float roughness = texelFetch(sampler2D(materialImage,nearestSampler),sourcePixel,0).g;
        covered = covered && (!isnan(roughness) && !isinf(roughness)) && roughness <= dataBlock.cameraRoughness.w;
        current.a = clamp(current.a,0.0,1.0);
    }
#endif
    covered = covered && (!any(isnan(position)) && !any(isinf(position))) && (!any(isnan(normal)) && !any(isinf(normal)));
    // Geometric support is independent of reflection hit confidence. Reserved for Phase 4.
    float geometricConfidence = 0.0;
#if TEMPORAL_METRICS
    float diagnosticAlpha = covered ? 1.0 : 0.0;
    float diagnosticReactivity = 0.0;
    float diagnosticVariance = 0.0; // Zero also means no statistics evaluated (frozen/rejected).
#endif
#if TEMPORAL_MOMENTS
    vec2 currentMoments = vec2(0);
    if (covered)
    {
#if TEMPORAL_SCALAR
        float momentStatistic = boundedMomentStatistic(current.r);
#else
        // A reflection miss has no radiance sample; arbitrary miss RGB cannot enter moments.
        float luminance = dataBlock.counts.x == 2u && current.a <= CURRENT_STATISTICS_EPSILON ? 0.0 :
            max(dot(current.rgb,vec3(0.2126,0.7152,0.0722)),0.0);
        float momentStatistic = boundedMomentStatistic(luminance);
#endif
        currentMoments = vec2(momentStatistic,momentStatistic*momentStatistic);
    }
    vec2 resolvedMoments = currentMoments;
#endif
    vec4 result = current;
    float count = 1.0;
    vec4 expectedPosition = texelFetch(sampler2D(expectedPositionImage,nearestSampler),sourcePixel,0);
    vec4 expectedNormal = texelFetch(sampler2D(expectedNormalImage,nearestSampler),sourcePixel,0);
    float expectedNormalLength = length(expectedNormal.xyz);
    bool expectedValid = finite4(expectedPosition) && finite4(expectedNormal) &&
        expectedPosition.w > 0.5 && expectedNormal.w > 0.5 && expectedNormalLength > 1e-8 &&
        !isnan(expectedNormalLength) && !isinf(expectedNormalLength);
    vec3 expectedDirection = expectedNormal.xyz / (expectedNormalLength > 1e-8 ? expectedNormalLength : 1.0);
    vec2 previousUV = uv;
    bool accepted = covered && expectedValid && dataBlock.rejection.w > 0.5 &&
        dataBlock.flags.y != 0u && dataBlock.flags.z != 0u;
    if (dataBlock.flags.x != 0u)
    {
        vec4 clip = dataBlock.previousViewProjection*vec4(expectedPosition.xyz,1.0);
        accepted = accepted && finite4(clip) && clip.w > 1e-8 && clip.z >= 0.0 && clip.z <= clip.w;
        previousUV = clip.xy/(abs(clip.w) > 1e-8 ? clip.w : 1.0)*vec2(0.5,-0.5)+0.5;
    }
    accepted = accepted && all(greaterThanEqual(previousUV,vec2(0))) && all(lessThan(previousUV,vec2(1)));
#if TEMPORAL_DIAGNOSTIC
    if (dataBlock.rejection.w <= 0.5)
    {
        rejectionReason = min(rejectionReason,0u);
    }
    if (!expectedValid || dataBlock.flags.y == 0u || dataBlock.flags.z == 0u)
    {
        rejectionReason = min(rejectionReason,1u);
    }
    vec4 diagnosticClip = dataBlock.previousViewProjection*vec4(expectedPosition.xyz,1);
    if (!(all(greaterThanEqual(previousUV,vec2(0))) && all(lessThan(previousUV,vec2(1)))) || (dataBlock.flags.x != 0u &&
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
    uint token = texelFetch(usampler2D(tokenImage,nearestSampler),sourcePixel,0).r;
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
            float metadataDepth = texelFetch(sampler2D(metadataDepthImage,nearestSampler),sourcePixel,0).r;
#if TEMPORAL_WORLD
            vec4 currentClip = dataBlock.currentViewProjection*vec4(position,1.0);
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
        vec2 q = previousUV*vec2(dataBlock.extent.zw)-0.5;
        ivec2 base = ivec2(floor(q));
        vec2 fraction = q-vec2(base);
        ivec2 metadataExtent = textureSize(usampler2D(previousTokenImage,nearestSampler),0);
        vec4 previousCameraPosition = dataBlock.previousView*vec4(expectedPosition.xyz,1.0);
        float threshold = dataBlock.rejection.x+dataBlock.rejection.y*length(previousCameraPosition.xyz);
        vec4 historySum = vec4(0);
        float validWeight = 0.0;
#if TEMPORAL_MOMENTS
        vec2 momentSum = vec2(0);
#endif
        float minimumCount = float(dataBlock.counts.y);
        for (int tap = 0; tap < 4; ++tap)
        {
            ivec2 offset = ivec2(tap & 1,tap >> 1);
            ivec2 previousPixel = base+offset;
            float weight = (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
                (offset.y == 0 ? 1.0-fraction.y : fraction.y);
            if (weight <= 0.0 || any(lessThan(previousPixel,ivec2(0))) || any(greaterThanEqual(previousPixel,ivec2(dataBlock.extent.zw))))
            {
                continue;
            }
            // Map this signal texel center into the retained integer metadata image.
            vec2 candidateUV = (vec2(previousPixel)+0.5)/vec2(dataBlock.extent.zw);
            ivec2 metadataPixel = ivec2(floor(candidateUV*vec2(metadataExtent)));
            uint previousToken = texelFetch(usampler2D(previousTokenImage,nearestSampler),metadataPixel,0).r;
            if (previousToken == 0u || previousToken != token)
            {
#if TEMPORAL_DIAGNOSTIC
                // Coverage precedes identity even when both predicates fail.
                vec4 rejectedPosition = texelFetch(sampler2D(previousPositionImage,nearestSampler),previousPixel,0);
                vec4 rejectedNormal = texelFetch(sampler2D(previousNormalImage,nearestSampler),previousPixel,0);
                uint tokenReason = rejectedPosition.w <= 0.5 || rejectedNormal.w < 1.0 ||
                    !finite4(vec4(rejectedNormal.w)) ? 3u : 4u;
                rejectionReason = min(rejectionReason,tokenReason);
#endif
                continue;
            }
            vec4 previousPosition = texelFetch(sampler2D(previousPositionImage,nearestSampler),previousPixel,0);
            vec4 previousNormal = texelFetch(sampler2D(previousNormalImage,nearestSampler),previousPixel,0);
            vec4 candidate = texelFetch(sampler2D(previousImage,nearestSampler),previousPixel,0);
            bool candidateValid = finite4(candidate) && validPreviousGeometry(previousPosition,previousNormal,
                expectedPosition.xyz,expectedDirection,previousCameraPosition,dataBlock.previousView,
                threshold,dataBlock.rejection.z);
#if !TEMPORAL_SCALAR
            // GI valid black survives; missing GI coverage does not contribute.
            candidateValid = candidateValid && (dataBlock.counts.x != 1u || candidate.a > 0.5);
#endif
#if TEMPORAL_MOMENTS
            vec2 candidateMoments = vec2(0);
            if (candidateValid)
            {
                candidateMoments = texelFetch(sampler2D(previousMomentImage,nearestSampler),previousPixel,0).rg;
                candidateValid = validMoments(candidateMoments);
            }
#endif
            if (!candidateValid)
            {
#if TEMPORAL_DIAGNOSTIC
                vec4 candidateCamera = dataBlock.previousView*vec4(previousPosition.xyz,1);
                uint candidateReason = 7u;
                if (!finite4(previousNormal) || dot(expectedDirection,previousNormal.xyz) < dataBlock.rejection.z)
                {
                    candidateReason = min(candidateReason,6u);
                }
                if (!finite4(previousPosition) || !finite4(candidateCamera) || !finite4(previousCameraPosition) ||
                    !finite4(vec4(threshold)) || threshold < 0.0 ||
                    -candidateCamera.z <= 0.0 || -previousCameraPosition.z <= 0.0 ||
                    abs(candidateCamera.z-previousCameraPosition.z) > threshold ||
                    length(previousPosition.xyz-expectedPosition.xyz) > threshold)
                {
                    candidateReason = min(candidateReason,5u);
                }
                if (previousPosition.w <= 0.5 || previousNormal.w < 1.0 ||
                    !finite4(vec4(previousNormal.w)))
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
        vec2 previousMoments = vec2(0);
        if (accepted)
        {
            previousMoments = momentSum/validWeight;
            accepted = validMoments(previousMoments);
        }
#endif
        vec4 previous = vec4(0);
        if (accepted)
        {
            previous = historySum/validWeight;
#if !TEMPORAL_SCALAR
            if (dataBlock.counts.x == 2u)
            {
                previous.rgb = previous.a > 1e-8 ? previous.rgb/previous.a : vec3(0);
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
                CurrentStatistics currentStatistics = gatherCurrentStatistics(ivec2(pixel),token,position,normal);
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
                vec3 weighted = current.rgb*current.a*(1.0-oldWeight)+previous.rgb*previous.a*oldWeight;
                result = vec4(confidence > 1e-8 ? weighted/confidence : vec3(0),confidence);
            }
            else
            {
                result = vec4(mix(current.rgb,previous.rgb,oldWeight),1.0);
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
        result = vec4(1);
#else
        result = vec4(0);
#endif
        position = vec3(0);
        normal = vec3(0);
        count = 0.0;
#if TEMPORAL_MOMENTS
        resolvedMoments = vec2(0);
#endif
    }
    imageStore(outputImage,ivec2(pixel),result);
    imageStore(outputPosition,ivec2(pixel),vec4(position,covered ? 1.0 : 0.0));
    imageStore(outputNormal,ivec2(pixel),vec4(normal,count));
#if TEMPORAL_MOMENTS
    imageStore(outputMoments,ivec2(pixel),vec4(resolvedMoments,0,1));
#endif
#if TEMPORAL_METRICS
    // Bounded visual encoding: alpha, reactivity, variance/(1+variance), count/maxFrames.
    vec4 metrics = covered ? vec4(clamp(diagnosticAlpha,0.0,1.0),clamp(diagnosticReactivity,0.0,1.0),
        diagnosticVariance/(1.0+diagnosticVariance),clamp(count/float(dataBlock.counts.y),0.0,1.0)) : vec4(0);
    imageStore(outputMetrics,ivec2(pixel),metrics);
#endif
#if TEMPORAL_DIAGNOSTIC
    uint reason = accepted ? (geometricConfidence >= 1.0-1e-6 ? 9u : 8u) : rejectionReason;
    imageStore(outputAcceptance,ivec2(pixel),vec4(reasonColor(reason),1));
#endif
}
