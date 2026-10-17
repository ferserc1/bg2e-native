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

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#define RT_COUNT 32
#include "../lib/rt_probe.glsl"
layout(local_size_x=16, local_size_y=16) in;
void main()
{
    uint pixel = gl_GlobalInvocationID.x;
    if (pixel >= 32u || gl_GlobalInvocationID.y != 0u)
    {
        return;
    }
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevel, 0u, 255u,
        vec3((float(pixel)+0.5)/32.0*0.8-0.4,-0.25,0), 0.001, vec3(0,0,1), 4.0);
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
        {
            uint instance = rayQueryGetIntersectionInstanceIdEXT(query, false);
            vec4 color = surface(instance, rayQueryGetIntersectionPrimitiveIndexEXT(query, false),
                rayQueryGetIntersectionBarycentricsEXT(query, false), pixel);
            if (color.a >= materials[instances[instance].ranges.w].parameters.x)
            {
                rayQueryConfirmIntersectionEXT(query);
            }
        }
    }
    vec4 result = vec4(1,0,1,1);
    if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        result = surface(rayQueryGetIntersectionInstanceIdEXT(query, true),
            rayQueryGetIntersectionPrimitiveIndexEXT(query, true), rayQueryGetIntersectionBarycentricsEXT(query, true), pixel);
        result.a = 1.0;
    }
    imageStore(outputImage, ivec2(pixel,0), result);
}
