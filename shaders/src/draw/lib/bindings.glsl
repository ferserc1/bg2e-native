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

#ifndef DRAW_BINDINGS_GLSL
#define DRAW_BINDINGS_GLSL
struct DrawMaterialData {
    vec4 albedo;
    vec4 fresnelTint;
    vec4 sheenColor;
    vec4 factors;
    vec4 emissionAlpha;
    vec4 uvScale[3];
    uvec4 uvSets[2];
    uvec4 channels;
    uvec4 invert;
};
struct DrawFrameData {
    mat4 view;
    mat4 projection;
    mat4 inverseViewProjection;
    vec4 cameraPosition;
    vec4 extentAndDelta;
    uvec4 frameInfo;
};
struct DrawObjectData {
    mat4 model;
    mat4 normal;
    DrawMaterialData material;
    uvec4 identity;
};
#include "solar.glsl"

struct DrawLightData {
    vec4 positionIntensity;
    vec4 color;
    vec4 directionAngle;
    vec4 parameters;
    uvec4 flags;
};
DrawLightData resolveSolarLight(DrawLightData light)
{
    if (light.flags.x == 6u)
    {
        if (light.parameters.w > 0.5)
        {
            light.positionIntensity.w *= solarVisibility(-light.directionAngle.y, light.parameters.y, light.parameters.z);
        }
        light.parameters.y = solarAngularRadius(light.parameters.y, light.parameters.z) * 57.29577951308232;
        light.flags.x = 4u;
    }
    return light;
}

struct DrawLightUniforms {
    uvec4 count;
    DrawLightData lights[8];
};
struct DrawEnvironmentData {
    vec4 parameters;
};
vec2 drawSelectUV(DrawMaterialData m, uint slot, vec2 uv0, vec2 uv1)
{
    uint index = 0u;
    vec2 scale = vec2(1.0);
    if (slot == 0u)
    {
        index = m.uvSets[0].x;
        scale = m.uvScale[0].xy;
    }
    else if (slot == 1u)
    {
        index = m.uvSets[0].y;
        scale = m.uvScale[1].xy;
    }
    else if (slot == 2u)
    {
        index = m.uvSets[0].z;
        scale = m.uvScale[1].zw;
    }
    else if (slot == 3u)
    {
        index = m.uvSets[0].w;
        scale = m.uvScale[0].zw;
    }
    else if (slot == 4u)
    {
        index = m.uvSets[1].x;
        scale = m.uvScale[2].xy;
    }
    else if (slot == 5u)
    {
        index = m.uvSets[1].y;
        scale = m.uvScale[2].zw;
    }
    return (index == 0u ? uv0 : uv1) * scale;
}
vec4 drawDecodeSample(DrawMaterialData m, uint slot, vec4 sampleValue)
{
    if ((m.uvSets[1].w & (1u << slot)) != 0u)
    {
        vec3 c = max(sampleValue.rgb, vec3(0.0));
        vec3 low = c / 12.92;
        vec3 high = pow((c + 0.055) / 1.055, vec3(2.4));
        sampleValue.r = c.r <= 0.04045 ? low.r : high.r;
        sampleValue.g = c.g <= 0.04045 ? low.g : high.g;
        sampleValue.b = c.b <= 0.04045 ? low.b : high.b;
    }
    return sampleValue;
}
float drawScalarSample(vec4 value, uint channel, uint invertValue)
{
    float result = value[channel];
    return invertValue != 0u ? 1.0 - result : result;
}

