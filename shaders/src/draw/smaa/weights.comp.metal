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

// SMAA_RT_METRICS = (1/width, 1/height, width, height)

//-----------------------------------------------------------------------------
// Horizontal / vertical search functions

float SMAASearchLength(float2 e,float offset,texture2d<float> image2, sampler linearSampler)
{
    // The image2 is flipped vertically, with the left/right cases taking
    // half of the horizontal space.
    float2 scale = SMAA_SEARCHTEX_SIZE * float2(0.5, -1.0);
    float2 bias = SMAA_SEARCHTEX_SIZE * float2(offset, 1.0);

    // Scale and bias to access texel centers.
    scale += float2(-1.0, 1.0);
    bias += float2(0.5, -0.5);

    // Convert from pixel coordinates to texcoords.
    scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
    bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;

    return image2.sample(linearSampler,scale * e + bias,level(0.0)).r;
}

float SMAASearchXLeft(float2 texcoord,float end,texture2d<float> image0, texture2d<float> image2, sampler linearSampler, float4 rtMetrics)
{
    float2 e = float2(0.0, 1.0);
    while (texcoord.x > end &&
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0)
    { // Or is there a crossing edge that breaks the line?
        e = image0.sample(linearSampler,texcoord,level(0.0)).rg;
        texcoord = -float2(2.0, 0.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e,0.0,image2,linearSampler) + 3.25;
    return rtMetrics.x * offset + texcoord.x;
}

float SMAASearchXRight(float2 texcoord,float end,texture2d<float> image0, texture2d<float> image2, sampler linearSampler, float4 rtMetrics)
{
    float2 e = float2(0.0, 1.0);
    while (texcoord.x < end &&
           e.g > 0.8281 &&
           e.r == 0.0)
    {
        e = image0.sample(linearSampler,texcoord,level(0.0)).rg;
        texcoord = float2(2.0, 0.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e,0.5,image2,linearSampler) + 3.25;
    return -rtMetrics.x * offset + texcoord.x;
}

float SMAASearchYUp(float2 texcoord,float end,texture2d<float> image0, texture2d<float> image2, sampler linearSampler, float4 rtMetrics)
{
    float2 e = float2(1.0, 0.0);
    while (texcoord.y > end &&
           e.r > 0.8281 &&
           e.g == 0.0)
    {
        e = image0.sample(linearSampler,texcoord,level(0.0)).rg;
        texcoord = -float2(0.0, 2.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e.gr,0.0,image2,linearSampler) + 3.25;
    return rtMetrics.y * offset + texcoord.y;
}

float SMAASearchYDown(float2 texcoord,float end,texture2d<float> image0, texture2d<float> image2, sampler linearSampler, float4 rtMetrics)
{
    float2 e = float2(1.0, 0.0);
    while (texcoord.y < end &&
           e.r > 0.8281 &&
           e.g == 0.0)
    {
        e = image0.sample(linearSampler,texcoord,level(0.0)).rg;
        texcoord = float2(0.0, 2.0) * rtMetrics.xy + texcoord;
    }
    float offset = -(255.0 / 127.0) * SMAASearchLength(e.gr,0.5,image2,linearSampler) + 3.25;
    return -rtMetrics.y * offset + texcoord.y;
}

//-----------------------------------------------------------------------------
// Area function

float2 SMAAArea(float2 dist,float e1,float e2,float offset,texture2d<float> image1, sampler linearSampler)
{
    // Rounding prevents precision errors of bilinear filtering.
    float2 texcoord = float2(SMAA_AREATEX_MAX_DISTANCE) * round(4.0 * float2(e1, e2)) + dist;

    // Scale and bias to map to texel space.
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Move to the proper place according to the subpixel offset.
    texcoord.y = SMAA_AREATEX_SUBTEX_SIZE * offset + texcoord.y;

    return image1.sample(linearSampler,texcoord,level(0.0)).rg;
}

//-----------------------------------------------------------------------------

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
    float2 pixcoord = float2(pixelCoord) + 0.5;

    // Offsets normally computed in the SMAA vertex shader.
    float4 offset0 = rtMetrics.xyxy * float4(-0.25, -0.125, 1.25, -0.125) + texcoord.xyxy;
    float4 offset1 = rtMetrics.xyxy * float4(-0.125, -0.25, -0.125, 1.25) + texcoord.xyxy;
    float4 offset2 = rtMetrics.xxyy * (float4(-2.0, 2.0, -2.0, 2.0) * float(SMAA_MAX_SEARCH_STEPS)) + float4(offset0.xz, offset1.yw);

    float4 weights = float4(0.0);

    float2 e = image0.sample(linearSampler,texcoord,level(0.0)).rg;

    if (e.g > 0.0)
    { // Edge at north
        float3 coords;
        float2 d;

        // Find the distance to the left.
        coords.x = SMAASearchXLeft(offset0.xy,offset2.x,image0,image2,linearSampler,rtMetrics);
        coords.y = offset1.y;
        d.x = coords.x;

        // Fetch the left crossing edges (bilinear, two at a time).
        float e1 = image0.sample(linearSampler,coords.xy,level(0.0)).r;

        // Find the distance to the right.
        coords.z = SMAASearchXRight(offset0.zw,offset2.y,image0,image2,linearSampler,rtMetrics);
        d.y = coords.z;

        // Distances in pixel units.
        d = abs(round(rtMetrics.zz * d - pixcoord.xx));

        // The area texture is compressed quadratically.
        float2 sqrt_d = sqrt(d);

        // Fetch the right crossing edges.
        float e2 = image0.sample(linearSampler,float2(coords.z, coords.y),level(0.0),int2(1, 0)).r;

        weights.rg = SMAAArea(sqrt_d,e1,e2,0.0,image1,linearSampler);
    }

    if (e.r > 0.0)
    { // Edge at west
        float3 coords;
        float2 d;

        // Find the distance upwards.
        coords.y = SMAASearchYUp(offset1.xy,offset2.z,image0,image2,linearSampler,rtMetrics);
        coords.x = offset0.x;
        d.x = coords.y;

        float e1 = image0.sample(linearSampler,coords.xy,level(0.0)).g;

        // Find the distance downwards.
        coords.z = SMAASearchYDown(offset1.zw,offset2.w,image0,image2,linearSampler,rtMetrics);
        d.y = coords.z;

        d = abs(round(rtMetrics.ww * d - pixcoord.yy));

        float2 sqrt_d = sqrt(d);

        float e2 = image0.sample(linearSampler,float2(coords.x, coords.z),level(0.0),int2(0, 1)).g;

        weights.ba = SMAAArea(sqrt_d,e1,e2,0.0,image1,linearSampler);
    }

    outputImage.write(weights,uint2(pixelCoord));
}
