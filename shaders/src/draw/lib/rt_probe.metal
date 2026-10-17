/*
 *    business grade graphic engine (bg2e engine)
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
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;
struct ProbeVertex { float4 position; float4 normal; float4 uv; float4 tangent; };
struct ProbeInstance { float4x4 model; float4x4 normal; uint4 ranges; uint4 identity; };
struct ProbeMaterial { uint4 images; float4 parameters; };
struct ProbeArguments {
    array<texture2d<float>, RT_COUNT> images [[id(0)]];
    array<sampler, RT_COUNT> samplers [[id(RT_COUNT)]];
};
float4 surface(uint instanceIndex, uint primitive, float2 bary, uint pixel,
    const device ProbeVertex* vertices, const device uint* indices,
    const device ProbeInstance* instances, const device ProbeMaterial* materials,
    constant ProbeArguments& resources)
{
    ProbeInstance instance = instances[instanceIndex];
    ProbeMaterial material = materials[instance.ranges.w];
    uint first = instance.ranges.x + 3u * primitive;
    ProbeVertex a = vertices[instance.ranges.y + indices[first]];
    ProbeVertex b = vertices[instance.ranges.y + indices[first+1u]];
    ProbeVertex c = vertices[instance.ranges.y + indices[first+2u]];
    float3 weights = float3(1.0-bary.x-bary.y, bary);
    float4 uv = a.uv*weights.x + b.uv*weights.y + c.uv*weights.z;
    uint imageIndex = material.images.x + pixel % material.images.y;
    uint samplerIndex = pixel % 3u == 0u ? 1u : 0u;
    float2 selected = material.images.w == 0u ? uv.xy : uv.zw;
    return resources.images[imageIndex].sample(resources.samplers[samplerIndex], selected, level(0.0));
}
