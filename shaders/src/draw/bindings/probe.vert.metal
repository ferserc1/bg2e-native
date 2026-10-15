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
#include "../lib/bindings.metal"
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv0 [[attribute(2)]];
    float2 uv1 [[attribute(3)]];
    float3 tangent [[attribute(4)]];
};
struct VertexOut {
    float4 position [[position]];
    float2 uv0;
    float2 uv1;
};
vertex VertexOut vertMain(VertexIn in [[stage_in]], DRAW_VERTEX_DATA_ARGUMENTS)
{
    VertexOut out;
    out.uv0 = in.uv0;
    out.uv1 = in.uv1;
    out.position = drawFrame.projection * drawFrame.view * drawObject.model * float4(in.position, 1.0);
    return out;
}
