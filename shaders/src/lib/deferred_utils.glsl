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

#ifndef DEFERRED_UTILS_GLSL
#define DEFERRED_UTILS_GLSL

#include "uniforms.glsl"
#include "color_correction.glsl"

struct DeferredGBufferData {
    vec4 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    float ao;
    float sheenIntensity;
    vec3 worldPos;
    vec3 viewDir;
    vec3 F0;
    vec3 sheenColor;
    vec4 inputColor;
    bool isEmpty;
    vec3 fresnelTint;
    bool unlit;
};

vec3 reconstructWorldPosition(
    vec2 uv,
    float depth,
    mat4 inverseViewProjection
) {
    // We invert the V coordinate because in Vulkan, screen coordinates
    // are vertically inverted. We need to perform vertical sampling
    // because, to reconstruct the position using the inverse viewProjection matrix,
    // those coordinates will be screen coordinates
    vec2 ndc = vec2(
        uv.x * 2.0 - 1.0,
        (1.0 - uv.y) * 2.0 - 1.0
    );
    vec4 clipPosition = vec4(
        ndc.x,
        ndc.y,
        depth,
        1.0
    );

    vec4 worldPosition = inverseViewProjection * clipPosition;
    worldPosition /= worldPosition.w;

    return worldPosition.xyz;
}

DeferredGBufferData setupDeferredGBuffer(
    sampler2D g_Albedo, sampler2D g_Normal, sampler2D g_Material, sampler2D g_FresnelFlags, sampler2D g_SheenColor,
    sampler2D g_InputImage, sampler2D g_Depth,
    vec2 vTexcoord, mat4 inverseViewProjection,
    mat4 viewMatrix
) {
    DeferredGBufferData gbuf;

    gbuf.albedo = texture(g_Albedo, vTexcoord);
    gbuf.normal = texture(g_Normal, vTexcoord).xyz * 2.0 - 1.0;
    vec4 materialData = texture(g_Material, vTexcoord);
    gbuf.inputColor = texture(g_InputImage, vTexcoord);
    vec4 fresnelFlags = texture(g_FresnelFlags, vTexcoord);
    gbuf.fresnelTint = fresnelFlags.rgb;
    uint materialFlags = uint(round(fresnelFlags.a * 255.0));
    gbuf.unlit = (materialFlags & MATERIAL_FLAG_UNLIT) != 0u;

    if (gbuf.albedo.a == 0) {
        gbuf.isEmpty = true;
        return gbuf;
    }

    gbuf.isEmpty = false;

    float depth = texture(g_Depth, vTexcoord).r;
    gbuf.worldPos = reconstructWorldPosition(
        vTexcoord,
        depth,
        inverseViewProjection
    );

    gbuf.metallic = materialData.r;
    gbuf.roughness = max(materialData.g, 0.05);
    gbuf.ao = materialData.b;
    gbuf.sheenIntensity = materialData.a;

    vec3 cameraPos = vec3(inverse(viewMatrix)[3]);
    gbuf.viewDir = normalize(cameraPos - gbuf.worldPos);

    gbuf.F0 = mix(vec3(0.04), gbuf.albedo.rgb, gbuf.metallic);

    gbuf.sheenColor = texture(g_SheenColor, vTexcoord).rgb;

    return gbuf;
}

vec4 compositeFinalColor(
    vec3 ambient, vec3 directLighting,
    vec4 inputColor, float albedoAlpha,
    float inExposure, float gamma,
    float brightness, float contrast
) {
    vec3 color = ambient + directLighting;

    vec3 finalColor = exposure(color, inExposure);
    vec4 outColor = lineal2SRGB(vec4(finalColor, 1.0), gamma);
    outColor = brightnessContrast(outColor, brightness, contrast);

    outColor = mix(inputColor, outColor, albedoAlpha);

    return outColor;
}

uint hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float rand(inout uint seed)
{
    seed = hash(seed);
    return float(seed) / float(0xffffffffu);
}

mat3 buildTBN(vec3 normal)
{
    vec3 up = abs(normal.x) < 0.999
        ? vec3(0.0, 0.0, 1.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return mat3(tangent, bitangent, normal);
}

vec3 randomHemisphereDirection(vec3 normal, inout uint seed)
{
    float u1 = rand(seed);
    float u2 = rand(seed);

    float r = sqrt(u1);
    float phi = 6.28318530718 * u2;

    vec3 localDir = vec3(
        r * cos(phi),
        r * sin(phi),
        sqrt(max(0.0, 1.0 - u1))
    );

    return normalize(buildTBN(normal) * localDir);
}

#endif