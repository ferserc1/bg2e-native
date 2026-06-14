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

// SMAA neighborhood blending pass (third pass).
//
// Faithful port of the reference SMAANeighborhoodBlendingPS (Jimenez et al.,
// http://www.iryoku.com/smaa/). Gathers the blend weights of the current pixel
// and its right/bottom neighbors, picks the dominant blending direction and
// mixes the current pixel with the chosen neighbor via two bilinear fetches.

#version 460
#extension GL_ARB_shading_language_include : require

#include "lib/smaa.glsl"

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D blendTex;
layout(rgba8, set = 0, binding = 2) uniform image2D outputImage;

layout(push_constant) uniform PushConstant {
    vec2 texelSize;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// SMAA_RT_METRICS = (1/width, 1/height, width, height)
vec4 rtMetrics;

// Conditional move helpers (replace components where cond is true).
void SMAAMovc2(bvec2 cond, inout vec2 v, vec2 value) {
    if (cond.x) v.x = value.x;
    if (cond.y) v.y = value.y;
}
void SMAAMovc4(bvec4 cond, inout vec4 v, vec4 value) {
    SMAAMovc2(cond.xy, v.xy, value.xy);
    SMAAMovc2(cond.zw, v.zw, value.zw);
}

void main() {
    ivec2 size = imageSize(outputImage);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) {
        return;
    }

    rtMetrics = vec4(pc.texelSize, 1.0 / pc.texelSize.x, 1.0 / pc.texelSize.y);
    vec2 texcoord = (vec2(pixelCoord) + 0.5) * rtMetrics.xy;
    vec4 offset = rtMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + texcoord.xyxy;

    // Fetch the blending weights for the current pixel.
    vec4 a;
    a.x = textureLod(blendTex, offset.xy, 0.0).a;   // Right
    a.y = textureLod(blendTex, offset.zw, 0.0).g;   // Top
    a.wz = textureLod(blendTex, texcoord, 0.0).xz;  // Bottom / Left

    // No blending weights: pass the color through unchanged.
    if (dot(a, vec4(1.0)) < 1e-5) {
        vec4 color = textureLod(colorTex, texcoord, 0.0);
        imageStore(outputImage, pixelCoord, vec4(color.rgb, 1.0));
        return;
    }

    bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)

    // Calculate the blending offsets.
    vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
    vec2 blendingWeight = a.yw;
    SMAAMovc4(bvec4(h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
    SMAAMovc2(bvec2(h), blendingWeight, a.xz);
    blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));

    // Calculate the texture coordinates.
    vec4 blendingCoord = blendingOffset * vec4(rtMetrics.xy, -rtMetrics.xy) + texcoord.xyxy;

    // Exploit bilinear filtering to mix the current pixel with the chosen neighbor.
    vec4 color = blendingWeight.x * textureLod(colorTex, blendingCoord.xy, 0.0);
    color += blendingWeight.y * textureLod(colorTex, blendingCoord.zw, 0.0);

    imageStore(outputImage, pixelCoord, vec4(color.rgb, 1.0));
}
