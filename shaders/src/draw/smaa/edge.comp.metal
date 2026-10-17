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

#include <metal_stdlib>
using namespace metal;
struct SMAAUniforms { float4 metrics; float4 parameters; };
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
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / float2(160.0, 560.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)
#define SMAA_SEARCHTEX_SIZE float2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE float2(64.0, 16.0)

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

float luma(float2 uv,texture2d<float> image0, sampler linearSampler)
{
    const float3 w = float3(0.2126, 0.7152, 0.0722);
    return dot(image0.sample(linearSampler,uv,level(0.0)).rgb, w);
}

kernel void compMain(uint2 grid [[thread_position_in_grid]],
    constant SMAAUniforms& pc [[buffer(1)]],
    texture2d<float> image0 [[texture(0)]],
    texture2d<float> image1 [[texture(1)]],
    texture2d<float> image2 [[texture(2)]],
    sampler linearSampler [[sampler(0)]],
    texture2d<float,access::write> outputImage [[texture(3)]])
{
    int2 size = int2(outputImage.get_width(),outputImage.get_height());
    int2 pixelCoord = int2(grid);
    if (pixelCoord.x >= size.x || pixelCoord.y >= size.y)
    {
        return;
    }

    float4 rtMetrics = pc.metrics;
    float2 texcoord = (float2(pixelCoord) + 0.5) * rtMetrics.xy;

    // Neighbor offsets normally computed in the SMAA vertex shader.
    float4 offset0 = rtMetrics.xyxy * float4(-1.0, 0.0, 0.0, -1.0) + texcoord.xyxy; // left / top
    float4 offset1 = rtMetrics.xyxy * float4(1.0, 0.0, 0.0, 1.0) + texcoord.xyxy;   // right / bottom
    float4 offset2 = rtMetrics.xyxy * float4(-2.0, 0.0, 0.0, -2.0) + texcoord.xyxy; // leftleft / toptop

    float2 threshold = float2(SMAA_THRESHOLD);

    // Luma of the current pixel and its left/top neighbors.
    float L = luma(texcoord,image0,linearSampler);
    float Lleft = luma(offset0.xy,image0,linearSampler);
    float Ltop = luma(offset0.zw,image0,linearSampler);

    float4 delta;
    delta.xy = abs(L - float2(Lleft, Ltop));
    float2 edges = step(threshold, delta.xy);

    // No edge here: write zero (the edges image is not cleared each frame).
    if (dot(edges, float2(1.0)) == 0.0)
    {
        outputImage.write(float4(0.0),uint2(pixelCoord));
        return;
    }

    // Right / bottom deltas.
    float Lright = luma(offset1.xy,image0,linearSampler);
    float Lbottom = luma(offset1.zw,image0,linearSampler);
    delta.zw = abs(L - float2(Lright, Lbottom));

    // Maximum delta in the direct neighborhood.
    float2 maxDelta = max(delta.xy, delta.zw);

    // Left-left / top-top deltas.
    float Lleftleft = luma(offset2.xy,image0,linearSampler);
    float Ltoptop = luma(offset2.zw,image0,linearSampler);
    delta.zw = abs(float2(Lleft, Ltop) - float2(Lleftleft, Ltoptop));

    // Final maximum delta.
    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation.
    edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

    outputImage.write(float4(edges, 0.0, 0.0),uint2(pixelCoord));
}
