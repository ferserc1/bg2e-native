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

// SMAA neighborhood blending pass (third pass).
//
// Faithful port of the reference SMAANeighborhoodBlendingPS (Jimenez et al.,
// http://www.iryoku.com/smaa/). Gathers the blend weights of the current pixel
// and its right/bottom neighbors, picks the dominant blending direction and
// mixes the current pixel with the chosen neighbor via two bilinear fetches.

// SMAA_RT_METRICS = (1/width, 1/height, width, height)

// Conditional move helpers (replace components where cond is true).
void SMAAMovc2(bool2 cond, thread float2& v, float2 value)
{
    if (cond.x)
    {
        v.x = value.x;
    }
    if (cond.y)
    {
        v.y = value.y;
    }
}

void SMAAMovc4(bool4 cond, thread float4& v, float4 value)
{
    if (cond.x)
    {
        v.x = value.x;
    }
    if (cond.y)
    {
        v.y = value.y;
    }
    if (cond.z)
    {
        v.z = value.z;
    }
    if (cond.w)
    {
        v.w = value.w;
    }
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
    float4 offset = rtMetrics.xyxy * float4(1.0, 0.0, 0.0, 1.0) + texcoord.xyxy;

    // Fetch the blending weights for the current pixel.
    float4 a;
    a.x = image1.sample(linearSampler,offset.xy,level(0.0)).a;  // Right
    a.y = image1.sample(linearSampler,offset.zw,level(0.0)).g;  // Top
    a.wz = image1.sample(linearSampler,texcoord,level(0.0)).xz; // Bottom / Left

    // No blending weights: pass the color through unchanged.
    if (dot(a, float4(1.0)) < 1e-5)
    {
        float4 color = image0.sample(linearSampler,texcoord,level(0.0));
        outputImage.write(float4(color.rgb, 1.0),uint2(pixelCoord));
        return;
    }

    bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)

    // Calculate the blending offsets.
    float4 blendingOffset = float4(0.0, a.y, 0.0, a.w);
    float2 blendingWeight = a.yw;
    SMAAMovc4(bool4(h), blendingOffset, float4(a.x, 0.0, a.z, 0.0));
    SMAAMovc2(bool2(h), blendingWeight, a.xz);
    blendingWeight /= dot(blendingWeight, float2(1.0, 1.0));

    // Calculate the texture coordinates.
    float4 blendingCoord = blendingOffset * float4(rtMetrics.xy, -rtMetrics.xy) + texcoord.xyxy;

    // Exploit bilinear filtering to mix the current pixel with the chosen neighbor.
    float4 color = blendingWeight.x * image0.sample(linearSampler,blendingCoord.xy,level(0.0));
    color += blendingWeight.y * image0.sample(linearSampler,blendingCoord.zw,level(0.0));

    outputImage.write(float4(color.rgb, 1.0),uint2(pixelCoord));
}
