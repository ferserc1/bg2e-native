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

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler uSampler;

// cameraPos: viewer position for the reflection vector.
// lod:       explicit cubemap mip level used when sampling. Always 0 for the
//            single-mip maps; cycles through the specular map mip levels.
layout(push_constant) uniform PushConstants
{
    vec3 cameraPos;
    float lod;
}
push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 I = normalize(fragWorldPos - push.cameraPos);
    vec3 R = reflect(I, N);
    outColor = textureLod(samplerCube(uCubeMap, uSampler), R, push.lod);
}
