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
 
#ifndef UNIFORMS_GLSL
#define UNIFORMS_GLSL

#include "color_correction.glsl"

struct PBRMaterialData
{
    vec4 albedo;
    vec4 fresnelTint;

    vec2 albedoScale;
    vec2 normalScale;
    vec2 metalnessScale;
    vec2 roughnessScale;

    float metalness;
    float roughness;

    int albedoUVSet;
    int normalUVSet;
    int metalnessUVSet;
    int roughnessUVSet;
    int aoUVSet;
    
    float sheenIntensity;
    vec4 sheenColor;

    uint unlit;
    float refractionFactor;

    // Channel selection for single-channel textures
    int metalnessChannel;
    int roughnessChannel;
    int aoChannel;

    // Channel value inversion (reuses the former 2-uint padding)
    int metalnessInvert;
    int roughnessInvert;

    // Light emission
    float lightEmission;
    vec2 lightEmissionScale;
    int lightEmissionChannel;
    int lightEmissionInvert;
    int lightEmissionUVSet;

    uint padding;
};

const uint MATERIAL_FLAG_UNLIT          = 1u << 0;

struct LightData
{
    vec3 position;
    float intensity;
    vec4 color;
    vec3 direction;
    int type;
    float spotAngle;
    float spotCutoff;
    int castShadows;

    float sourceSize;
    int shadowSamples;

    int affectsReflections;
    int padding1;
    int padding2;
};

const int LIGHT_COUNT = 8;
const int LIGHT_TYPE_POINT = 5;
const int LIGHT_TYPE_DIRECTIONAL = 4;
const int LIGHT_TYPE_SPOT = 1;
const int LIGHT_TYPE_DISABLED = 10;

vec4 sampleAlbedo(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat, float gamma)
{
    vec2 uv[2] = { uv0, uv1 };
    return SRGB2Lineal(texture(tex, uv[mat.albedoUVSet] * mat.albedoScale), gamma) * mat.albedo;
}

float sampleMetallic(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat)
{
    vec2 uv[2] = { uv0, uv1 };
    float value = texture(tex, uv[mat.metalnessUVSet] * mat.metalnessScale)[mat.metalnessChannel];
    if (mat.metalnessInvert != 0)
    {
        value = 1.0 - value;
    }
    return value * mat.metalness;
}

float sampleRoughness(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat)
{
    const float minRoughness = 0.05; // Minimum roughness value: even a mirror have some roughness
    vec2 uv[2] = { uv0, uv1 };
    float value = texture(tex, uv[mat.roughnessUVSet] * mat.roughnessScale)[mat.roughnessChannel];
    if (mat.roughnessInvert != 0)
    {
        value = 1.0 - value;
    }
    return max(value * mat.roughness, minRoughness);
}

vec3 sampleNormal(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat, mat3 TBN)
{
    vec2 uv[2] = { uv0, uv1 };
    vec3 normal = texture(tex, uv[mat.normalUVSet] * mat.normalScale).xyz * 2.0 - 1.0;
    return normalize(TBN * normal);
}

float sampleAmbientOcclussion(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat)
{
    vec2 uv[2] = { uv0, uv1 };
    return texture(tex, uv[mat.aoUVSet])[mat.aoChannel];
}

float sampleLightEmission(sampler2D tex, vec2 uv0, vec2 uv1, PBRMaterialData mat)
{
    vec2 uv[2] = { uv0, uv1 };
    float value = texture(tex, uv[mat.lightEmissionUVSet] * mat.lightEmissionScale)[mat.lightEmissionChannel];
    if (mat.lightEmissionInvert != 0)
    {
        value = 1.0 - value;
    }
    return value * mat.lightEmission;
}

vec3 calcF0(vec3 albedo, PBRMaterialData mat)
{
    return mix(vec3(0.04), albedo, mat.metalness) * mat.fresnelTint.rgb;
}

vec3 calcSheen(vec3 normal, vec3 viewDir, vec3 sheenColor, float sheenIntensity)
{
    float NdotV = max(dot(normal, viewDir), 0.0);
    float facing = 1.0 - NdotV;
    
    // Adjustable power: controls how concentrated the brightness is
    float falloff = pow(facing, 5.0);
    return sheenColor * falloff * sheenIntensity;
}

#endif
