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

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPosition;

void main()
{
    const vec3 positions[4] = vec3[4](
        vec3(-1.0, -1.0, 0.0),
        vec3( 1.0, -1.0, 0.0),
        vec3( 1.0,  1.0, 0.0),
        vec3(-1.0,  1.0, 0.0)
    );
    
    const vec2 uvs[4] = vec2[4](
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0)
    );
    
    const int indexes[6] = {
        0, 1, 2,
        2, 3, 0
    };
    
    int index = indexes[gl_VertexIndex];
    outPosition = positions[index];
    gl_Position = vec4(outPosition, 1.0f);
    outUV = uvs[index];
}
