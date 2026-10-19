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
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    float altitude = max(n.y, 0.0);
    vec3 sky = mix(params.skyHorizonColor.rgb, params.skyZenithColor.rgb, sqrt(altitude));
    vec3 ground = params.groundColor.rgb;
    vec3 color = mix(ground, sky, smoothstep(-0.03, 0.03, n.y));
    vec3 sun = normalize(vec3(0.4, 0.7, 0.2));
    color += vec3(8.0, 6.5, 4.0) * pow(max(dot(n, sun), 0.0), 512.0);
    return vec4(color, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
