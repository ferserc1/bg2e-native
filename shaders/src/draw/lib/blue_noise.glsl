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

#ifndef BG2E_DRAW_BLUE_NOISE_GLSL
#define BG2E_DRAW_BLUE_NOISE_GLSL

uint bg2eNoiseHash(uint value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    return value ^ (value >> 16u);
}

// Deterministic alternative; this hash does not have blue-noise spatial quality.
vec2 bg2eHashNoise2(ivec2 pixel, uint renderedFrame, uint sampleIndex)
{
    uint seed = bg2eNoiseHash(uint(pixel.x)) ^ bg2eNoiseHash(uint(pixel.y) + 0x9e3779b9u) ^
        bg2eNoiseHash(renderedFrame) ^ bg2eNoiseHash(sampleIndex + 0x85ebca6bu);
    return vec2(bg2eNoiseHash(seed) >> 9u, bg2eNoiseHash(seed ^ 0x68bc21ebu) >> 9u) / 8388608.0;
}

// Metadata: tile width/height, source channels, logical layers. The atlas is
// sampled color 2D RGBA8 UNORM (not sRGB), one mip, logical layers stacked in Y.
// Separate handles keep the combined sampler constructor at the texture operation.
// Disabled/missing noise calls bg2eHashNoise2 without unbound resources.
vec2 bg2eBlueNoise2(texture2D atlas, sampler atlasSampler, uvec4 metadata,
                    ivec2 pixel, uint renderedFrame, uint sampleIndex)
{
    ivec2 tile = ivec2(metadata.xy);
    ivec2 wrapped = pixel % tile;
    if (wrapped.x < 0)
    {
        wrapped.x += tile.x;
    }
    if (wrapped.y < 0)
    {
        wrapped.y += tile.y;
    }
    uint layer = (renderedFrame + sampleIndex * 7u) % metadata.w;
    ivec2 texel = ivec2(wrapped.x, wrapped.y + int(layer * metadata.y));
    vec2 ranks = floor(texelFetch(sampler2D(atlas, atlasSampler), texel, 0).rg * 255.0 + 0.5);
    vec2 bins = (ranks + 0.5) / 256.0;
    uint sequence = (renderedFrame * 233u + sampleIndex * 61u) & 65535u;
    vec2 shift = fract(vec2(0.7548776662, 0.56984029) * float(sequence));
    return fract(bins + shift);
}
#endif
