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

layout(location = 0) out vec4 colorAttachment1;
layout(location = 1) out vec4 colorAttachment2;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inPosition;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 2, binding = 0) uniform samplerCube giTex;

layout(push_constant) uniform constants {
    float gamma;
} pushConstants;

void main() {
    float gamma = pushConstants.gamma;
    
    vec3 color = texture(colorTex, inTexCoord).rgb;
    vec3 lighting = texture(giTex, inPosition).rgb;
    
    color = pow(color, vec3(1.0 / gamma));
    colorAttachment1 = vec4(color, 1.0f) * vec4(lighting, 1.0);
    colorAttachment2 = vec4(lighting, 1.0f);
}
