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
#define DRAW_BINDINGS_VERTEX
#include "../lib/bindings.glsl"
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec2 texcoord1;
layout(location = 4) in vec3 tangent;
layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 worldTangent;
layout(location = 3) out vec2 uv0;
layout(location = 4) out vec2 uv1;
layout(location = 5) out float tangentSign;
void main()
{
    worldPosition = (drawObject.data.model * vec4(position, 1.0)).xyz;
    worldNormal = normalize(mat3(drawObject.data.normal) * normal);
    vec3 t = mat3(drawObject.data.model) * tangent;
    t -= worldNormal * dot(t, worldNormal);
    if (dot(t, t) < 1e-8)
    {
        t = cross(abs(worldNormal.z) < 0.99 ? vec3(0, 0, 1) : vec3(0, 1, 0), worldNormal);
    }
    worldTangent = normalize(t);
    tangentSign = determinant(mat3(drawObject.data.model)) < 0.0 ? -1.0 : 1.0;
    uv0 = texcoord0;
    uv1 = texcoord1;
    gl_Position = drawFrame.data.projection * drawFrame.data.view * vec4(worldPosition, 1.0);
}
