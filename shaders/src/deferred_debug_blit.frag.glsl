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

layout(binding = 0) uniform sampler2D srcImage;

layout(push_constant) uniform PC {
    uint channelMode;
} pc;

layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(srcImage, vTexcoord);
    if (pc.channelMode == 1) {
        outColor = vec4(color.r, color.r, color.r, 1.0);
    } else if (pc.channelMode == 2) {
        outColor = vec4(color.g, color.g, color.g, 1.0);
    } else if (pc.channelMode == 3) {
        outColor = vec4(color.b, color.b, color.b, 1.0);
    } else if (pc.channelMode == 4) {
        outColor = vec4(color.a, color.a, color.a, 1.0);
    } else if (pc.channelMode == 5) {
        outColor = vec4(color.rgb * color.a, 1.0);
    } else {
        outColor = vec4(color.rgb, 1.0);
    }
}
