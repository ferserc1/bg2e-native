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

// SMAA luma edge detection pass (first pass).
//
// Faithful port of the reference SMAALumaEdgeDetectionPS (Jimenez et al.,
// http://www.iryoku.com/smaa/). Detects edges from the asymmetric luma gradient
// against the left/top neighbors and applies local contrast adaptation to
// suppress edges adjacent to much stronger ones.
//
//   edges.r = vertical edge   (|L - Lleft| above threshold)
//   edges.g = horizontal edge (|L - Ltop|  above threshold)

#version 460
#extension GL_ARB_shading_language_include : require

#include "lib/smaa.glsl"

layout(set = 0, binding = 0) uniform sampler2D inputImage;
layout(rg8, set = 0, binding = 1) uniform image2D edgesOutput;

layout(push_constant) uniform PushConstant {
    vec2 texelSize;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// SMAA_RT_METRICS = (1/width, 1/height, width, height)
vec4 rtMetrics;

float luma(vec2 uv) {
    const vec3 w = vec3(0.2126, 0.7152, 0.0722);
    return dot(textureLod(inputImage, uv, 0.0).rgb, w);
}

void main() {
    ivec2 size = imageSize(edgesOutput);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) {
        return;
    }

    rtMetrics = vec4(pc.texelSize, 1.0 / pc.texelSize.x, 1.0 / pc.texelSize.y);
    vec2 texcoord = (vec2(pixelCoord) + 0.5) * rtMetrics.xy;

    // Neighbor offsets normally computed in the SMAA vertex shader.
    vec4 offset0 = rtMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + texcoord.xyxy; // left / top
    vec4 offset1 = rtMetrics.xyxy * vec4( 1.0, 0.0, 0.0,  1.0) + texcoord.xyxy; // right / bottom
    vec4 offset2 = rtMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + texcoord.xyxy; // leftleft / toptop

    vec2 threshold = vec2(SMAA_THRESHOLD);

    // Luma of the current pixel and its left/top neighbors.
    float L = luma(texcoord);
    float Lleft = luma(offset0.xy);
    float Ltop  = luma(offset0.zw);

    vec4 delta;
    delta.xy = abs(L - vec2(Lleft, Ltop));
    vec2 edges = step(threshold, delta.xy);

    // No edge here: write zero (the edges image is not cleared each frame).
    if (dot(edges, vec2(1.0)) == 0.0) {
        imageStore(edgesOutput, pixelCoord, vec4(0.0));
        return;
    }

    // Right / bottom deltas.
    float Lright  = luma(offset1.xy);
    float Lbottom = luma(offset1.zw);
    delta.zw = abs(L - vec2(Lright, Lbottom));

    // Maximum delta in the direct neighborhood.
    vec2 maxDelta = max(delta.xy, delta.zw);

    // Left-left / top-top deltas.
    float Lleftleft = luma(offset2.xy);
    float Ltoptop   = luma(offset2.zw);
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

    // Final maximum delta.
    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation.
    edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

    imageStore(edgesOutput, pixelCoord, vec4(edges, 0.0, 0.0));
}
