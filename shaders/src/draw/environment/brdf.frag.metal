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
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]])
{
    float2 pixel = position.xy;
    float2 uv = pixel / float(params.size);
    float nv = clamp(uv.x, 0.0001, 0.9999);
    float roughness = clamp(uv.y, 0.0001, 0.9999);
    float3 v = float3(sqrt(1.0 - nv * nv), 0.0, nv);
    float2 result = float2(0.0);
    for (uint i = 0u; i < params.samples; ++i)
    {
        float3 h = sampleGGX(hammersley(i, params.samples), float3(0, 0, 1), roughness);
        float3 l = normalize(2.0 * dot(v, h) * h - v);
        float nl = max(l.z, 0.0);
        float nh = max(h.z, 0.0);
        float vh = clamp(dot(v, h), 0.0, 1.0);
        if (nl > 0.0)
        {
            float g = geometryIBL(nv, roughness) * geometryIBL(nl, roughness);
            float visibility = g * vh / max(nh * nv, 0.000001);
            float fc = pow(1.0 - vh, 5.0);
            result += float2((1.0 - fc) * visibility, fc * visibility);
        }
    }
    return float4(result / float(params.samples), 0.0, 1.0);
}
