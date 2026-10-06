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

#include "lib/basic_lighting.glsl"
#include "lib/ray_tracing.glsl"
#define RT_MATERIAL_SET 1
#include "lib/rt_material_data.glsl"

layout(set = 0, binding = 0) uniform sampler2D uvWorldPosition;
layout(set = 0, binding = 1) uniform sampler2D uvWorldNormal;
layout(set = 0, binding = 2) uniform usampler2D uvValidMask;
layout(set = 0, binding = 3) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 4, r8) uniform image2D shadowOutput;

layout(std430, set = 2, binding = 0) readonly buffer SceneLightBuffer {
    LightData lights[];
} sceneLights;

layout(push_constant) uniform PushConstant {
    uint lightCount;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 extent = imageSize(shadowOutput);
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }
    if (texelFetch(uvValidMask, pixel, 0).r == 0u)
    {
        imageStore(shadowOutput, pixel, vec4(1.0, 0.0, 0.0, 0.0));
        return;
    }

    vec3 worldPosition = texelFetch(uvWorldPosition, pixel, 0).xyz;
    vec3 worldNormal = texelFetch(uvWorldNormal, pixel, 0).xyz;
    float normalLengthSquared = dot(worldNormal, worldNormal);
    if (normalLengthSquared <= 1.0e-12)
    {
        imageStore(shadowOutput, pixel, vec4(1.0, 0.0, 0.0, 0.0));
        return;
    }
    worldNormal *= inversesqrt(normalLengthSquared);

    float weightedVisibility = 0.0;
    float totalWeight = 0.0;
    for (uint lightIndex = 0u; lightIndex < pc.lightCount; ++lightIndex)
    {
        LightData light = sceneLights.lights[lightIndex];
        if (light.type == LIGHT_TYPE_DISABLED || light.castShadows == 0)
        {
            continue;
        }

        vec3 unshadowedDirect = computeBasicLighting(light, worldPosition, worldNormal, vec3(1.0));
        float weight = dot(max(unshadowedDirect, vec3(0.0)), vec3(0.2126, 0.7152, 0.0722));
        if (weight <= 1.0e-8)
        {
            continue;
        }

        float visibility = queryShadowCutout(tlas, worldPosition, worldNormal, light, 32);
        weightedVisibility += weight * visibility;
        totalWeight += weight;
    }

    // Visibility is averaged by each light's unshadowed Lambert radiance,
    // intensity, color and distance attenuation. With no contributing shadow-
    // casting light the independent shadow layer is neutral.
    float visibility = totalWeight > 1.0e-8 ? weightedVisibility / totalWeight : 1.0;
    imageStore(shadowOutput, pixel, vec4(visibility, 0.0, 0.0, 0.0));
}
