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

#ifndef BLUE_NOISE_GLSL
#define BLUE_NOISE_GLSL

// Exact texel fetch preserves the ranked blue-noise values. Linear filtering
// blends neighboring ranks and changes both their histogram and spectrum.
// Independent precomputed layers and a frame/sample-dependent Cranley-Patterson
// rotation provide different samples during temporal accumulation.
vec2 bnRand2(sampler2DArray blueNoiseTex, ivec2 pixel, uint frameIndex, uint sampleIndex)
{
    ivec3 texSize = textureSize(blueNoiseTex, 0);

    const vec2 r2 = vec2(0.7548776662, 0.56984029);
    uint sequenceIndex = frameIndex * 233u + sampleIndex * 61u;
    // Bound the float conversion so long-running sessions keep fractional bits.
    vec2 shift = fract(r2 * float(sequenceIndex & 65535u));

    ivec2 texel = ivec2(
        (pixel.x % texSize.x + texSize.x) % texSize.x,
        (pixel.y % texSize.y + texSize.y) % texSize.y);
    int layer = int((frameIndex + sampleIndex * 7u) % uint(texSize.z));
    vec2 v = texelFetch(blueNoiseTex, ivec3(texel, layer), 0).rg;
    // R8G8_UNORM maps 255 to 1.0; sample bin centers in [0,1) instead.
    v = (v * 255.0 + 0.5) / 256.0;
    return fract(v + shift);
}

// Cosine-weighted hemisphere direction from two uniform random values.
// Identical mapping to randomHemisphereDirection() in deferred_utils.glsl,
// but taking explicit xi values so it can be fed by bnRand2().
vec3 bnHemisphereDirection(vec3 normal, vec2 xi)
{
    float r = sqrt(xi.x);
    float phi = 6.28318530718 * xi.y;

    vec3 localDir = vec3(
        r * cos(phi),
        r * sin(phi),
        sqrt(max(0.0, 1.0 - xi.x))
    );

    vec3 up = abs(normal.x) < 0.999
        ? vec3(0.0, 0.0, 1.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(mat3(tangent, bitangent, normal) * localDir);
}

#endif
