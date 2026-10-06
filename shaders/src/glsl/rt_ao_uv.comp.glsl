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
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "lib/deferred_utils.glsl"
#include "lib/ray_tracing.glsl"
#define RT_MATERIAL_SET 1
#include "lib/rt_material_data.glsl"

layout(set = 0, binding = 0) uniform sampler2D uvWorldPosition;
layout(set = 0, binding = 1) uniform sampler2D uvWorldNormal;
layout(set = 0, binding = 2) uniform usampler2D uvValidMask;
layout(set = 0, binding = 3) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 4, r8) uniform image2D aoOutput;

layout(push_constant) uniform PushConstant {
    int sampleCount;
    float maxRayDistance;
    float bias;
    float falloff;
    uint frameIndex;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

vec3 uvRandomHemisphereDirection(vec3 normal, inout uint seed)
{
    float u1 = rand(seed);
    float u2 = rand(seed);
    float radius = sqrt(u1);
    float phi = 6.28318530718 * u2;
    vec3 localDirection = vec3(
        radius * cos(phi),
        radius * sin(phi),
        sqrt(max(0.0, 1.0 - u1))
    );

    // Choose a reference axis that cannot be parallel to the surface normal.
    vec3 reference = abs(normal.z) < 0.999
        ? vec3(0.0, 0.0, 1.0)
        : vec3(0.0, 1.0, 0.0);
    vec3 tangent = normalize(cross(reference, normal));
    vec3 bitangent = cross(normal, tangent);
    return normalize(mat3(tangent, bitangent, normal) * localDirection);
}

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 extent = imageSize(aoOutput);
    if (pixelCoord.x >= extent.x || pixelCoord.y >= extent.y)
    {
        return;
    }

    if (texelFetch(uvValidMask, pixelCoord, 0).r == 0u)
    {
        imageStore(aoOutput, pixelCoord, vec4(1.0, 0.0, 0.0, 0.0));
        return;
    }
    vec3 worldPosition = texelFetch(uvWorldPosition, pixelCoord, 0).xyz;
    vec3 worldNormal = texelFetch(uvWorldNormal, pixelCoord, 0).xyz;
    float normalLengthSquared = dot(worldNormal, worldNormal);
    if (normalLengthSquared <= 1.0e-12)
    {
        imageStore(aoOutput, pixelCoord, vec4(1.0, 0.0, 0.0, 0.0));
        return;
    }
    worldNormal *= inversesqrt(normalLengthSquared);

    uint seed = uint(pixelCoord.x) * 1973u ^
                uint(pixelCoord.y) * 9277u ^
                pc.frameIndex * 26699u;
    float occlusion = 0.0;
    for (int sampleIndex = 0; sampleIndex < pc.sampleCount; ++sampleIndex)
    {
        vec3 rayDirection = uvRandomHemisphereDirection(worldNormal, seed);
        float hitDistance;
        if (queryAOCutout(tlas, worldPosition, worldNormal, rayDirection,
                          pc.maxRayDistance, pc.bias, hitDistance))
        {
            float distanceFactor = 1.0 - clamp(hitDistance / pc.maxRayDistance, 0.0, 1.0);
            occlusion += pow(distanceFactor, pc.falloff);
        }
    }

    float visibility = 1.0 - clamp(occlusion / float(pc.sampleCount), 0.0, 1.0);
    imageStore(aoOutput, pixelCoord, vec4(visibility, 0.0, 0.0, 0.0));
}
