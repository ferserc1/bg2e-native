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
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out flat int outCurrentMipLevel;
layout (location = 2) out flat int outTotalMipLevels;

layout (push_constant) uniform constants {
    int currentFace;
    int currentMipLevel;
    int totalMipLevels;
} pushConstants;

layout (set = 0, binding = 0) uniform ProjectionData {
    mat4 view[6];
    mat4 proj;
} projectionData;

void main() {
    mat4 view = mat4(mat3(projectionData.view[pushConstants.currentFace]));

    gl_Position = projectionData.proj * view * vec4(inPosition, 1.0f);
    outTexCoord = inTexCoord;
    outCurrentMipLevel = pushConstants.currentMipLevel;
    outTotalMipLevels = pushConstants.totalMipLevels;
}
