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
layout(set = 0, binding = 0) uniform textureCube sky;
layout(set = 0, binding = 1) uniform sampler skySampler;
layout(push_constant) uniform Push
{
    mat4 inverseViewProjection;
    vec4 parameters;
} params;
layout(location = 0) out vec4 outColor;
void main()
{
    vec2 uv = gl_FragCoord.xy / params.parameters.xy;
    vec4 world = params.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    vec3 direction = normalize(world.xyz);
    outColor = vec4(textureLod(samplerCube(sky, skySampler), direction, params.parameters.z).rgb * exp2(params.parameters.w), 1.0);
}
