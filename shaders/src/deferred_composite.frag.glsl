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

#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"
#include "lib/color_correction.glsl"
#include "lib/deferred_utils.glsl"

// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Material;
layout(set = 0, binding = 3) uniform sampler2D g_InputImage;
layout(set = 0, binding = 4) uniform sampler2D g_Depth;

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

// Push constants
layout(push_constant) uniform PushConstant {
    float gamma;
    float brightness;
    float contrast;
    float exposure;

    uint lightCount;

    uint padding1;
    uint padding2;
    uint padding3;

    mat4 inverseViewProjection;
} pushConstant;


layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample G-buffers
    vec4 albedo = texture(g_Albedo, vTexcoord);
    vec3 normal = texture(g_Normal, vTexcoord).xyz * 2.0 - 1.0;  // Map [0,1] to [-1,1]
    vec4 materialData = texture(g_Material, vTexcoord);
    vec4 inputColor = texture(g_InputImage, vTexcoord);

    float depth = texture(g_Depth, vTexcoord).r;
    vec3 worldPos = reconstructWorldPosition(
        vTexcoord,
        depth,
        pushConstant.inverseViewProjection
    );

    float metallic = materialData.r;
    float roughness = max(materialData.g, 0.05);
    float ao = materialData.b;
    float sheenIntensity = materialData.a;

    // Camera position from inverse view matrix
    vec3 cameraPos = vec3(inverse(sceneData.viewMatrix)[3]);
    vec3 viewDir = normalize(cameraPos - worldPos);

    // F0 for Fresnel
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 sheenColor = albedo.rgb;

    // Direct lighting loop
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pushConstant.lightCount; i++) {
        if (LightsBuffer.lights[i].type == LIGHT_TYPE_DISABLED) continue;
        Lo += calcRadiance(LightsBuffer.lights[i], viewDir, worldPos, metallic, roughness,
                          F0, normal, albedo.rgb, sheenIntensity, sheenColor, ao);
    }

    // Ambient/IBL lighting
    vec3 ambient = calcAmbientLight(viewDir, normal, F0, albedo.rgb, metallic, roughness,
                                    irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                    brdfLUT, ao, sheenIntensity, sheenColor);

    vec3 color = ambient + Lo;

    // Blend with input image (previous layer) using alpha
    vec3 finalColor = mix(inputColor.rgb, color, albedo.a);

    // Color correction
    finalColor = exposure(finalColor, pushConstant.exposure);
    outColor = lineal2SRGB(vec4(finalColor, 1.0), pushConstant.gamma);
    outColor = brightnessContrast(outColor, pushConstant.brightness, pushConstant.contrast);
}
