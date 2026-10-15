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

layout(location = 0) out vec2 uv;

void main()
{
    const vec3 positions[4] = vec3[4](
        vec3(0.3f, 0.3f, 0.0f),
        vec3(-0.3f, 0.3f, 0.0f),
        vec3(-0.3f, -0.3f, 0.0f),
        vec3(0.3f, -0.3f, 0.0f));

    const vec2 uvs[4] = vec2[4](
        vec2(1.0f, 1.0f),
        vec2(0.0f, 1.0f),
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f));

    const int indices[6] = int[6](
        0, 1, 2,
        2, 3, 0);

    int index = indices[gl_VertexIndex];
    vec3 position = positions[index];
    gl_Position = vec4(position, 1.0f);
    uv = uvs[index];
}
