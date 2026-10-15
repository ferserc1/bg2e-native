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
#define DRAW_BINDINGS_FRAGMENT
#include "../lib/bindings.glsl"
layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 worldTangent;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 5) in float tangentSign;
layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gMaterial;
layout(location = 3) out vec4 gFresnel;
layout(location = 4) out vec4 gSheen;
layout(location = 5) out vec4 gBaked;
void main()
{
    DrawMaterialData m = drawObject.data.material;
    vec4 a = drawSampleMaterial(0u, uv0, uv1) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard;
    }
    vec3 n = normalize(worldNormal);
    vec3 t = normalize(worldTangent - n * dot(n, worldTangent));
    vec3 mapped = drawSampleMaterial(3u, uv0, uv1).xyz * 2.0 - 1.0;
    mapped = t * mapped.x + cross(n, t) * mapped.y * tangentSign + n * mapped.z;
    n = dot(mapped, mapped) > 1e-8 ? normalize(mapped) : n;
    float metallic = clamp(drawScalarSample(drawSampleMaterial(1u, uv0, uv1), m.channels.x, m.invert.x) * m.factors.x, 0.0, 1.0);
    float roughness = clamp(drawScalarSample(drawSampleMaterial(2u, uv0, uv1), m.channels.y, m.invert.y) * m.factors.y, 0.04, 1.0);
    float ao = clamp(drawSampleMaterial(4u, uv0, uv1)[m.channels.z], 0.0, 1.0);
    float emission = max(drawScalarSample(drawSampleMaterial(5u, uv0, uv1), m.channels.w, m.invert.z) * m.emissionAlpha.x, 0.0);
    gAlbedo = vec4(a.rgb, (flags & 4u) == 0u ? 1.0 : a.a);
    gNormal = vec4(n * 0.5 + 0.5, emission);
    gMaterial = vec4(metallic, roughness, ao, m.factors.z);
    gFresnel = vec4(m.fresnelTint.rgb, float(flags) / 255.0);
    gSheen = vec4(m.sheenColor.rgb, m.factors.w);
    gBaked = vec4(1.0);
}
