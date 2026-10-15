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

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
};

struct CameraUBO {
    float4x4 projection;
    float4x4 view;
    float4 cameraPos;
};

struct ModelUBO {
    float4x4 model;
};

vertex VertexOut vertMain(VertexIn in [[stage_in]],
                          constant CameraUBO& camera [[buffer(2)]],
                          constant ModelUBO& model [[buffer(3)]])
{
    VertexOut out;
    float4 worldPos = model.model * float4(in.position, 1.0);
    float3x3 normalMatrix = float3x3(
        model.model[0].xyz,
        model.model[1].xyz,
        model.model[2].xyz);

    out.position = camera.projection * camera.view * worldPos;
    out.worldPos = worldPos.xyz;
    out.normal = normalize(normalMatrix * in.normal);
    return out;
}
