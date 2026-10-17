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
layout(set=0,binding=0,std140) uniform DenoiseUniforms
{
    mat4 inverseViewProjection;
    mat4 worldToView;
    vec4 thresholds;
    vec4 parameters;
    uvec4 extent;
    uvec4 counts;
} dataBlock;
layout(set=0,binding=1) uniform texture2D normalImage;
layout(set=0,binding=2) uniform texture2D positionImage;
layout(set=0,binding=3) uniform texture2D coverageImage;
layout(set=0,binding=4) uniform texture2D signalImage;
layout(set=0,binding=5) uniform texture2D materialImage;
layout(set=0,binding=6) uniform sampler nearestSampler;
#if DENOISE_SCALAR
layout(set=0,binding=7,r16f) uniform image2D outputImage;
#else
layout(set=0,binding=7,rgba16f) uniform image2D outputImage;
#endif
bool denoiseFinite(float value)
{
    return !isnan(value) && !isinf(value);
}
bool denoiseFinite3(vec3 value)
{
    return !any(isnan(value)) && !any(isinf(value));
}
ivec2 denoiseSource(ivec2 pixel)
{
    vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.extent.zw);
    return min(ivec2(uv*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
}
bool denoiseSurface(ivec2 pixel, out vec3 position, out vec3 normal)
{
    ivec2 source = denoiseSource(pixel);
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),source,0).xyz;
    bool covered = dot(encoded,encoded) > 1e-8;
#if DENOISE_WORLD
    covered = covered && texelFetch(sampler2D(coverageImage,nearestSampler),source,0).r > 0.0;
    position = texelFetch(sampler2D(positionImage,nearestSampler),source,0).xyz;
    normal = normalize(encoded);
#else
    float depth = texelFetch(sampler2D(positionImage,nearestSampler),source,0).r;
    covered = covered && depth < 1.0;
    vec2 uv = (vec2(source)+0.5)/vec2(dataBlock.extent.xy);
    vec4 world = dataBlock.inverseViewProjection*vec4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
    covered = covered && denoiseFinite((dataBlock.worldToView*vec4(position,1)).z);
#endif
    return covered && denoiseFinite3(position) && denoiseFinite3(normal) && dot(normal,normal) > 1e-8;
}
bool denoiseSignal(ivec2 pixel, out vec4 value)
{
    value = texelFetch(sampler2D(signalImage,nearestSampler),pixel,0);
#if DENOISE_SCALAR
    bool valid = denoiseFinite(value.r);
    value = vec4(clamp(value.r,0.0,1.0));
    return valid;
#else
    if (!denoiseFinite3(value.rgb) || !denoiseFinite(value.a))
    {
        return false;
    }
    value.rgb = max(value.rgb,vec3(0));
    if (dataBlock.counts.x == 1u)
    {
        bool valid = value.a > 0.5;
        value.a = 1.0;
        return valid;
    }
    ivec2 source = denoiseSource(pixel);
    float roughness = texelFetch(sampler2D(materialImage,nearestSampler),source,0).g;
    value.a = clamp(value.a,0.0,1.0);
    return denoiseFinite(roughness) && roughness <= dataBlock.parameters.y;
#endif
}
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    vec3 centerPosition, centerNormal;
    vec4 center;
    if (!denoiseSurface(ivec2(pixel),centerPosition,centerNormal) || !denoiseSignal(ivec2(pixel),center))
    {
#if DENOISE_SCALAR
        imageStore(outputImage,ivec2(pixel),vec4(1));
#else
        imageStore(outputImage,ivec2(pixel),vec4(0));
#endif
        return;
    }
    vec4 sum = vec4(0);
    float weightSum = 0.0, confidenceSum = 0.0;
    int radius = int(dataBlock.counts.y);
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            ivec2 neighbor = ivec2(pixel)+ivec2(x,y);
            if (any(lessThan(neighbor,ivec2(0))) || any(greaterThanEqual(neighbor,ivec2(dataBlock.extent.zw))))
            {
                continue;
            }
            vec3 position, normal;
            vec4 value;
            if (!denoiseSurface(neighbor,position,normal) || !denoiseSignal(neighbor,value))
            {
                continue;
            }
            float cosine = clamp(dot(centerNormal,normal),-1.0,1.0);
#if DENOISE_WORLD
            // Explicit UV surfaces reject distant world points, including coplanar chart seams.
            float depthDifference = length(position-centerPosition);
#else
            float depthDifference = abs((dataBlock.worldToView*vec4(position-centerPosition,0)).z);
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
    vec4 result = center;
    if (weightSum > 1e-8)
    {
#if DENOISE_SCALAR
        result = vec4(clamp(sum.r/weightSum,0.0,1.0));
#else
        if (dataBlock.counts.x == 2u)
        {
            result = vec4(confidenceSum > 1e-8 ? sum.rgb/confidenceSum : vec3(0),
                clamp(confidenceSum/weightSum,0.0,1.0));
        }
        else
        {
            result = vec4(sum.rgb/weightSum,1.0);
        }
        result.rgb = min(result.rgb,vec3(65504));
#endif
    }
    imageStore(outputImage,ivec2(pixel),result);
}
