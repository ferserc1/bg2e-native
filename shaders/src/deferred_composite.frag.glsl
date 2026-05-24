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
layout(set = 0, binding = 3) uniform sampler2D g_FresnelFlags;
layout(set = 0, binding = 4) uniform sampler2D g_SheenColor;
layout(set = 0, binding = 5) uniform sampler2D g_InputImage;
layout(set = 0, binding = 6) uniform sampler2D g_Depth;
layout(set = 0, binding = 7) uniform sampler2D g_AO;

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

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pushConstant.lightCount; i++) {
        if (LightsBuffer.lights[i].type == LIGHT_TYPE_DISABLED) continue;
        Lo += calcRadiance(LightsBuffer.lights[i], gbuf.viewDir, gbuf.worldPos,
                          gbuf.metallic, gbuf.roughness,
                          gbuf.F0, gbuf.normal, gbuf.albedo.rgb,
                          gbuf.sheenIntensity, gbuf.sheenColor, gbuf.ao);
    }

    vec3 ambient = calcAmbientLight(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                    gbuf.metallic, gbuf.roughness,
                                    irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                    brdfLUT, gbuf.ao * texture(g_AO, vTexcoord).r, gbuf.sheenIntensity, gbuf.sheenColor);

    outColor = compositeFinalColor(ambient, Lo, gbuf.inputColor, gbuf.albedo.a,
                                   pushConstant.exposure, pushConstant.gamma,
                                   pushConstant.brightness, pushConstant.contrast);
}
