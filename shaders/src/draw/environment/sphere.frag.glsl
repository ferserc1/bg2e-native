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
layout(set = 0, binding = 0) uniform texture2D sourceImage;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    vec2 uv = vec2(atan(n.z, n.x) / (2.0 * PI) + 0.5, acos(clamp(n.y, -1.0, 1.0)) / PI);
    float lod = clamp(log2(max(float(params.sourceSize) / (4.0 * float(params.size)), 1.0)), 0.0, params.sourceMaxLod);
    return vec4(textureLod(sampler2D(sourceImage, sourceSampler), uv, lod).rgb, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
