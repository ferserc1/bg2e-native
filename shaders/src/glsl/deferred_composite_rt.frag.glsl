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

#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"
#include "lib/color_correction.glsl"
#include "lib/deferred_utils.glsl"
#include "lib/ray_tracing.glsl"

// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Material;
layout(set = 0, binding = 3) uniform sampler2D g_FresnelFlags;
layout(set = 0, binding = 4) uniform sampler2D g_SheenColor;
layout(set = 0, binding = 5) uniform sampler2D g_InputImage;
layout(set = 0, binding = 6) uniform sampler2D g_Depth;
// binding 7: scalar AO (RTAO mode) or HDR GI irradiance (RTGI mode), selected by indirectMode
layout(set = 0, binding = 7) uniform sampler2D g_Indirect;
layout(set = 0, binding = 8) uniform sampler2D g_RTReflection;

// Scene data (set=1)
layout(set = 1, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

// Environment data (set=2)
layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;
layout(set = 2, binding = 3) uniform EnvironmentData {
    float maxReflectionLOD;
} environmentData;

// Light data (set=3)
layout(std430, set = 3, binding = 0) readonly buffer LightBuffer {
    LightData lights[];
} LightsBuffer;

// RT TLAS (set=4)
layout(set = 4, binding = 0) uniform accelerationStructureEXT tlas;

// Push constants
layout(push_constant) uniform PushConstant {
    float gamma;
    float brightness;
    float contrast;
    float exposure;

    uint lightCount;
    uint indirectMode;   // 0 = RTAO (scalar AO), 1 = RTGI (HDR irradiance)
    uint padding2;
    uint padding3;

    mat4 inverseViewProjection;
} pushConstant;

layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

// Ambient lighting with RT reflections and AO scalar factor.
vec3 calcAmbientLightWithReflections(
    vec3 viewDir,
    vec3 normal,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness,
    samplerCube inIrradianceMap,
    samplerCube inPrefilteredEnvMap,
    float maxLOD,
    sampler2D inBrdfLUT,
    float ambientOcclussion,
    float sheenIntensity,
    vec3 sheenColor,
    vec4 rtReflection
) {
    vec3 R = reflect(-viewDir, normal);

    vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);

    vec3 Ks = F;
    vec3 Kd = 1.0 - Ks;
    Kd *= 1.0 - metallic;

    vec3 irradiance = texture(inIrradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    float roughnessDelta = 1.0 / maxLOD;
    float sampleRoughness = roughness * maxLOD;
    vec3 prefilteredColor1 = textureLod(inPrefilteredEnvMap, R, sampleRoughness).rgb;
    vec3 prefilteredColor2 = textureLod(inPrefilteredEnvMap, R, max(sampleRoughness - roughnessDelta, 0.0)).rgb;
    vec3 envReflection = mix(prefilteredColor1, prefilteredColor2, fract(sampleRoughness));

    vec3 finalReflection = mix(envReflection, rtReflection.rgb, rtReflection.a);

    vec2 brdfUV = vec2(clamp(max(dot(normal, viewDir), 0.0), 0.01, 0.99), roughness);
    vec2 envBRDF = texture(inBrdfLUT, brdfUV).rg;

    vec3 specular = finalReflection * (F * envBRDF.x + envBRDF.y);

    vec3 base = (Kd * diffuse + specular) * ambientOcclussion;
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;

    return base + sheen;
}

// Ambient lighting with RTGI irradiance.
// giIrradiance is the pre-computed indirect irradiance arriving at the surface
// (without the primary surface albedo applied — that is done here).
vec3 calcAmbientLightWithGI(
    vec3 viewDir,
    vec3 normal,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness,
    samplerCube inPrefilteredEnvMap,
    float maxLOD,
    sampler2D inBrdfLUT,
    float sheenIntensity,
    vec3 sheenColor,
    vec3 giIrradiance,
    vec4 rtReflection
) {
    vec3 R = reflect(-viewDir, normal);

    vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);

    vec3 Ks = F;
    vec3 Kd = 1.0 - Ks;
    Kd *= 1.0 - metallic;

    // GI irradiance already encodes occlusion (dark areas = blocked paths).
    // Apply albedo here so that color bleeding from bounced surfaces shows through.
    vec3 diffuse = Kd * giIrradiance * albedo;

    // Specular from the environment map + RT reflections (unchanged from RTAO path).
    float roughnessDelta = 1.0 / maxLOD;
    float sampleRoughness = roughness * maxLOD;
    vec3 prefilteredColor1 = textureLod(inPrefilteredEnvMap, R, sampleRoughness).rgb;
    vec3 prefilteredColor2 = textureLod(inPrefilteredEnvMap, R, max(sampleRoughness - roughnessDelta, 0.0)).rgb;
    vec3 envReflection = mix(prefilteredColor1, prefilteredColor2, fract(sampleRoughness));

    vec3 finalReflection = mix(envReflection, rtReflection.rgb, rtReflection.a);

    vec2 brdfUV = vec2(clamp(max(dot(normal, viewDir), 0.0), 0.01, 0.99), roughness);
    vec2 envBRDF = texture(inBrdfLUT, brdfUV).rg;

    vec3 specular = finalReflection * (F * envBRDF.x + envBRDF.y);

    vec3 base = diffuse + specular;
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity);

    return base + sheen;
}

