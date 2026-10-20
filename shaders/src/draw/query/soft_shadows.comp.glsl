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
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#include "../lib/bindings.glsl"
struct ShadowInstanceData {
    uvec4 ranges;
    vec4 alpha;
    uvec4 options;
};
// Matches the immutable TracingVertex buffer shared by all tracing consumers.
struct ShadowVertex {
    vec4 position;
    vec4 normal;
    vec4 uv;
    vec4 tangent;
};
struct ShadowUniforms {
    mat4 inverseViewProjection;
    vec4 parameters;
    uvec4 counts;
    DrawLightUniforms lights;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(std140, set = 0, binding = 0) uniform ShadowBlock { ShadowUniforms data; } queryData;
layout(std430, set = 0, binding = 1) readonly buffer InstanceBlock { ShadowInstanceData values[]; } instanceData;
layout(std430, set = 0, binding = 2) readonly buffer VertexBlock { ShadowVertex values[]; } vertices;
layout(std430, set = 0, binding = 9) readonly buffer IndexBlock { uint values[]; } indices;
layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevel;
layout(set = 0, binding = 4) uniform texture2D normalImage;
layout(set = 0, binding = 5) uniform texture2D depthImage;
layout(set = 0, binding = 6) uniform sampler nearestSampler;
layout(rgba16f, set = 0, binding = 7) uniform writeonly image2D shadow0;
layout(rgba16f, set = 0, binding = 8) uniform writeonly image2D shadow1;
layout(constant_id = 0) const uint alphaCapacity = 16384u;
layout(set = 0, binding = 16) uniform texture2D alphaImages[alphaCapacity];
layout(set = 0, binding = 24) uniform sampler alphaSamplers[alphaCapacity];
float sampleAlpha(uint index, vec2 uv)
{
    return textureLod(sampler2D(alphaImages[nonuniformEXT(index)], alphaSamplers[nonuniformEXT(index)]), uv, 0.0).a;
}
bool acceptsCandidate(uint instance, uint primitive, vec2 bary)
{
    if (instance >= queryData.data.counts.y)
    {
        return false;
    }
    ShadowInstanceData data = instanceData.values[instance];
    if (data.ranges.z == 0xffffffffu)
    {
        // No alpha test, or a texture omitted at capacity; keep scalar alpha.
        return (data.options.y & 2u) == 0u || data.alpha.w >= data.alpha.z;
    }
    if (primitive >= data.ranges.y)
    {
        return false;
    }
    uint first = data.ranges.x + primitive * 3u;
    vec4 a = vertices.values[data.options.z + indices.values[first]].uv;
    vec4 b = vertices.values[data.options.z + indices.values[first + 1u]].uv;
    vec4 c = vertices.values[data.options.z + indices.values[first + 2u]].uv;
    vec4 interpolated = a * (1.0 - bary.x - bary.y) + b * bary.x + c * bary.y;
    vec2 uv = (data.options.x == 0u ? interpolated.xy : interpolated.zw) * data.alpha.xy;
    return sampleAlpha(data.ranges.z, uv) * data.alpha.w >= data.alpha.z;
}
bool visible(vec3 origin, vec3 direction, float distance)
{
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevel, gl_RayFlagsTerminateOnFirstHitEXT, 0xffu, origin,
        queryData.data.parameters.x, direction, distance);
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT &&
            acceptsCandidate(uint(rayQueryGetIntersectionInstanceIdEXT(query, false)),
                uint(rayQueryGetIntersectionPrimitiveIndexEXT(query, false)), rayQueryGetIntersectionBarycentricsEXT(query, false)))
        {
            rayQueryConfirmIntersectionEXT(query);
        }
    }
    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT;
}
uint shadowHash(uint v)
{
    v ^= v >> 16u;
    v *= 0x7feb352du;
    v ^= v >> 15u;
    v *= 0x846ca68bu;
    return v ^ (v >> 16u);
}
vec3 sampleLightDirection(vec3 direction, float angle, uint index, uint count, uint seed)
{
    vec3 up = abs(direction.z) < 0.99 ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 t = normalize(cross(up, direction));
    vec3 b = cross(direction, t);
    float radius = sqrt((float(index) + 0.5) / float(count)) * tan(min(angle, 1.4));
    float rotation = float(shadowHash(seed)) * (6.28318530718 / 4294967296.0);
    float phase = float(index) * 2.39996322973 + rotation;
    return normalize(direction + radius * (cos(phase) * t + sin(phase) * b));
}
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    uvec2 extent = uvec2(imageSize(shadow0));
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }
    vec4 first = vec4(1.0);
    vec4 second = vec4(1.0);
    vec3 encoded = texelFetch(sampler2D(normalImage, nearestSampler), ivec2(pixel), 0).xyz;
    if (dot(encoded, encoded) > 1e-8)
    {
        vec3 normal = normalize(encoded * 2.0 - 1.0);
        float depth = texelFetch(sampler2D(depthImage, nearestSampler), ivec2(pixel), 0).r;
        vec2 uv = (vec2(pixel) + 0.5) / vec2(extent);
        vec4 world = queryData.data.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
        vec3 position = world.xyz / world.w;
        float bias = queryData.data.parameters.x;
        vec3 origin = position + normal * bias;
        for (uint i = 0u; i < queryData.data.counts.x; ++i)
        {
            DrawLightData light = queryData.data.lights.lights[i];
            float visibility = 1.0;
            if (light.flags.y != 0u)
            {
                vec3 delta = light.flags.x == 4u ? -light.directionAngle.xyz : light.positionIntensity.xyz - origin;
                float distance = light.flags.x == 4u ? queryData.data.parameters.y : length(delta) - bias;
                if (distance > bias && dot(delta, delta) > 1e-12)
                {
                    vec3 direction = normalize(delta);
                    float angle = light.flags.x == 4u ? light.parameters.y * 0.01745329251994 : atan(light.parameters.y / max(distance, bias));
                    uint count = min(max(light.flags.z, 1u), queryData.data.counts.w);
                    visibility = 0.0;
                    uint seed = pixel.x + pixel.y * extent.x + i * 104729u;
                    for (uint sampleIndex = 0u; sampleIndex < count; ++sampleIndex)
                    {
                        vec3 sampleDirection = count == 1u ? direction : sampleLightDirection(direction, angle, sampleIndex, count, seed);
                        visibility += visible(origin, sampleDirection, distance) ? 1.0 : 0.0;
                    }
                    visibility /= float(count);
                }
            }
            if (i < 4u)
            {
                first[i] = visibility;
            }
            else
            {
                second[i - 4u] = visibility;
            }
        }
    }
    imageStore(shadow0, ivec2(pixel), first);
    imageStore(shadow1, ivec2(pixel), second);
}
