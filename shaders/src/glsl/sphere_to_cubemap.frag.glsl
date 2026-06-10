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
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D colorTexture;

void main() {
    outColor = texture(colorTexture, inTexCoord);
}