void main() {
    DeferredGBufferData gbuf = setupDeferredGBuffer(
        g_Albedo, g_Normal, g_Material, g_FresnelFlags, g_SheenColor,
        g_InputImage, g_Depth,
        vTexcoord,
        pushConstant.inverseViewProjection,
        sceneData.viewMatrix
    );

    if (gbuf.isEmpty) {
        outColor = gbuf.inputColor;
        outColor.a = 0.0;
        return;
    }

    if (gbuf.unlit) {
        outColor = fragmentShaderOutput(
            vec4(gbuf.albedo.rgb, 1.0),
            pushConstant.exposure,
            pushConstant.gamma,
            pushConstant.brightness,
            pushConstant.contrast
        );
        return;
    }

    // Select AO value for the direct-lighting sheen term.
    // In RTAO mode: combine material AO with the RT AO texture.
    // In RTGI mode: use only baked material AO (g_Indirect contains GI irradiance, not AO).
    float aoForDirectLight = (pushConstant.indirectMode == 0u)
        ? gbuf.ao * texture(g_Indirect, vTexcoord).r
        : gbuf.ao;

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pushConstant.lightCount; i++) {
        if (LightsBuffer.lights[i].type != LIGHT_TYPE_DISABLED) {
            float shadowFactor = 1.0;
            if (LightsBuffer.lights[i].castShadows != 0) {
                shadowFactor = queryShadow(tlas, gbuf.worldPos, gbuf.normal, LightsBuffer.lights[i], 32);
            }
            Lo += shadowFactor * calcRadiance(LightsBuffer.lights[i], gbuf.viewDir, gbuf.worldPos,
                              gbuf.metallic, gbuf.roughness,
                              gbuf.F0, gbuf.normal, gbuf.albedo.rgb,
                              gbuf.sheenIntensity, gbuf.sheenColor, aoForDirectLight);
        }
    }

    vec4 rtReflection = texture(g_RTReflection, vTexcoord);

    vec3 ambient;
    if (pushConstant.indirectMode == 0u) {
        // RTAO: g_Indirect is a scalar AO texture (R channel)
        float ao = gbuf.ao * texture(g_Indirect, vTexcoord).r;
        ambient = calcAmbientLightWithReflections(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                        gbuf.metallic, gbuf.roughness,
                                        irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                        brdfLUT, ao, gbuf.sheenIntensity, gbuf.sheenColor,
                                        rtReflection);
    } else {
        // RTGI: g_Indirect is HDR irradiance (RGB channels), albedo is applied inside calcAmbientLightWithGI
        vec3 giIrradiance = texture(g_Indirect, vTexcoord).rgb;
        ambient = calcAmbientLightWithGI(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                        gbuf.metallic, gbuf.roughness,
                                        prefilteredEnvMap, environmentData.maxReflectionLOD,
                                        brdfLUT, gbuf.sheenIntensity, gbuf.sheenColor,
                                        giIrradiance, rtReflection);
    }

    // Light emission: albedo-colored glow from G-buffer
    vec3 emissionColor = gbuf.albedo.rgb * gbuf.lightEmission;

    // Refraction on translucent fragments (per-material factor from the G-buffer).
    vec4 background = applyRefraction(
        sceneData.viewMatrix, gbuf.normal, vTexcoord,
        gbuf.refractionFactor, gbuf.albedo, g_InputImage
    );

    outColor = compositeFinalColor(ambient + emissionColor, Lo, background, gbuf.albedo.a,
                                   pushConstant.exposure, pushConstant.gamma,
                                   pushConstant.brightness, pushConstant.contrast);
}
