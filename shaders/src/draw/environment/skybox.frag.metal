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
struct SkyboxPush {
    float4x4 inverseViewProjection;
    float4 parameters;
};
fragment float4 fragMain(float4 position [[position]], constant SkyboxPush& params [[buffer(0)]],
    texturecube<float> sky [[texture(0)]], sampler skySampler [[sampler(0)]])
{
    float2 uv = position.xy / params.parameters.xy;
    float4 world = params.inverseViewProjection * float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    float3 direction = normalize(world.xyz);
    return float4(sky.sample(skySampler, direction, level(params.parameters.z)).rgb * exp2(params.parameters.w), 1.0);
}
