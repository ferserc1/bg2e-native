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
struct ShadowTriangleUV {
    float4 a;
    float4 b;
    float4 c;
};
struct ShadowUniforms {
    float4x4 inverseViewProjection;
    float4 parameters;
    uint4 counts;
    DrawLightUniforms lights;
};
float sampleAlpha(uint index, float2 uv, texture2d<float> alpha0, sampler alphaSampler0, texture2d<float> alpha1, sampler alphaSampler1, texture2d<float> alpha2, sampler alphaSampler2, texture2d<float> alpha3, sampler alphaSampler3, texture2d<float> alpha4, sampler alphaSampler4, texture2d<float> alpha5, sampler alphaSampler5, texture2d<float> alpha6, sampler alphaSampler6, texture2d<float> alpha7, sampler alphaSampler7)
{
    if (index == 0u)
    {
        return alpha0.sample(alphaSampler0, uv, level(0.0)).a;
    }
    if (index == 1u)
    {
        return alpha1.sample(alphaSampler1, uv, level(0.0)).a;
    }
    if (index == 2u)
    {
        return alpha2.sample(alphaSampler2, uv, level(0.0)).a;
    }
    if (index == 3u)
    {
        return alpha3.sample(alphaSampler3, uv, level(0.0)).a;
    }
    if (index == 4u)
    {
        return alpha4.sample(alphaSampler4, uv, level(0.0)).a;
    }
    if (index == 5u)
    {
        return alpha5.sample(alphaSampler5, uv, level(0.0)).a;
    }
    if (index == 6u)
    {
        return alpha6.sample(alphaSampler6, uv, level(0.0)).a;
    }
    if (index == 7u)
    {
        return alpha7.sample(alphaSampler7, uv, level(0.0)).a;
    }
    return 0.0;
}
bool acceptsCandidate(uint instance, uint primitive, float2 bary,
    constant ShadowUniforms& dataBlock, const device ShadowInstanceData* instances,
    const device ShadowTriangleUV* triangles, texture2d<float> alpha0, sampler alphaSampler0, texture2d<float> alpha1, sampler alphaSampler1, texture2d<float> alpha2, sampler alphaSampler2, texture2d<float> alpha3, sampler alphaSampler3, texture2d<float> alpha4, sampler alphaSampler4, texture2d<float> alpha5, sampler alphaSampler5, texture2d<float> alpha6, sampler alphaSampler6, texture2d<float> alpha7, sampler alphaSampler7)
{
    if (instance >= dataBlock.counts.y)
    {
        return false;
    }
    ShadowInstanceData data = instances[instance];
    if (data.ranges.z == 0xffffffffu)
    {
        return true;
    }
    if (primitive >= data.ranges.y)
    {
        return false;
    }
    ShadowTriangleUV triangle = triangles[data.ranges.x + primitive];
    float4 interpolated = triangle.a * (1.0 - bary.x - bary.y) + triangle.b * bary.x + triangle.c * bary.y;
    float2 uv = (data.options.x == 0u ? interpolated.xy : interpolated.zw) * data.alpha.xy;
    return sampleAlpha(data.ranges.z, uv, alpha0, alphaSampler0, alpha1, alphaSampler1, alpha2, alphaSampler2, alpha3, alphaSampler3, alpha4, alphaSampler4, alpha5, alphaSampler5, alpha6, alphaSampler6, alpha7, alphaSampler7) * data.alpha.w >= data.alpha.z;
}
bool visible(float3 origin, float3 direction, float distance,
    instance_acceleration_structure topLevel, constant ShadowUniforms& dataBlock,
    const device ShadowInstanceData* instances, const device ShadowTriangleUV* triangles,
    texture2d<float> alpha0, sampler alphaSampler0, texture2d<float> alpha1, sampler alphaSampler1, texture2d<float> alpha2, sampler alphaSampler2, texture2d<float> alpha3, sampler alphaSampler3, texture2d<float> alpha4, sampler alphaSampler4, texture2d<float> alpha5, sampler alphaSampler5, texture2d<float> alpha6, sampler alphaSampler6, texture2d<float> alpha7, sampler alphaSampler7)
{
    ray shadowRay;
    shadowRay.origin = origin;
    shadowRay.direction = direction;
    shadowRay.min_distance = dataBlock.parameters.x;
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
                query.get_candidate_triangle_barycentric_coord(), dataBlock, instances, triangles,
                alpha0, alphaSampler0, alpha1, alphaSampler1, alpha2, alphaSampler2, alpha3, alphaSampler3, alpha4, alphaSampler4, alpha5, alphaSampler5, alpha6, alphaSampler6, alpha7, alphaSampler7))
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
kernel void compMain(
    uint2 pixelIndex [[thread_position_in_grid]],
    constant ShadowUniforms& dataBlock [[buffer(1)]],
    const device ShadowInstanceData* instances [[buffer(2)]],
    const device ShadowTriangleUV* triangles [[buffer(3)]],
    instance_acceleration_structure topLevel [[buffer(4)]],
    texture2d<float> normalImage [[texture(0)]],
    depth2d<float> depthImage [[texture(1)]],
    texture2d<float, access::write> shadow0 [[texture(2)]],
    texture2d<float, access::write> shadow1 [[texture(3)]],
    texture2d<float> alpha0 [[texture(4)]],
    sampler alphaSampler0 [[sampler(0)]],
    texture2d<float> alpha1 [[texture(5)]],
    sampler alphaSampler1 [[sampler(1)]],
    texture2d<float> alpha2 [[texture(6)]],
    sampler alphaSampler2 [[sampler(2)]],
    texture2d<float> alpha3 [[texture(7)]],
    sampler alphaSampler3 [[sampler(3)]],
    texture2d<float> alpha4 [[texture(8)]],
    sampler alphaSampler4 [[sampler(4)]],
    texture2d<float> alpha5 [[texture(9)]],
    sampler alphaSampler5 [[sampler(5)]],
    texture2d<float> alpha6 [[texture(10)]],
    sampler alphaSampler6 [[sampler(6)]],
    texture2d<float> alpha7 [[texture(11)]],
    sampler alphaSampler7 [[sampler(7)]])
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
            DrawLightData light = dataBlock.lights.lights[i];
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
                        visibility += visible(origin, sampleDirection, distance, topLevel, dataBlock, instances, triangles, alpha0, alphaSampler0, alpha1, alphaSampler1, alpha2, alphaSampler2, alpha3, alphaSampler3, alpha4, alphaSampler4, alpha5, alphaSampler5, alpha6, alphaSampler6, alpha7, alphaSampler7) ? 1.0 : 0.0;
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
