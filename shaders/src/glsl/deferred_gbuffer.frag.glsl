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
#include "lib/normal_map.glsl"

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicTex;
layout(set = 1, binding = 4) uniform sampler2D roughnessTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;
layout(set = 1, binding = 6) uniform sampler2D lightEmissionTex;

layout(location = 0) out vec4 g_Albedo;
layout(location = 1) out vec4 g_Normal;
layout(location = 2) out vec4 g_Material;
layout(location = 3) out vec4 g_FresnelColorFlags;
layout(location = 4) out vec4 g_SheenColor;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in mat3 inTBN;

void main() {
    PBRMaterialData mat = objectData.material;

    // Albedo (sRGB to linear)
    vec4 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, mat, 2.2);
    g_Albedo = albedo;

    // Normal (world space, mapped to 0-1)
    vec3 normal = sampleNormal(normalTex, inUV0, inUV1, mat, inTBN);
    float emission = sampleLightEmission(lightEmissionTex, inUV0, inUV1, mat);
    g_Normal = vec4(normal * 0.5 + 0.5, emission);

    // Material properties
    float metallic = sampleMetallic(metallicTex, inUV0, inUV1, mat);
    float roughness = sampleRoughness(roughnessTex, inUV0, inUV1, mat);
    float ao = sampleAmbientOcclussion(aoTex, inUV0, inUV1, mat);
    float sheen = mat.sheenIntensity;
    g_Material = vec4(metallic, roughness, ao, sheen);

    // Fresnel color + flags
    float unlitFlag = (mat.unlit & MATERIAL_FLAG_UNLIT) != 0u ? 1.0 : 0.0;
    g_FresnelColorFlags = vec4(mat.fresnelTint.rgb, unlitFlag);

    // Sheen color (RGB), refraction factor packed in the reserved alpha channel
    g_SheenColor = vec4(mat.sheenColor.rgb, mat.refractionFactor);
}
