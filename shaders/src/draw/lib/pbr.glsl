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

#ifndef BG2E_DRAW_PBR_GLSL
#define BG2E_DRAW_PBR_GLSL
#include "lib/math.glsl"
vec3 drawFresnelSchlick(float cosine, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}
vec3 drawFresnelSchlickRoughness(float cosine, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}
float drawDistributionGGX(float nDotH, float roughness)
{
    float a = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(DRAW_PI * denominator * denominator, 1e-8);
}
float drawGeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 1e-7);
}
vec3 drawDirectBRDF(vec3 normal, vec3 view, vec3 light, vec3 albedo, float metallic, float roughness)
{
    vec3 halfVector = normalize(view + light);
    float nv = max(dot(normal, view), 0.0);
    float nl = max(dot(normal, light), 0.0);
    vec3 f = drawFresnelSchlick(max(dot(halfVector, view), 0.0), mix(vec3(0.04), albedo, metallic));
    float d = drawDistributionGGX(max(dot(normal, halfVector), 0.0), roughness);
    float g = drawGeometrySchlickGGX(nv, roughness) * drawGeometrySchlickGGX(nl, roughness);
    return ((1.0 - f) * (1.0 - metallic) * albedo / DRAW_PI + d * g * f / max(4.0 * nv * nl, 1e-4)) * nl;
}
#endif
