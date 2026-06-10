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

#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/color_correction.glsl"


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV0;
layout(location = 2) out vec2 outUV1;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out vec3 outFragPos;

layout(location = 5) out mat3 outTBN;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;


void main() {
	gl_Position = sceneData.projMatrix * sceneData.viewMatrix * objectData.modelMatrix * vec4(inPosition, 1.0f);
	outUV0 = inUV0;
    outUV1 = inUV1;

    outTBN = TBNMatrix(objectData.modelMatrix, inNormal, inTangent);
    
    outNormal = mat3(objectData.modelMatrix) * inNormal;

    outViewPos = (inverse(sceneData.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    outFragPos = (objectData.modelMatrix * vec4(inPosition, 1.0f)).xyz;
}