#if defined(DRAW_BINDINGS_VERTEX)
layout(std140, set = 0, binding = 0) uniform DrawFrameBlock
{
    DrawFrameData data;
} drawFrame;
layout(std140, set = 1, binding = 0) uniform DrawObjectBlock
{
    DrawObjectData data;
} drawObject;
#elif defined(DRAW_BINDINGS_FRAGMENT)
layout(std140, set = 0, binding = 1) uniform DrawFrameBlock
{
    DrawFrameData data;
} drawFrame;
layout(std140, set = 1, binding = 1) uniform DrawObjectBlock
{
    DrawObjectData data;
} drawObject;
layout(std140, set = 2, binding = 0) uniform DrawLightBlock
{
    DrawLightUniforms data;
} drawLights;
layout(std140, set = 3, binding = 8) uniform DrawEnvironmentBlock
{
    DrawEnvironmentData data;
} drawEnvironment;
layout(set = 1, binding = 2) uniform texture2D drawAlbedo;
layout(set = 1, binding = 8) uniform sampler drawAlbedoSampler;
layout(set = 1, binding = 3) uniform texture2D drawMetalness;
layout(set = 1, binding = 9) uniform sampler drawMetalnessSampler;
layout(set = 1, binding = 4) uniform texture2D drawRoughness;
layout(set = 1, binding = 10) uniform sampler drawRoughnessSampler;
layout(set = 1, binding = 5) uniform texture2D drawNormal;
layout(set = 1, binding = 11) uniform sampler drawNormalSampler;
layout(set = 1, binding = 6) uniform texture2D drawAO;
layout(set = 1, binding = 12) uniform sampler drawAOSampler;
layout(set = 1, binding = 7) uniform texture2D drawEmission;
layout(set = 1, binding = 13) uniform sampler drawEmissionSampler;
layout(set = 3, binding = 0) uniform textureCube drawSky;
layout(set = 3, binding = 4) uniform sampler drawSkySampler;
layout(set = 3, binding = 1) uniform textureCube drawIrradiance;
layout(set = 3, binding = 5) uniform sampler drawIrradianceSampler;
layout(set = 3, binding = 2) uniform textureCube drawPrefilter;
layout(set = 3, binding = 6) uniform sampler drawPrefilterSampler;
layout(set = 3, binding = 3) uniform texture2D drawBRDF;
layout(set = 3, binding = 7) uniform sampler drawBRDFSampler;
vec4 drawSampleMaterial(uint slot, vec2 uv0, vec2 uv1)
{
    vec2 uv = drawSelectUV(drawObject.data.material, slot, uv0, uv1);
    vec4 value = vec4(1.0);
    if (slot == 0u)
    {
        value = texture(sampler2D(drawAlbedo, drawAlbedoSampler), uv);
    }
    else if (slot == 1u)
    {
        value = texture(sampler2D(drawMetalness, drawMetalnessSampler), uv);
    }
    else if (slot == 2u)
    {
        value = texture(sampler2D(drawRoughness, drawRoughnessSampler), uv);
    }
    else if (slot == 3u)
    {
        value = texture(sampler2D(drawNormal, drawNormalSampler), uv);
    }
    else if (slot == 4u)
    {
        value = texture(sampler2D(drawAO, drawAOSampler), uv);
    }
    else if (slot == 5u)
    {
        value = texture(sampler2D(drawEmission, drawEmissionSampler), uv);
    }
    return drawDecodeSample(drawObject.data.material, slot, value);
}
vec4 drawSampleMaterialLod(uint slot, vec2 uv0, vec2 uv1, float lod)
{
    vec2 uv = drawSelectUV(drawObject.data.material, slot, uv0, uv1);
    vec4 value = vec4(1.0);
    if (slot == 0u)
    {
        value = textureLod(sampler2D(drawAlbedo, drawAlbedoSampler), uv, lod);
    }
    else if (slot == 1u)
    {
        value = textureLod(sampler2D(drawMetalness, drawMetalnessSampler), uv, lod);
    }
    else if (slot == 2u)
    {
        value = textureLod(sampler2D(drawRoughness, drawRoughnessSampler), uv, lod);
    }
    else if (slot == 3u)
    {
        value = textureLod(sampler2D(drawNormal, drawNormalSampler), uv, lod);
    }
    else if (slot == 4u)
    {
        value = textureLod(sampler2D(drawAO, drawAOSampler), uv, lod);
    }
    else if (slot == 5u)
    {
        value = textureLod(sampler2D(drawEmission, drawEmissionSampler), uv, lod);
    }
    return drawDecodeSample(drawObject.data.material, slot, value);
}
#endif
#endif
