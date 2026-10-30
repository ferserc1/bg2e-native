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
#include "../lib/solar.metal"
struct SkyboxPush {
    float4x4 inverseViewProjection;
    float4 parameters;
    float4 sunDirectionSize;
    float4 solarParameters;
    float4 sunIrradiance;
};
fragment float4 fragMain(float4 position [[position]], constant SkyboxPush& params [[buffer(0)]],
    texturecube<float> sky [[texture(0)]], sampler skySampler [[sampler(0)]])
{
    float2 uv = position.xy / params.parameters.xy;
    float4 world = params.inverseViewProjection * float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    float3 direction = normalize(world.xyz);
    float3 color = sky.sample(skySampler, direction, level(params.parameters.z)).rgb;
    // Chord distance is stable for the small physical solar angle. Evaluate
    // coverage at screen resolution rather than magnifying cubemap texels.
    float chord = length(direction - params.sunDirectionSize.xyz);
    float radius = 2.0 * sin(0.5 * solarAngularRadius(params.sunDirectionSize.w, params.solarParameters.z));
    float edge = max(0.5 * (length(dfdx(direction)) + length(dfdy(direction))), 1e-7);
    float disk = 1.0 - smoothstep(max(radius - edge, 0.0), radius + edge, chord);
    float horizonEdge = max(0.5 * fwidth(direction.y), 1e-7);
    float horizon = smoothstep(-horizonEdge, horizonEdge, direction.y);
    color += params.sunIrradiance.rgb * solarDiskNormalization(params.sunDirectionSize.w, params.solarParameters.z) * disk * horizon * params.sunIrradiance.w;
    return float4(color * exp2(params.parameters.w), 1.0);
}
