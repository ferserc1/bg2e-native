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

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out flat int outCurrentMipLevel;
layout (location = 2) out flat int outTotalMipLevels;
layout (location = 3) out flat int outFaceIndex;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 view[6]; // One view matrix for each cube face
    mat4 proj;
} sceneData;

layout (push_constant) uniform Constants {
    int currentFace;
    int currentMipLevel;
    int totalMipLevels;
} SkyPushConstants;

void main() {
    mat4 view = mat4(mat3(sceneData.view[SkyPushConstants.currentFace]));
    gl_Position = sceneData.proj * view * vec4(inPosition, 1.0f);
    outNormal = normalize(inPosition);
    outCurrentMipLevel = SkyPushConstants.currentMipLevel;
    outTotalMipLevels = SkyPushConstants.totalMipLevels;
    outFaceIndex = SkyPushConstants.currentFace;
}