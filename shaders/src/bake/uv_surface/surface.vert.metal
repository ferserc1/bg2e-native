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
struct UvVertex { float4 position; float4 normal; float4 uv; uint4 identity; };
struct Transform { float4x4 model; float4x4 normal; };
struct Varyings {
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
    uint2 identity [[flat]];
};
vertex Varyings vertMain(uint index [[vertex_id]],
    constant Transform& transform [[buffer(2)]], const device UvVertex* vertices [[buffer(3)]])
{
    UvVertex item = vertices[index];
    Varyings out;
    // Match Vulkan: Metal's top-left viewport maps clip +Y to row zero, so v=0
    // is the first framebuffer row for the UV surface bake.
    out.position = float4(item.uv.x*2.0-1.0,1.0-item.uv.y*2.0,0.0,1.0);
    out.worldPosition = (transform.model*item.position).xyz;
    float3x3 normalMatrix(transform.normal[0].xyz,transform.normal[1].xyz,transform.normal[2].xyz);
    out.worldNormal = normalize(normalMatrix*item.normal.xyz);
    out.identity = item.identity.xy;
    return out;
}
