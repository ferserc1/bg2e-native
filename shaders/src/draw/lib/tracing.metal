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

#ifndef BG2E_DRAW_TRACING_METAL
#define BG2E_DRAW_TRACING_METAL
#include "bindings.metal"
#include <metal_raytracing>
using namespace raytracing;
struct TraceVertex { float4 position; float4 normal; float4 uv; float4 tangent; };
struct TraceInstance { float4x4 model; float4x4 normal; uint4 ranges; uint4 identity; };
struct TraceMaterial { DrawMaterialData values; uint4 images[2]; uint4 samplers[2]; };
struct TraceArguments {
    array<texture2d<float>,TRACE_CAPACITY> images [[id(0)]];
    array<sampler,TRACE_CAPACITY> samplers [[id(TRACE_CAPACITY)]];
};
struct TraceData {
    const device TraceVertex* vertices;
    const device uint* indices;
    const device TraceInstance* instances;
    const device TraceMaterial* materials;
    constant TraceArguments* resources;
};
float4 traceUV(uint instance, uint primitive, float2 bary, thread const TraceData& data)
{
    TraceInstance record = data.instances[instance];
    uint first = record.ranges.x+primitive*3u;
    TraceVertex a = data.vertices[record.ranges.y+data.indices[first]];
    TraceVertex b = data.vertices[record.ranges.y+data.indices[first+1u]];
    TraceVertex c = data.vertices[record.ranges.y+data.indices[first+2u]];
    return a.uv*(1.0-bary.x-bary.y)+b.uv*bary.x+c.uv*bary.y;
}
float4 traceSample(uint materialIndex, uint slot, float4 uv, thread const TraceData& data)
{
    TraceMaterial material = data.materials[materialIndex];
    uint imageIndex = material.images[slot/4u][slot%4u];
    uint samplerIndex = material.samplers[slot/4u][slot%4u];
    if (imageIndex == 0xffffffffu || samplerIndex == 0xffffffffu)
    {
        // No argument-buffer access or sRGB conversion for an absent texture.
        return slot == 3u ? float4(0.5,0.5,1.0,1.0) : float4(1.0);
    }
    float2 selected = drawSelectUV(material.values,slot,uv.xy,uv.zw);
    return drawDecodeSample(material.values,slot,
        data.resources->images[imageIndex].sample(data.resources->samplers[samplerIndex],selected,level(0.0)));
}
bool traceAccept(uint instance, uint primitive, float2 bary, uint instanceCount, thread const TraceData& data)
{
    if (instance >= instanceCount || primitive >= data.instances[instance].ranges.z)
    {
        return false;
    }
    uint material = data.instances[instance].ranges.w;
    if ((data.materials[material].values.uvSets[1].z & 2u) == 0u)
    {
        return true;
    }
    float alpha = traceSample(material,0u,traceUV(instance,primitive,bary,data),data).a * data.materials[material].values.albedo.a;
    return alpha >= data.materials[material].values.emissionAlpha.y;
}
float3 traceGeometricNormal(uint instance, uint primitive, float3 direction, thread const TraceData& data)
{
    TraceInstance record = data.instances[instance];
    uint first = record.ranges.x+primitive*3u;
    float3 a = data.vertices[record.ranges.y+data.indices[first]].position.xyz;
    float3 b = data.vertices[record.ranges.y+data.indices[first+1u]].position.xyz;
    float3 c = data.vertices[record.ranges.y+data.indices[first+2u]].position.xyz;
    float3 n = (record.normal*float4(cross(b-a,c-a),0)).xyz;
    n = dot(n,n) > 1e-16 ? normalize(n) : -direction;
    return dot(n,direction) > 0.0 ? -n : n;
}
struct TraceHit { bool hit; float distance; float3 normal; };
TraceHit traceNearest(float3 origin, float3 direction, float minimum, float maximum, uint instanceCount,
    instance_acceleration_structure scene, thread const TraceData& data)
{
    ray queryRay;
    queryRay.origin = origin;
    queryRay.direction = direction;
    queryRay.min_distance = minimum;
    queryRay.max_distance = maximum;
    intersection_params parameters;
    parameters.assume_geometry_type(geometry_type::triangle);
    parameters.accept_any_intersection(false);
    intersection_query<instancing,triangle_data> query(queryRay,scene,255u,parameters);
    while (query.next())
    {
        if (query.get_candidate_intersection_type() == intersection_type::triangle &&
            traceAccept(query.get_candidate_instance_id(),query.get_candidate_primitive_id(),
                query.get_candidate_triangle_barycentric_coord(),instanceCount,data))
        {
            query.commit_triangle_intersection();
        }
    }
    TraceHit result;
    result.hit = query.get_committed_intersection_type() == intersection_type::triangle;
    result.distance = maximum;
    result.normal = -direction;
    if (result.hit)
    {
        result.distance = query.get_committed_distance();
        result.normal = traceGeometricNormal(query.get_committed_instance_id(),query.get_committed_primitive_id(),direction,data);
    }
    return result;
}
#endif
