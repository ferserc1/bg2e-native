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

#ifndef BG2E_DRAW_BLUE_NOISE_METAL
#define BG2E_DRAW_BLUE_NOISE_METAL
#include <metal_stdlib>
using namespace metal;

uint bg2eNoiseHash(uint value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    return value ^ (value >> 16u);
}

float2 bg2eHashNoise2(int2 pixel, uint renderedFrame, uint sampleIndex)
{
    uint seed = bg2eNoiseHash(uint(pixel.x)) ^ bg2eNoiseHash(uint(pixel.y) + 0x9e3779b9u) ^
        bg2eNoiseHash(renderedFrame) ^ bg2eNoiseHash(sampleIndex + 0x85ebca6bu);
    return float2(bg2eNoiseHash(seed) >> 9u, bg2eNoiseHash(seed ^ 0x68bc21ebu) >> 9u) / 8388608.0;
}

// Same atlas and sequence convention as blue_noise.glsl. Exact reads preserve
// rank values. Bind the exported nearest/repeat sampler only if a caller uses
// normalized sampling; read() itself does not consume a native sampler index.
float2 bg2eBlueNoise2(texture2d<float> atlas, uint4 metadata,
                     int2 pixel, uint renderedFrame, uint sampleIndex)
{
    int2 tile = int2(metadata.xy);
    int2 wrapped = pixel % tile;
    if (wrapped.x < 0)
    {
        wrapped.x += tile.x;
    }
    if (wrapped.y < 0)
    {
        wrapped.y += tile.y;
    }
    uint layer = (renderedFrame + sampleIndex * 7u) % metadata.w;
    uint2 texel = uint2(wrapped.x, wrapped.y + int(layer * metadata.y));
    float2 ranks = floor(atlas.read(texel).rg * 255.0 + 0.5);
    float2 bins = (ranks + 0.5) / 256.0;
    uint sequence = (renderedFrame * 233u + sampleIndex * 61u) & 65535u;
    float2 shift = fract(float2(0.7548776662, 0.56984029) * float(sequence));
    return fract(bins + shift);
}
#endif
