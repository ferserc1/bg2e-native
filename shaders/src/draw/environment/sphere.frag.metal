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
float3 srgbToLinear(float3 color)
{
    float3 low = color / 12.92;
    float3 high = pow(max((color + 0.055) / 1.055, float3(0.0)), float3(2.4));
    return select(high, low, color <= 0.04045);
}
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]], texture2d<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    float2 uv = float2(atan2(n.z, n.x) / (2.0 * PI) + 0.5, acos(clamp(n.y, -1.0, 1.0)) / PI);
    float3 encoded = sourceImage.sample(sourceSampler, uv).rgb;
    float3 linear = params.roughness < 0.0 ? srgbToLinear(encoded) : encoded;
    return float4(linear, 1.0);
}
