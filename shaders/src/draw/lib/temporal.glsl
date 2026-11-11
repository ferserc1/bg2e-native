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

#ifndef TEMPORAL_STABLE_GI
#define TEMPORAL_STABLE_GI 0
#endif
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
#endif
#if TEMPORAL_STABLE_GI
    vec4 stableJitter; // current and previous framebuffer UV offsets.
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
#if TEMPORAL_DIAGNOSTIC
layout(set=0,binding=16,rgba16f) uniform image2D outputAcceptance;
#endif
#if TEMPORAL_STABLE_GI
#include "temporal_stable_input.glsl"
#endif

void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.extent.zw);
#if TEMPORAL_STABLE_GI
    vec2 jitteredUV = uv+dataBlock.stableJitter.xy;
    bool receiverInBounds = all(greaterThanEqual(jitteredUV,vec2(0))) && all(lessThan(jitteredUV,vec2(1)));
    ivec2 sourcePixel = clamp(ivec2(jitteredUV*vec2(dataBlock.extent.xy)),ivec2(0),ivec2(dataBlock.extent.xy)-1);
#else
    ivec2 sourcePixel = min(ivec2(uv*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
#endif
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),sourcePixel,0).xyz;
    vec3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if TEMPORAL_STABLE_GI
    covered = covered && receiverInBounds;
#endif
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
#if TEMPORAL_STABLE_GI
    vec4 current = stableGICurrent(ivec2(pixel),position,normal);
#else
    vec4 current = texelFetch(sampler2D(currentImage,nearestSampler),ivec2(pixel),0);
#endif
#if TEMPORAL_SCALAR
    covered = covered && (!isnan(current.r) && !isinf(current.r));
    current = vec4(clamp(current.r,0.0,1.0));
#else
    covered = covered && (!any(isnan(current)) && !any(isinf(current)));
    if (dataBlock.counts.x == 1u)
    {
#if !TEMPORAL_STABLE_GI
        covered = covered && current.a > 0.5;
        current.a = 1.0;
#endif
    }
    else
    {
        float roughness = texelFetch(sampler2D(materialImage,nearestSampler),sourcePixel,0).g;
        covered = covered && (!isnan(roughness) && !isinf(roughness)) && roughness <= dataBlock.cameraRoughness.w;
        current.a = clamp(current.a,0.0,1.0);
    }
#endif
    covered = covered && (!any(isnan(position)) && !any(isinf(position))) && (!any(isnan(normal)) && !any(isinf(normal)));
    vec4 result = current;
    float count = 1.0;
#if TEMPORAL_STABLE_GI
    bool hasCurrentGI = current.a > 0.5;
    count = hasCurrentGI ? 1.0 : 0.0;
#endif
    vec2 previousUV = uv;
    bool accepted = covered && dataBlock.rejection.w > 0.5;
    if (dataBlock.flags.x != 0u)
    {
        vec4 clip = dataBlock.previousViewProjection*vec4(position,1.0);
        accepted = accepted && clip.w > 1e-8 && clip.z >= 0.0 && clip.z <= clip.w;
        previousUV = clip.xy/(abs(clip.w) > 1e-8 ? clip.w : 1.0)*vec2(0.5,-0.5)+0.5;
    }
    accepted = accepted && all(greaterThanEqual(previousUV,vec2(0))) && all(lessThan(previousUV,vec2(1)));
