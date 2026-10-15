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
};

struct PushConstants {
    packed_float3 cameraPos; // Match the host vec3 + float layout (16 bytes).
    float lod;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant PushConstants& push [[buffer(0)]],
                         texturecube<float> uCubeMap [[texture(0)]],
                         sampler uSampler [[sampler(0)]])
{
    float3 N = normalize(in.normal);
    float3 I = normalize(in.worldPos - float3(push.cameraPos));
    float3 R = reflect(I, N);
    return uCubeMap.sample(uSampler, R, level(push.lod));
}
