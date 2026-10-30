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
#include <metal_raytracing>
#include "../lib/bindings.metal"
using namespace metal;
using namespace raytracing;
struct ShadowInstanceData {
    uint4 ranges;
    float4 alpha;
    uint4 options;
};
// Matches the immutable TracingVertex buffer shared by all tracing consumers.
struct ShadowVertex {
    float4 position;
    float4 normal;
    float4 uv;
    float4 tangent;
};
struct ShadowUniforms {
    float4x4 inverseViewProjection;
    float4 parameters;
    uint4 counts;
    DrawLightUniforms lights;
};
struct ShadowImageArgument { texture2d<float> image [[id(0)]]; };
struct ShadowSamplerArgument { sampler state [[id(0)]]; };
float sampleAlpha(uint index, float2 uv, constant ShadowImageArgument* images,
    constant ShadowSamplerArgument* samplers)
{
    return images[index].image.sample(samplers[index].state, uv, level(0.0)).a;
}
bool acceptsCandidate(uint instance, uint primitive, float2 bary,
    constant ShadowUniforms& dataBlock, const device ShadowInstanceData* instances,
    const device ShadowVertex* vertices, const device uint* indices, constant ShadowImageArgument* images,
    constant ShadowSamplerArgument* samplers)
{
    if (instance >= dataBlock.counts.y)
    {
        return false;
    }
    ShadowInstanceData data = instances[instance];
    if ((data.options.y & 2u) == 0u)
    {
        // CPU-packed classification: skip UV/index fetches and alpha sampling.
        return true;
    }
    if (data.ranges.z == 0xffffffffu)
    {
        return data.alpha.w >= data.alpha.z;
    }
    if (primitive >= data.ranges.y)
    {
        return false;
    }
    uint first = data.ranges.x + primitive * 3u;
    float4 a = vertices[data.options.z + indices[first]].uv;
    float4 b = vertices[data.options.z + indices[first + 1u]].uv;
    float4 c = vertices[data.options.z + indices[first + 2u]].uv;
    float4 interpolated = a * (1.0 - bary.x - bary.y) + b * bary.x + c * bary.y;
    float2 uv = (data.options.x == 0u ? interpolated.xy : interpolated.zw) * data.alpha.xy;
    return sampleAlpha(data.ranges.z, uv, images, samplers) * data.alpha.w >= data.alpha.z;
}
bool visible(float3 origin, float3 direction, float distance,
    instance_acceleration_structure topLevel, constant ShadowUniforms& dataBlock,
    const device ShadowInstanceData* instances, const device ShadowVertex* vertices, const device uint* indices,
    constant ShadowImageArgument* images, constant ShadowSamplerArgument* samplers)
{
    ray shadowRay;
    shadowRay.origin = origin;
    shadowRay.direction = direction;
    // The origin is already offset by the bias; retain nearby blockers.
    shadowRay.min_distance = 1e-6;
    shadowRay.max_distance = distance;
    intersection_params params;
    params.assume_geometry_type(geometry_type::triangle);
    params.accept_any_intersection(true);
    // Instance opacity preserves fast opaque hits and exposes cutout candidates.
    intersection_query<instancing, triangle_data> query(shadowRay, topLevel, 0xffu, params);
    while (query.next())
    {
        if (query.get_candidate_intersection_type() == intersection_type::triangle &&
            acceptsCandidate(query.get_candidate_instance_id(), query.get_candidate_primitive_id(),
                query.get_candidate_triangle_barycentric_coord(), dataBlock, instances, vertices, indices,
                images, samplers))
        {
            query.commit_triangle_intersection();
        }
    }
    return query.get_committed_intersection_type() == intersection_type::none;
}
uint shadowHash(uint v)
{
    v ^= v >> 16u;
    v *= 0x7feb352du;
    v ^= v >> 15u;
    v *= 0x846ca68bu;
    return v ^ (v >> 16u);
}
float3 sampleLightDirection(float3 direction, float angle, uint index, uint count, uint seed)
{
    float3 up = abs(direction.z) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 t = normalize(cross(up, direction));
    float3 b = cross(direction, t);
    float radius = sqrt((float(index) + 0.5) / float(count)) * tan(min(angle, 1.4));
    float rotation = float(shadowHash(seed)) * (6.28318530718 / 4294967296.0);
    float phase = float(index) * 2.39996322973 + rotation;
    return normalize(direction + radius * (cos(phase) * t + sin(phase) * b));
}
// v5 splits invocation, scene and persistent alpha sets without changing Metal buffer/texture indices.
kernel void compMain(
    uint2 pixelIndex [[thread_position_in_grid]],
    constant ShadowUniforms& dataBlock [[buffer(1)]],
    const device ShadowInstanceData* instances [[buffer(2)]],
    const device ShadowVertex* vertices [[buffer(3)]],
    const device uint* indices [[buffer(7)]],
    instance_acceleration_structure topLevel [[buffer(4)]],
    texture2d<float> normalImage [[texture(0)]],
    depth2d<float> depthImage [[texture(1)]],
    texture2d<float, access::write> shadow0 [[texture(2)]],
    texture2d<float, access::write> shadow1 [[texture(3)]],
    constant ShadowImageArgument* images [[buffer(5)]],
    constant ShadowSamplerArgument* samplers [[buffer(6)]])
{
    uint2 pixel = pixelIndex;
    uint2 extent = uint2(shadow0.get_width(), shadow0.get_height());
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }
    float4 first = float4(1.0);
    float4 second = float4(1.0);
    float3 encoded = normalImage.read(pixel).xyz;
    if (dot(encoded, encoded) > 1e-8)
    {
        float3 normal = normalize(encoded * 2.0 - 1.0);
        float depth = depthImage.read(pixel);
        float2 uv = (float2(pixel) + 0.5) / float2(extent);
        float4 world = dataBlock.inverseViewProjection * float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
        float3 position = world.xyz / world.w;
        float bias = dataBlock.parameters.x;
        float3 origin = position + normal * bias;
        for (uint i = 0u; i < dataBlock.counts.x; ++i)
        {
            DrawLightData light = resolveSolarLight(dataBlock.lights.lights[i]);
            float visibility = 1.0;
            if (light.flags.y != 0u)
            {
                float3 delta = light.flags.x == 4u ? -light.directionAngle.xyz : light.positionIntensity.xyz - origin;
                float distance = light.flags.x == 4u ? dataBlock.parameters.y : length(delta) - bias;
                if (distance > bias && dot(delta, delta) > 1e-12)
                {
                    float3 direction = normalize(delta);
                    float angle = light.flags.x == 4u ? light.parameters.y * 0.01745329251994 : atan(light.parameters.y / max(distance, bias));
                    uint count = min(max(light.flags.z, 1u), dataBlock.counts.w);
                    visibility = 0.0;
                    uint seed = pixel.x + pixel.y * extent.x + i * 104729u;
                    for (uint sampleIndex = 0u; sampleIndex < count; ++sampleIndex)
                    {
                        float3 sampleDirection = count == 1u ? direction : sampleLightDirection(direction, angle, sampleIndex, count, seed);
                        visibility += visible(origin, sampleDirection, distance, topLevel, dataBlock, instances, vertices, indices, images, samplers) ? 1.0 : 0.0;
                    }
                    visibility /= float(count);
                }
            }
            if (i < 4u)
            {
                first[i] = visibility;
            }
            else
            {
                second[i - 4u] = visibility;
            }
        }
    }
    shadow0.write(first, pixel);
    shadow1.write(second, pixel);
}