#if TEMPORAL_METADATA
    if (dataBlock.flags.y != 0u)
    {
        accepted = accepted && dataBlock.flags.z != 0u;
        if (accepted)
        {
            // UV is checked before conversion: an out-of-bounds predecessor never clamps.
#if TEMPORAL_STABLE_GI
            vec2 metadataUV = previousUV+dataBlock.stableJitter.zw;
            if (any(lessThan(metadataUV,vec2(0))) || any(greaterThanEqual(metadataUV,vec2(1))))
            {
                accepted = false;
            }
            ivec2 metadataPixel = ivec2(clamp(metadataUV,vec2(0),vec2(1))*vec2(dataBlock.extent.xy));
            metadataPixel = min(metadataPixel,ivec2(dataBlock.extent.xy)-1);
#else
            ivec2 metadataPixel = ivec2(previousUV*vec2(dataBlock.extent.xy));
#endif
            uint token = texelFetch(usampler2D(tokenImage,nearestSampler),sourcePixel,0).r;
            uint previousToken = texelFetch(usampler2D(previousTokenImage,nearestSampler),metadataPixel,0).r;
            float metadataDepth = texelFetch(sampler2D(metadataDepthImage,nearestSampler),sourcePixel,0).r;
#if TEMPORAL_WORLD
            vec4 currentClip = dataBlock.currentViewProjection*vec4(position,1.0);
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
#if TEMPORAL_WORLD
        ivec2 previousPixel = ivec2(previousUV*vec2(dataBlock.extent.zw));
        vec4 previousPosition = texelFetch(sampler2D(previousPositionImage,nearestSampler),previousPixel,0);
        vec4 previousNormal = texelFetch(sampler2D(previousNormalImage,nearestSampler),previousPixel,0);
        vec4 previous = texelFetch(sampler2D(previousImage,nearestSampler),previousPixel,0);
        float threshold = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.cameraRoughness.xyz);
        accepted = previousPosition.w > 0.5 && previousNormal.w >= 1.0 &&
            length(previousPosition.xyz-position) <= threshold && dot(normal,previousNormal.xyz) >= dataBlock.rejection.z;
#else
        // Receiver guides and signals can have different pixel centers. Reproject
        // the receiver displacement, retaining the signal's own subpixel offset.
        vec2 historyUV = uv;
        if (dataBlock.flags.x != 0u)
        {
#if TEMPORAL_STABLE_GI
        historyUV += previousUV-(sourceUV-dataBlock.stableJitter.xy);
        if (dataBlock.flags.w != 0u)
        {
            historyUV = uv;
        }
#else
        historyUV += previousUV-sourceUV;
#endif
        }
        vec2 historyPixel = historyUV*vec2(dataBlock.extent.zw)-0.5;
#if TEMPORAL_STABLE_GI
        // A stationary history cell must remain an exact texel, not an almost-integer UV.
        if (dataBlock.flags.w != 0u && all(equal(historyUV,uv)))
        {
            historyPixel = vec2(pixel);
        }
#endif
        ivec2 base = ivec2(floor(historyPixel));
        vec2 fraction = historyPixel-vec2(base);
        vec4 historySum = vec4(0.0);
        float weightSum = 0.0;
        float minimumCount = float(dataBlock.counts.y);
        float threshold = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.cameraRoughness.xyz);
        // Preserve fractional jitter motion instead of rounding every frame to a
        // whole history texel, which can accumulate a directional drift.
        for (int tap = 0; tap < 4; ++tap)
        {
            ivec2 offset = ivec2(tap & 1,tap >> 1);
            ivec2 previousPixel = base+offset;
            float weight = (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
                (offset.y == 0 ? 1.0-fraction.y : fraction.y);
            if (weight <= 0.0 || any(lessThan(previousPixel,ivec2(0))) ||
                any(greaterThanEqual(previousPixel,ivec2(dataBlock.extent.zw))))
            {
                continue;
            }
            vec4 previousPosition = texelFetch(sampler2D(previousPositionImage,nearestSampler),previousPixel,0);
            vec4 previousNormal = texelFetch(sampler2D(previousNormalImage,nearestSampler),previousPixel,0);
            vec4 candidate = texelFetch(sampler2D(previousImage,nearestSampler),previousPixel,0);
            if (previousPosition.w <= 0.5 || previousNormal.w < 1.0 ||
                !(length(previousPosition.xyz-position) <= threshold) ||
                !(dot(normal,previousNormal.xyz) >= dataBlock.rejection.z) ||
                (any(isnan(candidate)) || any(isinf(candidate))))
            {
                continue;
            }
#if !TEMPORAL_SCALAR
            if (dataBlock.counts.x == 1u && candidate.a <= 0.5)
            {
                continue;
            }
            if (dataBlock.counts.x == 2u)
            {
                candidate.a = clamp(candidate.a,0.0,1.0);
                candidate.rgb *= candidate.a;
            }
#endif
            historySum += candidate*weight;
            weightSum += weight;
            minimumCount = min(minimumCount,previousNormal.w);
        }
        accepted = weightSum > 1e-6;
        vec4 previous = accepted ? historySum/weightSum : vec4(0.0);
#if !TEMPORAL_SCALAR
        if (dataBlock.counts.x == 2u)
        {
            previous.rgb = previous.a > 1e-8 ? previous.rgb/previous.a : vec3(0.0);
        }
#endif
        vec4 previousNormal = vec4(0.0,0.0,0.0,minimumCount);
#endif
        if (accepted)
        {
            float oldCount = min(previousNormal.w,float(dataBlock.counts.y));
            float oldWeight = dataBlock.counts.z != 0u && oldCount >= float(dataBlock.counts.y) ? 1.0 :
                1.0-1.0/min(oldCount+1.0,float(dataBlock.counts.y));
#if TEMPORAL_STABLE_GI
            // Only validated receiver history can fill a missing current sample.
            if (!hasCurrentGI)
            {
                oldWeight = 1.0;
            }
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
#if TEMPORAL_STABLE_GI
            // Reusing history is not an additional observation.
            if (!hasCurrentGI)
            {
                count = oldCount;
            }
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
    }
    imageStore(outputImage,ivec2(pixel),result);
    imageStore(outputPosition,ivec2(pixel),vec4(position,covered ? 1.0 : 0.0));
    imageStore(outputNormal,ivec2(pixel),vec4(normal,count));
#if TEMPORAL_DIAGNOSTIC
    imageStore(outputAcceptance,ivec2(pixel),vec4(covered ? (accepted ? vec3(0,1,0) : vec3(1,0,0)) : vec3(0),1));
#endif
}
