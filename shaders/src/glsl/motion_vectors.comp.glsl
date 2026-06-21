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

#version 460
#extension GL_ARB_shading_language_include : require

#include "lib/deferred_utils.glsl"

layout(binding = 0) uniform sampler2D g_Depth;
layout(binding = 1, rg16f) uniform image2D outMotionVectors;

layout(push_constant) uniform PC {
    mat4  currentInvViewProj;
    mat4  prevViewProj;
    vec2  renderSize;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= int(pc.renderSize.x) || pixelCoord.y >= int(pc.renderSize.y)) {
        return;
    }

    vec2 currentUV = (vec2(pixelCoord) + 0.5) / pc.renderSize;
    float depth = texture(g_Depth, currentUV).r;

    // Sky / empty pixels have no meaningful motion
    if (depth >= 1.0) {
        imageStore(outMotionVectors, pixelCoord, vec4(0.0));
        return;
    }

    vec3 worldPos = reconstructWorldPosition(currentUV, depth, pc.currentInvViewProj);

    vec4 prevClip = pc.prevViewProj * vec4(worldPos, 1.0);
    if (prevClip.w <= 0.0) {
        imageStore(outMotionVectors, pixelCoord, vec4(0.0));
        return;
    }

    // Convert previous clip position to UV (Vulkan Y-flip)
    vec2 prevNDC = prevClip.xy / prevClip.w;
    vec2 prevUV  = vec2(prevNDC.x * 0.5 + 0.5, 0.5 - prevNDC.y * 0.5);

    // Pixel-space motion vector: where this pixel came from in the previous frame
    vec2 motionVec = (prevUV - currentUV) * pc.renderSize;

    imageStore(outMotionVectors, pixelCoord, vec4(motionVec, 0.0, 0.0));
}
