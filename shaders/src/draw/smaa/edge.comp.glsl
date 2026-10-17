/*
 *    business grade graphic engine (bg2e engine)
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
layout(set=0,binding=0,std140) uniform SMAAUniforms
{
    vec4 metrics;
    vec4 parameters;
} pc;
layout(set=0,binding=1) uniform texture2D image0;
layout(set=0,binding=2) uniform texture2D image1;
layout(set=0,binding=3) uniform texture2D image2;
layout(set=0,binding=4) uniform sampler linearSampler;
layout(set=0,binding=5,rg8) uniform image2D outputImage;
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
#define SMAA_THRESHOLD pc.parameters.x
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0
#define SMAA_MAX_SEARCH_STEPS int(pc.parameters.y)
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25
#define SMAA_AREATEX_MAX_DISTANCE 16
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20
// Dimensions of the official iryku_smaa precomputed lookup tables:
//   AreaTex   : 160 x 560 (R8G8)
//   SearchTex :  64 x  16 (R8), addressed as a 66 x 33 logical grid
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / vec2(160.0, 560.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)
#define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0)

// SMAA luma edge detection pass (first pass).
//
// Faithful port of the reference SMAALumaEdgeDetectionPS (Jimenez et al.,
// http://www.iryoku.com/smaa/). Detects edges from the asymmetric luma gradient
// against the left/top neighbors and applies local contrast adaptation to
// suppress edges adjacent to much stronger ones.
//
//   edges.r = vertical edge   (|L - Lleft| above threshold)
//   edges.g = horizontal edge (|L - Ltop|  above threshold)

// SMAA_RT_METRICS = (1/width, 1/height, width, height)
vec4 rtMetrics;

float luma(vec2 uv)
{
    const vec3 w = vec3(0.2126, 0.7152, 0.0722);
    return dot(textureLod(sampler2D(image0,linearSampler),uv,0.0).rgb, w);
}

void main()
{
    ivec2 size = imageSize(outputImage);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= size.x || pixelCoord.y >= size.y)
    {
        return;
    }

    rtMetrics = pc.metrics;
    vec2 texcoord = (vec2(pixelCoord) + 0.5) * rtMetrics.xy;

    // Neighbor offsets normally computed in the SMAA vertex shader.
    vec4 offset0 = rtMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + texcoord.xyxy; // left / top
    vec4 offset1 = rtMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + texcoord.xyxy;   // right / bottom
    vec4 offset2 = rtMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + texcoord.xyxy; // leftleft / toptop

    vec2 threshold = vec2(SMAA_THRESHOLD);

    // Luma of the current pixel and its left/top neighbors.
    float L = luma(texcoord);
    float Lleft = luma(offset0.xy);
    float Ltop = luma(offset0.zw);

    vec4 delta;
    delta.xy = abs(L - vec2(Lleft, Ltop));
    vec2 edges = step(threshold, delta.xy);

    // No edge here: write zero (the edges image is not cleared each frame).
    if (dot(edges, vec2(1.0)) == 0.0)
    {
        imageStore(outputImage, pixelCoord, vec4(0.0));
        return;
    }

    // Right / bottom deltas.
    float Lright = luma(offset1.xy);
    float Lbottom = luma(offset1.zw);
    delta.zw = abs(L - vec2(Lright, Lbottom));

    // Maximum delta in the direct neighborhood.
    vec2 maxDelta = max(delta.xy, delta.zw);

    // Left-left / top-top deltas.
    float Lleftleft = luma(offset2.xy);
    float Ltoptop = luma(offset2.zw);
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

    // Final maximum delta.
    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation.
    edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

    imageStore(outputImage, pixelCoord, vec4(edges, 0.0, 0.0));
}
