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

#define RT_COUNT 128
#include "../lib/rt_probe.metal"
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    texture2d<float, access::write> outputImage [[texture(0)]],
    const device ProbeVertex* vertices [[buffer(1)]],
    const device uint* indices [[buffer(2)]],
    const device ProbeInstance* instances [[buffer(3)]],
    const device ProbeMaterial* materials [[buffer(4)]],
    instance_acceleration_structure topLevel [[buffer(5)]],
    constant ProbeArguments& resources [[buffer(6)]])
{
    if (pixel.x >= 32u || pixel.y != 0u)
    {
        return;
    }
    ray probeRay;
    probeRay.origin = float3((float(pixel.x)+0.5)/32.0*0.8-0.4,-0.25,0);
    probeRay.direction = float3(0,0,1);
    probeRay.min_distance = 0.001;
    probeRay.max_distance = 4.0;
    intersection_params parameters;
    parameters.assume_geometry_type(geometry_type::triangle);
    parameters.accept_any_intersection(false);
    intersection_query<instancing, triangle_data> query(probeRay, topLevel, 255u, parameters);
    while (query.next())
    {
        if (query.get_candidate_intersection_type() == intersection_type::triangle)
        {
            uint instance = query.get_candidate_instance_id();
            float4 color = surface(instance, query.get_candidate_primitive_id(), query.get_candidate_triangle_barycentric_coord(),
                pixel.x, vertices, indices, instances, materials, resources);
            if (color.a >= materials[instances[instance].ranges.w].parameters.x)
            {
                query.commit_triangle_intersection();
            }
        }
    }
    float4 result = float4(1,0,1,1);
    if (query.get_committed_intersection_type() == intersection_type::triangle)
    {
        result = surface(query.get_committed_instance_id(), query.get_committed_primitive_id(),
            query.get_committed_triangle_barycentric_coord(), pixel.x, vertices, indices, instances, materials, resources);
        result.a = 1.0;
    }
    outputImage.write(result, pixel);
}
