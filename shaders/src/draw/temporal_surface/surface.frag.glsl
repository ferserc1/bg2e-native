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
layout(std140, set = 1, binding = 1) uniform DrawObjectBlock
{
    DrawObjectData data;
} drawObject;
layout(set = 1, binding = 2) uniform texture2D drawAlbedo;
layout(set = 1, binding = 8) uniform sampler drawAlbedoSampler;
layout(std140, set = 2, binding = 0) uniform TemporalSurfaceBlock
{
    uvec4 identity;
} temporalSurface;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 0) out uint surfaceToken;
void main()
{
    DrawMaterialData m = drawObject.data.material;
    vec4 a = drawDecodeSample(m, 0u, texture(sampler2D(drawAlbedo, drawAlbedoSampler), drawSelectUV(m, 0u, uv0, uv1))) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard;
    }
    surfaceToken = temporalSurface.identity.y != 0u ? temporalSurface.identity.x : 0u;
}
