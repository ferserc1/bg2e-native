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

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPosition;

layout (binding = 0) uniform sampler2D att1Tex;
layout (binding = 1) uniform sampler2D att2Tex;

void main()
{
    vec3 att1Color = texture(att1Tex, inUV).rgb;
    vec3 att2Color = texture(att2Tex, inUV).rgb;
    
    outColor = vec4(inPosition.x > 0 ? att2Color : att1Color, 1.0);
}
