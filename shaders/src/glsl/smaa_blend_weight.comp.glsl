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

// SMAA blending-weight calculation pass (second pass).
//
// This is a faithful port of the reference SMAABlendingWeightCalculationPS
// (Jimenez et al., http://www.iryoku.com/smaa/) using the official precomputed
// AreaTex/SearchTex lookup tables (see lib/third_party/iryku_smaa). It uses the
// orthogonal configuration only (no diagonal detection, no corner detection),
// equivalent to the LOW/MEDIUM presets.
//
// The edges texture must be sampled with LINEAR filtering: the accelerated
// search relies on bilinear interpolation between edge texels.

#version 460
#extension GL_ARB_shading_language_include : require

#include "lib/smaa.glsl"

layout(set = 0, binding = 0) uniform sampler2D edgesTex;
layout(set = 0, binding = 1) uniform sampler2D areaTex;
layout(set = 0, binding = 2) uniform sampler2D searchTex;
layout(rgba8, set = 0, binding = 3) uniform image2D blendWeightsOutput;

layout(push_constant) uniform PushConstant {
    vec2 texelSize;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// SMAA_RT_METRICS = (1/width, 1/height, width, height)
vec4 rtMetrics;

//-----------------------------------------------------------------------------
// Horizontal / vertical search functions

float SMAASearchLength(vec2 e, float offset) {
    // The searchTex is flipped vertically, with the left/right cases taking
    // half of the horizontal space.
    vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0);
    vec2 bias  = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0);

    // Scale and bias to access texel centers.
    scale += vec2(-1.0,  1.0);
    bias  += vec2( 0.5, -0.5);

    // Convert from pixel coordinates to texcoords.
    scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
    bias  *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;

    return textureLod(searchTex, scale * e + bias, 0.0).r;
}

float SMAASearchXLeft(vec2 texcoord, float end) {
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x > end &&
           e.g > 0.8281 &&  // Is there some edge not activated?
           e.r == 0.0) {    // Or is there a crossing edge that breaks the line?
        e = textureLod(edgesTex, texcoord, 0.0).rg;
        texcoord = -vec2(2.0, 0.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e, 0.0) + 3.25;
    return rtMetrics.x * offset + texcoord.x;
}

float SMAASearchXRight(vec2 texcoord, float end) {
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x < end &&
           e.g > 0.8281 &&
           e.r == 0.0) {
        e = textureLod(edgesTex, texcoord, 0.0).rg;
        texcoord = vec2(2.0, 0.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e, 0.5) + 3.25;
    return -rtMetrics.x * offset + texcoord.x;
}

float SMAASearchYUp(vec2 texcoord, float end) {
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y > end &&
           e.r > 0.8281 &&
           e.g == 0.0) {
        e = textureLod(edgesTex, texcoord, 0.0).rg;
        texcoord = -vec2(0.0, 2.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e.gr, 0.0) + 3.25;
    return rtMetrics.y * offset + texcoord.y;
}

float SMAASearchYDown(vec2 texcoord, float end) {
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y < end &&
           e.r > 0.8281 &&
           e.g == 0.0) {
        e = textureLod(edgesTex, texcoord, 0.0).rg;
        texcoord = vec2(0.0, 2.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e.gr, 0.5) + 3.25;
    return -rtMetrics.y * offset + texcoord.y;
}

//-----------------------------------------------------------------------------
// Area function

vec2 SMAAArea(vec2 dist, float e1, float e2, float offset) {
    // Rounding prevents precision errors of bilinear filtering.
    vec2 texcoord = vec2(SMAA_AREATEX_MAX_DISTANCE) * round(4.0 * vec2(e1, e2)) + dist;

    // Scale and bias to map to texel space.
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Move to the proper place according to the subpixel offset.
    texcoord.y = SMAA_AREATEX_SUBTEX_SIZE * offset + texcoord.y;

    return textureLod(areaTex, texcoord, 0.0).rg;
}

//-----------------------------------------------------------------------------

void main() {
    ivec2 size = imageSize(blendWeightsOutput);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) {
        return;
    }

    rtMetrics = vec4(pc.texelSize, 1.0 / pc.texelSize.x, 1.0 / pc.texelSize.y);

    vec2 texcoord = (vec2(pixelCoord) + 0.5) * rtMetrics.xy;
    vec2 pixcoord = vec2(pixelCoord) + 0.5;

    // Offsets normally computed in the SMAA vertex shader.
    vec4 offset0 = rtMetrics.xyxy * vec4(-0.25, -0.125,  1.25, -0.125) + texcoord.xyxy;
    vec4 offset1 = rtMetrics.xyxy * vec4(-0.125, -0.25, -0.125,  1.25) + texcoord.xyxy;
    vec4 offset2 = rtMetrics.xxyy * (vec4(-2.0, 2.0, -2.0, 2.0) * float(SMAA_MAX_SEARCH_STEPS))
                   + vec4(offset0.xz, offset1.yw);

    vec4 weights = vec4(0.0);

    vec2 e = textureLod(edgesTex, texcoord, 0.0).rg;

    if (e.g > 0.0) { // Edge at north
        vec3 coords;
        vec2 d;

        // Find the distance to the left.
        coords.x = SMAASearchXLeft(offset0.xy, offset2.x);
        coords.y = offset1.y;
        d.x = coords.x;

        // Fetch the left crossing edges (bilinear, two at a time).
        float e1 = textureLod(edgesTex, coords.xy, 0.0).r;

        // Find the distance to the right.
        coords.z = SMAASearchXRight(offset0.zw, offset2.y);
        d.y = coords.z;

        // Distances in pixel units.
        d = abs(round(rtMetrics.zz * d - pixcoord.xx));

        // The area texture is compressed quadratically.
        vec2 sqrt_d = sqrt(d);

        // Fetch the right crossing edges.
        float e2 = textureLodOffset(edgesTex, vec2(coords.z, coords.y), 0.0, ivec2(1, 0)).r;

        weights.rg = SMAAArea(sqrt_d, e1, e2, 0.0);
    }

    if (e.r > 0.0) { // Edge at west
        vec3 coords;
        vec2 d;

        // Find the distance upwards.
        coords.y = SMAASearchYUp(offset1.xy, offset2.z);
        coords.x = offset0.x;
        d.x = coords.y;

        float e1 = textureLod(edgesTex, coords.xy, 0.0).g;

        // Find the distance downwards.
        coords.z = SMAASearchYDown(offset1.zw, offset2.w);
        d.y = coords.z;

        d = abs(round(rtMetrics.ww * d - pixcoord.yy));

        vec2 sqrt_d = sqrt(d);

        float e2 = textureLodOffset(edgesTex, vec2(coords.x, coords.z), 0.0, ivec2(0, 1)).g;

        weights.ba = SMAAArea(sqrt_d, e1, e2, 0.0);
    }

    imageStore(blendWeightsOutput, pixelCoord, weights);
}
