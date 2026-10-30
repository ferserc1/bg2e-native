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
#extension GL_GOOGLE_include_directive : require
#include "../lib/bindings.glsl"
#include "../lib/pbr.glsl"
layout(set = 1, binding = 0) uniform texture2D image0;
layout(set = 1, binding = 10) uniform sampler sample0;
layout(set = 1, binding = 1) uniform texture2D image1;
layout(set = 1, binding = 11) uniform sampler sample1;
layout(set = 1, binding = 2) uniform texture2D image2;
layout(set = 1, binding = 12) uniform sampler sample2;
layout(set = 1, binding = 3) uniform texture2D image3;
layout(set = 1, binding = 13) uniform sampler sample3;
layout(set = 1, binding = 4) uniform texture2D image4;
layout(set = 1, binding = 14) uniform sampler sample4;
layout(set = 1, binding = 5) uniform texture2D image5;
layout(set = 1, binding = 15) uniform sampler sample5;
layout(set = 1, binding = 6) uniform texture2D image6;
layout(set = 1, binding = 16) uniform sampler sample6;
layout(set = 1, binding = 7) uniform texture2D image7;
layout(set = 1, binding = 17) uniform sampler sample7;
layout(std140, set = 0, binding = 1) uniform FrameBlock { DrawFrameData data; } drawFrame;
layout(std140, set = 2, binding = 0) uniform LightsBlock { DrawLightUniforms data; } drawLights;
layout(std140, set = 3, binding = 8) uniform EnvironmentBlock { DrawEnvironmentData data; } drawEnvironment;
layout(set = 3, binding = 1) uniform textureCube irradiance;
layout(set = 3, binding = 5) uniform sampler irradianceSampler;
layout(set = 3, binding = 2) uniform textureCube prefilter;
layout(set = 3, binding = 6) uniform sampler prefilterSampler;
layout(set = 3, binding = 3) uniform texture2D brdfLUT;
layout(set = 3, binding = 7) uniform sampler brdfSampler;
layout(set = 1, binding = 8) uniform texture2D aoVisibility;
layout(set = 1, binding = 18) uniform sampler aoSampler;
layout(set = 1, binding = 9) uniform texture2D giSignal;
layout(set = 1, binding = 19) uniform sampler giSampler;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform LightingDebug
{
    uvec4 value;
} lightingDebug;
vec4 composite()
{
    vec2 size = vec2(textureSize(sampler2D(image0, sample0), 0));
    vec2 uv = gl_FragCoord.xy / size;
    vec4 a = texture(sampler2D(image0, sample0), uv);
    vec4 normalEmission = texture(sampler2D(image1, sample1), uv);
    vec4 material = texture(sampler2D(image2, sample2), uv);
    vec4 fresnelFlags = texture(sampler2D(image3, sample3), uv);
    vec4 sheenRefraction = texture(sampler2D(image4, sample4), uv);
    vec3 baked = texture(sampler2D(image5, sample5), uv).rgb;
    float depth = texture(sampler2D(image6, sample6), uv).r;
    vec4 background = texture(sampler2D(image7, sample7), uv);
    // Cleared normals are zero; copied opaque depth alone is not coverage.
    if (dot(normalEmission.rgb, normalEmission.rgb) < 1e-8)
    {
        return background;
    }
    uint flags = uint(round(fresnelFlags.a * 255.0));
    vec3 n = normalize(normalEmission.xyz * 2.0 - 1.0);
    vec4 world = drawFrame.data.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    vec3 p = world.xyz / world.w;
    vec3 v = drawFrame.data.cameraPosition.xyz - p;
    v = dot(v, v) > 1e-8 ? normalize(v) : n;
    float nv = clamp(dot(n, v), 0.001, 1.0);
    float metallic = material.x;
    float roughness = max(material.y, 0.04);
    float ao = material.z;
    float rayAO = (flags & 32u) != 0u ? 1.0 : texture(sampler2D(aoVisibility,aoSampler),uv).r;
    vec3 f0 = mix(vec3(0.04) * fresnelFlags.rgb, a.rgb, metallic);
    if (lightingDebug.value.x == 1u)
    {
        return vec4(a.rgb, 1.0);
    }
    if (lightingDebug.value.x == 9u)
    {
        return vec4(vec3(ao*rayAO), 1.0);
    }
    if (lightingDebug.value.x == 10u)
    {
        return vec4(baked, 1.0);
    }
    if (lightingDebug.value.x == 11u)
    {
        return vec4(vec3(rayAO),1.0);
    }
    if (lightingDebug.value.x == 12u)
    {
        return vec4((texture(sampler2D(giSignal,giSampler),uv)).rgb,1.0);
    }
    if (lightingDebug.value.x == 13u)
    {
        return vec4((vec4(0.0)).rgb,1.0);
    }
    if (lightingDebug.value.x == 14u)
    {
        return vec4(vec3((vec4(0.0)).a),1.0);
    }
    vec3 direct = vec3(0.0);
    vec3 indirect = vec3(0.0);
    vec3 emission = a.rgb * normalEmission.a;
    vec3 color = a.rgb;
    if ((flags & 1u) == 0u)
    {
        color = vec3(0.0);
        for (uint i = 0u; i < drawLights.data.count.x; ++i)
        {
            DrawLightData light = resolveSolarLight(drawLights.data.lights[i]);
            vec3 delta = light.positionIntensity.xyz - p;
            float d2 = max(dot(delta, delta), 0.0001);
            vec3 l = light.flags.x == 4u ? normalize(-light.directionAngle.xyz) : delta * inversesqrt(d2);
            float attenuation = light.flags.x == 4u ? 1.0 : 1.0 / d2;
            if (light.flags.x == 1u)
            {
                float outer = cos(light.directionAngle.w * DRAW_PI / 180.0);
                float inner = cos(light.parameters.x * DRAW_PI / 180.0);
                attenuation *= clamp((dot(l, -light.directionAngle.xyz) - outer) / max(inner - outer, 1e-5), 0.0, 1.0);
            }
            vec3 h = v + l;
            h = dot(h, h) > 1e-8 ? normalize(h) : n;
            float nl = max(dot(n, l), 0.0);
            vec3 f = drawFresnelSchlick(max(dot(h, v), 0.0), f0);
            float d = drawDistributionGGX(max(dot(n, h), 0.0), roughness);
            float geometry = drawGeometrySchlickGGX(nv, roughness) * drawGeometrySchlickGGX(nl, roughness);
            vec3 brdf = (1.0 - f) * (1.0 - metallic) * a.rgb / DRAW_PI + d * geometry * f / max(4.0 * nv * nl, 1e-4);
            vec3 sheen = sheenRefraction.rgb * material.w * pow(1.0 - nv, 5.0) * ao * rayAO;
            color += (brdf + sheen) * light.color.rgb * light.positionIntensity.w * attenuation * nl;
        }
        direct = color;
        vec3 f = drawFresnelSchlickRoughness(nv, f0, roughness);
        vec3 irradiance = textureLod(samplerCube(irradiance, irradianceSampler), n, 0.0).rgb;
        vec3 prefiltered = textureLod(samplerCube(prefilter, prefilterSampler), reflect(-v, n), roughness * drawEnvironment.data.parameters.x).rgb;
        vec2 brdf = texture(sampler2D(brdfLUT, brdfSampler), vec2(nv, roughness)).rg;
        if (lightingDebug.value.x == 6u)
        {
            return vec4(irradiance, 1.0);
        }
        if (lightingDebug.value.x == 7u)
        {
            return vec4(prefiltered, 1.0);
        }
        if (lightingDebug.value.x == 8u)
        {
            return vec4(brdf, 0.0, 1.0);
        }
        vec3 ambient = ((1.0 - f) * (1.0 - metallic) * irradiance * a.rgb / DRAW_PI + prefiltered * (f0 * brdf.x + brdf.y)) * ao;
        ambient += sheenRefraction.rgb * material.w * pow(1.0 - nv, 5.0) * irradiance / DRAW_PI * ao;
        vec4 gi = texture(sampler2D(giSignal,giSampler),uv);
        if (gi.a > 0.5 && (flags & 32u) == 0u)
        {
            vec3 diffuseIBL = (1.0-f)*(1.0-metallic)*irradiance*a.rgb/DRAW_PI*ao;
            vec3 diffuseGI = (1.0-f)*(1.0-metallic)*a.rgb*gi.rgb*ao;
            ambient += diffuseGI-diffuseIBL;
        }
        vec3 preservedSpecular = (flags & 64u) != 0u ? prefiltered * (f0 * brdf.x + brdf.y) * ao : vec3(0.0);
        indirect = ((ambient - preservedSpecular) * baked + preservedSpecular) * rayAO;
        color += indirect + emission;
    }
    else
    {
        color += a.rgb * normalEmission.a;
    }
    if (lightingDebug.value.x == 5u)
    {
        bool invalid = any(isnan(color)) || any(isinf(color)) || any(isnan(p)) || any(isinf(p)) || any(isnan(n)) || any(isinf(n)) || any(isnan(direct)) || any(isinf(direct)) || any(isnan(indirect)) || any(isinf(indirect));
        return vec4(invalid ? vec3(1.0, 0.0, 1.0) : vec3(0.15), 1.0);
    }
    if (lightingDebug.value.x == 2u)
    {
        color = direct;
    }
    else if (lightingDebug.value.x == 3u)
    {
        color = indirect;
    }
    else if (lightingDebug.value.x == 4u)
    {
        color = emission;
    }
    if (lightingDebug.value.x == 0u && (flags & 4u) != 0u && a.a < 1.0)
    {
        vec3 viewNormal = mat3(drawFrame.data.view) * n;
        vec2 refractedUV = clamp(uv + viewNormal.xy * sheenRefraction.a, vec2(0.0), vec2(1.0));
        background = texture(sampler2D(image7, sample7), refractedUV);
        color = mix(background.rgb, color, a.a);
    }
    return vec4(color, 1.0);
}
void main()
{
    outColor = composite();
}
