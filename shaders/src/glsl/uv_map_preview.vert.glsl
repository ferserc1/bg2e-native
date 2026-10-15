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

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec3 inBarycentric;
layout(location = 2) in uint inSubmesh;

layout(location = 0) out vec3 barycentric;
layout(location = 1) flat out uint submeshIndex;

void main()
{
    barycentric = inBarycentric;
    submeshIndex = inSubmesh;

    // Same convention as the UV surface pass: the engine's negative-height
    // viewport maps v=1 to the top image row.
    vec2 uvInv = vec2(inUv.x, 1.0 - inUv.y);
    gl_Position = vec4(uvInv * 2.0 - 1.0, 0.0, 1.0);
}
