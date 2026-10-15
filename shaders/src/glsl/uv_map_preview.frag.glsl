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

#version 460

layout(location = 0) in vec3 barycentric;
layout(location = 1) flat in uint submeshIndex;

layout(push_constant) uniform PreviewParams
{
    float lineWidth;
    uint mode; // 0 = UV triangles, 1 = atlas boundary lines
}
params;

layout(location = 0) out vec4 outColor;

vec3 submeshTint(uint index)
{
    uint h = index * 2654435761u + 1013904223u;
    float r = float(h & 0xffu) / 255.0;
    float g = float((h >> 8u) & 0xffu) / 255.0;
    float b = float((h >> 16u) & 0xffu) / 255.0;
    return mix(vec3(0.35), vec3(0.95), vec3(r, g, b));
}

void main()
{
    if (params.mode == 1u)
    {
        // Atlas boundary
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    // Dim per-submesh fill shows the valid-texel coverage; the barycentric
    // edge factor draws the wireframe without polygon-mode line fill.
    float minBarycentric = min(barycentric.x, min(barycentric.y, barycentric.z));
    float edge = 1.0 - smoothstep(0.0, params.lineWidth, minBarycentric);
    vec3 fill = submeshTint(submeshIndex) * 0.35;
    outColor = vec4(mix(fill, vec3(1.0), edge), 1.0);
}
