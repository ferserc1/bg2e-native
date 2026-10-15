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

#ifndef BG2E_DRAW_PBR_METAL
#define BG2E_DRAW_PBR_METAL
#include "lib/math.metal"
inline float3 drawFresnelSchlick(float cosine, float3 f0)
{
    return f0 + (1.0f - f0) * pow(clamp(1.0f - cosine, 0.0f, 1.0f), 5.0f);
}
inline float3 drawFresnelSchlickRoughness(float cosine, float3 f0, float roughness)
{
    return f0 + (max(float3(1.0f - roughness), f0) - f0) * pow(clamp(1.0f - cosine, 0.0f, 1.0f), 5.0f);
}
inline float drawDistributionGGX(float nDotH, float roughness)
{
    float a = max(roughness * roughness, 1e-4f);
    float a2 = a * a;
    float denominator = nDotH * nDotH * (a2 - 1.0f) + 1.0f;
    return a2 / max(DRAW_PI * denominator * denominator, 1e-8f);
}
inline float drawGeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = r * r / 8.0f;
    return nDotV / max(nDotV * (1.0f - k) + k, 1e-7f);
}
inline float3 drawDirectBRDF(float3 normal, float3 view, float3 light, float3 albedo, float metallic, float roughness)
{
    float3 halfVector = normalize(view + light);
    float nv = max(dot(normal, view), 0.0f);
    float nl = max(dot(normal, light), 0.0f);
    float3 f = drawFresnelSchlick(max(dot(halfVector, view), 0.0f), mix(float3(0.04f), albedo, metallic));
    float d = drawDistributionGGX(max(dot(normal, halfVector), 0.0f), roughness);
    float g = drawGeometrySchlickGGX(nv, roughness) * drawGeometrySchlickGGX(nl, roughness);
    return ((1.0f - f) * (1.0f - metallic) * albedo / DRAW_PI + d * g * f / max(4.0f * nv * nl, 1e-4f)) * nl;
}
#endif
