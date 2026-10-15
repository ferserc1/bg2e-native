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
#include "common.metal"
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]], texturecube<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    float3 sum = float3(0.0);
    for (uint i = 0u; i < params.samples; ++i)
    {
        float2 xi = hammersley(i, params.samples);
        float phi = 2.0 * PI * xi.x;
        float3 local = float3(cos(phi) * sqrt(xi.y), sin(phi) * sqrt(xi.y), sqrt(1.0 - xi.y));
        float3 l = tangentToWorld(local, n);
        float lod = clamp(0.5 * log2(max(6.0 * float(params.sourceSize) * float(params.sourceSize) / float(params.samples), 1.0)), 0.0, params.sourceMaxLod);
        sum += sourceImage.sample(sourceSampler, l, level(lod)).rgb;
    }
    return float4(sum * (PI / float(params.samples)), 1.0);
}
