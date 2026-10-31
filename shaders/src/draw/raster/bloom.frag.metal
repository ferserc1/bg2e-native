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

#include <metal_stdlib>
using namespace metal;

struct BloomPush {
    float4 extent;
    float4 parameters;
};

float3 sampleSource(float2 uv, texture2d<float> sourceImage, sampler sourceSampler)
{
    return sourceImage.sample(sourceSampler, uv, level(0.0)).rgb;
}

float3 extractBright(int2 pixel, texture2d<float> sourceImage, constant BloomPush& settings)
{
    int2 size = int2(sourceImage.get_width(), sourceImage.get_height());
    float3 c = sourceImage.read(uint2(clamp(pixel, int2(0), size - 1))).rgb;
    // Prevent nonfinite highlights from contaminating the complete pyramid.
    c = float3(isnan(c.r) ? 0.0 : c.r, isnan(c.g) ? 0.0 : c.g, isnan(c.b) ? 0.0 : c.b);
    c = clamp(c, float3(0.0), float3(65504.0));
    float brightness = max(c.r, max(c.g, c.b));
    return c * (max(brightness - settings.parameters.x, 0.0) / max(brightness, 1e-6));
}

fragment float4 fragMain(float4 position [[position]], constant BloomPush& settings [[buffer(0)]],
    texture2d<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 uv = position.xy / settings.extent.xy;
    float2 texel = 1.0 / float2(int2(sourceImage.get_width(), sourceImage.get_height()));
    float3 color = float3(0.0);
    if (settings.extent.w < 0.5)
    {
        // Threshold individual HDR texels BEFORE averaging: a small saturated
        // highlight must not disappear into its darker neighbours at quarter size.
        int divisor = int(settings.extent.z);
        int2 base = int2(position.xy) * divisor;
        for (int y = 0; y < divisor; ++y)
        {
            for (int x = 0; x < divisor; ++x)
            {
                color += extractBright(base + int2(x, y), sourceImage, settings) / float(divisor * divisor);
            }
        }
    }
    else if (settings.extent.w < 1.5)
    {
        // Dual downsample: centre and four diagonals, using bilinear filtering.
        color = sampleSource(uv, sourceImage, sourceSampler) * 0.5;
        color += sampleSource(uv + texel * float2(-1.0, -1.0), sourceImage, sourceSampler) * 0.125;
        color += sampleSource(uv + texel * float2( 1.0, -1.0), sourceImage, sourceSampler) * 0.125;
        color += sampleSource(uv + texel * float2(-1.0,  1.0), sourceImage, sourceSampler) * 0.125;
        color += sampleSource(uv + texel * float2( 1.0,  1.0), sourceImage, sourceSampler) * 0.125;
    }
    else
    {
        // Dual upsample: eight bilinear taps. Fixed-function alpha blending
        // combines this coarse level with the retained fine level, without feedback.
        color = sampleSource(uv + texel * float2(-1.0,  0.0), sourceImage, sourceSampler) / 12.0;
        color += sampleSource(uv + texel * float2( 1.0,  0.0), sourceImage, sourceSampler) / 12.0;
        color += sampleSource(uv + texel * float2( 0.0, -1.0), sourceImage, sourceSampler) / 12.0;
        color += sampleSource(uv + texel * float2( 0.0,  1.0), sourceImage, sourceSampler) / 12.0;
        color += sampleSource(uv + texel * float2(-0.5, -0.5), sourceImage, sourceSampler) / 6.0;
        color += sampleSource(uv + texel * float2( 0.5, -0.5), sourceImage, sourceSampler) / 6.0;
        color += sampleSource(uv + texel * float2(-0.5,  0.5), sourceImage, sourceSampler) / 6.0;
        color += sampleSource(uv + texel * float2( 0.5,  0.5), sourceImage, sourceSampler) / 6.0;
    }
    return float4(color, settings.extent.w > 1.5 ? settings.parameters.y : 1.0);
}
