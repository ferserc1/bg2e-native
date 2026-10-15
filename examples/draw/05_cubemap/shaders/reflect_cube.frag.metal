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

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
    float3 localPos;
};

struct PushConstants {
    float3 cameraPos;
};

struct CubeRenderSettings {
    uint mode;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant PushConstants& push [[buffer(0)]],
                         constant CubeRenderSettings& settings [[buffer(3)]],
                         texturecube<float> uCubeMap [[texture(0)]],
                         sampler uSampler [[sampler(0)]])
{
    if (settings.mode == 0)
    {
        float3 N = normalize(in.normal);
        float3 I = normalize(in.worldPos - push.cameraPos);
        float3 R = reflect(I, N);
        return uCubeMap.sample(uSampler, R);
    }
    else
    {
        float3 D = normalize(in.localPos);
        return uCubeMap.sample(uSampler, D);
    }
}
