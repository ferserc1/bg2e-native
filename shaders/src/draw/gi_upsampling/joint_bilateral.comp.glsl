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

#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(std140, set = 0, binding = 0) uniform GIUpsamplingUniforms {
    mat4 inverseViewProjection;
    mat4 worldToView;
    uvec4 extent;
    vec4 thresholds;
    vec4 parameters;
} dataBlock;
layout(set = 0, binding = 1) uniform texture2D signalImage;
layout(set = 0, binding = 2) uniform texture2D normalImage;
layout(set = 0, binding = 3) uniform texture2D depthImage;
layout(set = 0, binding = 4) uniform sampler nearestSampler;
layout(rgba16f, set = 0, binding = 5) uniform writeonly image2D outputImage;

// Bit checks remain valid with backend fast-math compilation settings.
bool finiteValue(float value)
{
    return (floatBitsToUint(value) & 0x7f800000u) != 0x7f800000u;
}

bool finiteVector(vec3 value)
{
    return finiteValue(value.x) && finiteValue(value.y) && finiteValue(value.z);
}

bool receiver(ivec2 pixel, out vec3 normal, out float viewDepth)
{
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),pixel,0).rgb;
    float depth = texelFetch(sampler2D(depthImage,nearestSampler),pixel,0).r;
    if (!finiteVector(encoded) || dot(encoded,encoded) <= 1e-8 ||
        !finiteValue(depth) || depth < 0.0 || depth >= 1.0)
    {
        return false;
    }
    vec3 decoded = encoded*2.0-1.0;
    float normalLength = dot(decoded,decoded);
    if (!finiteVector(decoded) || !finiteValue(normalLength) || normalLength <= 0.0)
    {
        return false;
    }
    normal = decoded/sqrt(normalLength);
    vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.extent.xy);
    vec4 world = dataBlock.inverseViewProjection*vec4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    if (!finiteValue(world.w) || abs(world.w) <= 1e-8)
    {
        return false;
    }
    vec3 position = world.xyz/world.w;
    viewDepth = (dataBlock.worldToView*vec4(position,1.0)).z;
    return finiteVector(normal) && finiteVector(position) && finiteValue(viewDepth);
}

void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.xy)))
    {
        return;
    }
    ivec2 p = ivec2(pixel);
    vec3 centerNormal;
    float centerDepth;
    vec4 result = vec4(0.0);
    if (!receiver(p,centerNormal,centerDepth))
    {
        imageStore(outputImage,p,result);
        return;
    }
    vec2 source = (vec2(p)+0.5)/vec2(dataBlock.extent.xy)*vec2(dataBlock.extent.zw)-0.5;
    ivec2 base = ivec2(floor(source));
    vec3 sum = vec3(0.0);
    float weightSum = 0.0;
    float nearestDistance = 0.0;
    vec3 nearestValue = vec3(0.0);
    bool found = false;
    for (int j = -1; j <= 2; ++j)
    {
        for (int i = -1; i <= 2; ++i)
        {
            ivec2 q = base+ivec2(i,j);
            if (any(lessThan(q,ivec2(0))) || any(greaterThanEqual(q,ivec2(dataBlock.extent.zw))))
            {
                continue;
            }
            vec4 value = texelFetch(sampler2D(signalImage,nearestSampler),q,0);
            if (!finiteVector(value.rgb) || !finiteValue(value.a) || value.a <= 0.5)
            {
                continue;
            }
            // Preserve the RTGI producer's exact receiver association and operation order.
            ivec2 h = min(ivec2((vec2(q)+0.5)/vec2(dataBlock.extent.zw)*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
            vec3 normal;
            float depth;
            if (!receiver(h,normal,depth))
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
            vec2 delta = (vec2(h)-vec2(p))*vec2(dataBlock.extent.zw)/vec2(dataBlock.extent.xy);
            float distanceSquared = dot(delta,delta);
            vec3 radiance = max(value.rgb,vec3(0.0));
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
        vec3 radiance = nearestValue;
        if (finiteValue(weightSum) && weightSum > dataBlock.parameters.z && finiteVector(sum))
        {
            vec3 filtered = sum/weightSum;
            if (finiteVector(filtered))
            {
                radiance = filtered;
            }
        }
        result = vec4(clamp(radiance,vec3(0.0),vec3(65504.0)),1.0);
    }
    imageStore(outputImage,p,result);
}
