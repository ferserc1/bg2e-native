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

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/pbr.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inFragPos;
layout(location = 5) in mat3 inTBN;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicTex;
layout(set = 1, binding = 4) uniform sampler2D roughnessTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;

layout(push_constant) uniform PushConstant
{
    float gamma;
    float brightness;
    float contrast;
    float exposure;
} pushConstant;

layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;
layout(set = 2, binding = 3) uniform EnvironmentData {
    float maxReflectionLOD;
} environmentData;

layout (set = 3, binding = 0) uniform LightsBuffer {
    LightData lights[LIGHT_COUNT];
    int lightCount;
    vec3 padding;
} lightsBuffer;

layout(set = 4, binding = 0) uniform accelerationStructureEXT u_TLAS;

void main()
{
    PBRMaterialData mat = objectData.material;
    vec3 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, mat, 2.2).rgb;
    float metallic = sampleMetallic(metallicTex, inUV0, inUV1, mat);
    float roughness = sampleRoughness(roughnessTex, inUV0, inUV1, mat);
    float ambientOcclussion = sampleAmbientOcclussion(aoTex, inUV0, inUV1, mat);
    vec3 normal = sampleNormal(normalTex, inUV0, inUV1, mat, inTBN);

    bool unlit = (objectData.material.unlit & MATERIAL_FLAG_UNLIT) != 0u;

    if (unlit)
    {
        outColor = fragmentShaderOutput(
            vec4(albedo, 1.0),
            pushConstant.exposure,
            pushConstant.gamma,
            pushConstant.brightness,
            pushConstant.contrast
        );
        return;
    }

    vec3 viewDir = normalize(inViewPos - inFragPos);
    vec3 F0 = calcF0(albedo, mat);

    vec3 Lo = vec3(0.0);
    int lightCount = lightsBuffer.lightCount;
    for(int i = 0; i < lightCount; ++i)
    {
        float tMax = 10000000.0;
        vec3 dir = normalize(-lightsBuffer.lights[i].direction);
        if (lightsBuffer.lights[i].type == LIGHT_TYPE_POINT)
        {
            vec3 toLight = lightsBuffer.lights[i].position - inFragPos.xyz;
            tMax = length(toLight);
            dir = normalize(toLight);
        }
        if (lightsBuffer.lights[i].castShadows == 0)
        {
            Lo += calcRadiance(lightsBuffer.lights[i], viewDir, inFragPos, metallic, roughness, F0, normal, albedo, mat.sheenIntensity, mat.sheenColor.rgb, ambientOcclussion);
            continue;
        }
        vec3 origin = inFragPos.xyz + normal.xyz * 0.01;
        rayQueryEXT rq;
        rayQueryInitializeEXT(
            rq,
            u_TLAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            origin,
            0.001,
            dir,
            tMax
        );
        while (rayQueryProceedEXT(rq))
        {
        }

        if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT)
        {
            Lo += calcRadiance(lightsBuffer.lights[i], viewDir, inFragPos, metallic, roughness, F0, normal, albedo, mat.sheenIntensity, mat.sheenColor.rgb, ambientOcclussion);
        }
    }

    vec3 ambient = calcAmbientLight(
        viewDir, normal, F0,
        albedo, metallic, roughness,
        irradianceMap, prefilteredEnvMap,
        environmentData.maxReflectionLOD,
        brdfLUT, ambientOcclussion,
        mat.sheenIntensity, mat.sheenColor.rgb
    );

    vec3 color = ambient + Lo;

    outColor = fragmentShaderOutput(
        vec4(color, 1.0),
        pushConstant.exposure,
        pushConstant.gamma,
        pushConstant.brightness,
        pushConstant.contrast
    );
}
