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
    vec4 sunDirectionRadius;
    vec4 sunRadiance;
} params;
layout(location = 0) out vec4 outColor;
void main()
{
    vec2 uv = gl_FragCoord.xy / params.parameters.xy;
    vec4 world = params.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    vec3 direction = normalize(world.xyz);
    vec3 color = textureLod(samplerCube(sky, skySampler), direction, params.parameters.z).rgb;
    // Chord distance is stable for the small physical solar angle. Evaluate
    // coverage at screen resolution rather than magnifying cubemap texels.
    float chord = length(direction - params.sunDirectionRadius.xyz);
    float radius = 2.0 * sin(0.5 * params.sunDirectionRadius.w);
    float edge = max(0.5 * (length(dFdx(direction)) + length(dFdy(direction))), 1e-7);
    float disk = 1.0 - smoothstep(max(radius - edge, 0.0), radius + edge, chord);
    float horizonEdge = max(0.5 * fwidth(direction.y), 1e-7);
    float horizon = smoothstep(-horizonEdge, horizonEdge, direction.y);
    color += params.sunRadiance.rgb * disk * horizon * params.sunRadiance.w;
    outColor = vec4(color * exp2(params.parameters.w), 1.0);
}
