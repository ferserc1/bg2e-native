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

#include "../lib/bindings.metal"
#include "../lib/pbr.metal"
// Inspect exponent bits so finite-value diagnostics also work with fast math.
inline bool drawInvalid(float3 value)
{
    return any((as_type<uint3>(value) & uint3(0x7f800000u)) == uint3(0x7f800000u));
}
fragment float4 fragMain(float4 position [[position]],
    texture2d<float> image0 [[texture(0)]],
    sampler sample0 [[sampler(0)]],
    texture2d<float> image1 [[texture(1)]],
    sampler sample1 [[sampler(1)]],
    texture2d<float> image2 [[texture(2)]],
    sampler sample2 [[sampler(2)]],
    texture2d<float> image3 [[texture(3)]],
    sampler sample3 [[sampler(3)]],
    texture2d<float> image4 [[texture(4)]],
    sampler sample4 [[sampler(4)]],
    texture2d<float> image5 [[texture(5)]],
    sampler sample5 [[sampler(5)]],
    depth2d<float> image6 [[texture(6)]],
    sampler sample6 [[sampler(6)]],
    texture2d<float> image7 [[texture(7)]],
    sampler sample7 [[sampler(7)]],
    constant uint4& lightingDebug [[buffer(0)]],
    constant DrawFrameData& drawFrame [[buffer(6)]],
    constant DrawLightUniforms& drawLights [[buffer(4)]],
    constant DrawEnvironmentData& drawEnvironment [[buffer(5)]],
    texturecube<float> irradianceImage [[texture(9)]],
    sampler irradianceSampler [[sampler(9)]],
    texturecube<float> prefilter [[texture(10)]],
    sampler prefilterSampler [[sampler(10)]],
    texture2d<float> brdfLUT [[texture(11)]],
    sampler brdfSampler [[sampler(11)]],
    texture2d<float> shadow0 [[texture(12)]], sampler shadowSampler0 [[sampler(12)]],
    texture2d<float> shadow1 [[texture(13)]], sampler shadowSampler1 [[sampler(13)]],
    texture2d<float> aoVisibility [[texture(14)]], sampler aoSampler [[sampler(14)]],
    texture2d<float> reflectedSignal [[texture(16)]])
{
    float2 size = float2(image0.get_width(), image0.get_height());
    float2 uv = position.xy / size;
    float4 a = image0.sample(sample0, uv);
    float4 normalEmission = image1.sample(sample1, uv);
    float4 material = image2.sample(sample2, uv);
    float4 fresnelFlags = image3.sample(sample3, uv);
    float4 sheenRefraction = image4.sample(sample4, uv);
    float3 baked = image5.sample(sample5, uv).rgb;
    float depth = image6.sample(sample6, uv);
    float4 background = image7.sample(sample7, uv);
    // Cleared normals are zero; copied opaque depth alone is not coverage.
    if (dot(normalEmission.rgb, normalEmission.rgb) < 1e-8)
    {
        return background;
    }
    uint flags = uint(round(fresnelFlags.a * 255.0));
    float3 n = normalize(normalEmission.xyz * 2.0 - 1.0);
    float4 world = drawFrame.inverseViewProjection * float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float3 p = world.xyz / world.w;
    float3 v = drawFrame.cameraPosition.xyz - p;
    v = dot(v, v) > 1e-8 ? normalize(v) : n;
    float nv = clamp(dot(n, v), 0.001, 1.0);
    float metallic = material.x;
    float roughness = max(material.y, 0.04);
    float ao = material.z;
    float rayAO = aoVisibility.sample(aoSampler,uv).r;
    float3 f0 = mix(float3(0.04) * fresnelFlags.rgb, a.rgb, metallic);
    if (lightingDebug.x == 1u)
    {
        return float4(a.rgb, 1.0);
    }
    if (lightingDebug.x == 9u)
    {
        return float4(float3(ao*rayAO), 1.0);
    }
    if (lightingDebug.x == 10u)
    {
        return float4(baked, 1.0);
    }
    if (lightingDebug.x == 11u)
    {
        return float4(float3(rayAO),1.0);
    }
    if (lightingDebug.x == 12u)
    {
        return float4((float4(0.0)).rgb,1.0);
    }
    if (lightingDebug.x == 13u)
    {
        return float4((reflectedSignal.sample(sample0,uv)).rgb,1.0);
    }
    if (lightingDebug.x == 14u)
    {
        return float4(float3((reflectedSignal.sample(sample0,uv)).a),1.0);
    }
    float3 direct = float3(0.0);
    float3 indirect = float3(0.0);
    float3 emission = a.rgb * normalEmission.a;
    float3 color = a.rgb;
    if ((flags & 1u) == 0u)
    {
        color = float3(0.0);
        for (uint i = 0u; i < drawLights.count.x; ++i)
        {
            DrawLightData light = drawLights.lights[i];
            float3 delta = light.positionIntensity.xyz - p;
            float d2 = max(dot(delta, delta), 0.0001);
            float3 l = light.flags.x == 4u ? normalize(-light.directionAngle.xyz) : delta * rsqrt(d2);
            float attenuation = light.flags.x == 4u ? 1.0 : 1.0 / d2;
            if (light.flags.x == 1u)
            {
                float outer = cos(light.directionAngle.w * DRAW_PI / 180.0);
                float inner = cos(light.parameters.x * DRAW_PI / 180.0);
                attenuation *= clamp((dot(l, -light.directionAngle.xyz) - outer) / max(inner - outer, 1e-5), 0.0, 1.0);
            }
            float3 h = v + l;
            h = dot(h, h) > 1e-8 ? normalize(h) : n;
            float nl = max(dot(n, l), 0.0);
            float3 f = drawFresnelSchlick(max(dot(h, v), 0.0), f0);
            float d = drawDistributionGGX(max(dot(n, h), 0.0), roughness);
            float geometry = drawGeometrySchlickGGX(nv, roughness) * drawGeometrySchlickGGX(nl, roughness);
            float3 brdf = (1.0 - f) * (1.0 - metallic) * a.rgb / DRAW_PI + d * geometry * f / max(4.0 * nv * nl, 1e-4);
            float3 sheen = sheenRefraction.rgb * material.w * pow(1.0 - nv, 5.0) * ao * rayAO;
            float4 visibility = i < 4u ? shadow0.sample(shadowSampler0, uv) : shadow1.sample(shadowSampler1, uv);
            color += visibility[i & 3u] * (brdf + sheen) * light.color.rgb * light.positionIntensity.w * attenuation * nl;
        }
        direct = color;
        float3 f = drawFresnelSchlickRoughness(nv, f0, roughness);
        float3 irradiance = irradianceImage.sample(irradianceSampler, n, level(0.0)).rgb;
        float3 prefiltered = prefilter.sample(prefilterSampler, reflect(-v, n), level(roughness * drawEnvironment.parameters.x)).rgb;
        float2 brdf = brdfLUT.sample(brdfSampler, float2(nv, roughness)).rg;
        if (lightingDebug.x == 6u)
        {
            return float4(irradiance, 1.0);
        }
        if (lightingDebug.x == 7u)
        {
            return float4(prefiltered, 1.0);
        }
        if (lightingDebug.x == 8u)
        {
            return float4(brdf, 0.0, 1.0);
        }
        float3 ambient = ((1.0 - f) * (1.0 - metallic) * irradiance * a.rgb / DRAW_PI + prefiltered * (f0 * brdf.x + brdf.y)) * ao;
        ambient += sheenRefraction.rgb * material.w * pow(1.0 - nv, 5.0) * irradiance / DRAW_PI * ao;
        float4 reflected = reflectedSignal.sample(sample0,uv);
        float3 reflectedIBL = prefiltered*(f0*brdf.x+brdf.y)*ao;
        float3 reflectedHit = reflected.rgb*(f0*brdf.x+brdf.y)*ao;
        ambient += (reflectedHit-reflectedIBL)*clamp(reflected.a,0.0,1.0);
        indirect = ambient * baked * rayAO;
        color += indirect + emission;
    }
    else
    {
        color += a.rgb * normalEmission.a;
    }
    if (lightingDebug.x == 5u)
    {
        bool invalid = drawInvalid(color) || drawInvalid(p) || drawInvalid(n) || drawInvalid(direct) || drawInvalid(indirect);
        return float4(invalid ? float3(1.0, 0.0, 1.0) : float3(0.15), 1.0);
    }
    if (lightingDebug.x == 2u)
    {
        color = direct;
    }
    else if (lightingDebug.x == 3u)
    {
        color = indirect;
    }
    else if (lightingDebug.x == 4u)
    {
        color = emission;
    }
    if (lightingDebug.x == 0u && (flags & 4u) != 0u && a.a < 1.0)
    {
        float3 viewNormal = float3x3(drawFrame.view[0].xyz, drawFrame.view[1].xyz, drawFrame.view[2].xyz) * n;
        float2 refractedUV = clamp(uv + viewNormal.xy * sheenRefraction.a, float2(0.0), float2(1.0));
        background = image7.sample(sample7, refractedUV);
        color = mix(background.rgb, color, a.a);
    }
    return float4(color, 1.0);
}
