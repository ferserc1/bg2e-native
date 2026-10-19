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

#ifndef BG2E_DRAW_TRACING_GLSL
#define BG2E_DRAW_TRACING_GLSL
#include "bindings.glsl"
struct TraceVertex { vec4 position; vec4 normal; vec4 uv; vec4 tangent; };
struct TraceInstance { mat4 model; mat4 normal; uvec4 ranges; uvec4 identity; };
struct TraceMaterial { DrawMaterialData values; uvec4 images[2]; uvec4 samplers[2]; };
layout(set=1,binding=0,std430) readonly buffer TraceVertices
{
    TraceVertex traceVertices[];
};
layout(set=1,binding=1,std430) readonly buffer TraceIndices
{
    uint traceIndices[];
};
layout(set=1,binding=2,std430) readonly buffer TraceInstances
{
    TraceInstance traceInstances[];
};
layout(set=1,binding=3,std430) readonly buffer TraceMaterials
{
    TraceMaterial traceMaterials[];
};
layout(set=1,binding=4) uniform accelerationStructureEXT traceScene;
layout(set=1,binding=5) uniform texture2D traceImages[TRACE_CAPACITY];
layout(set=1,binding=6) uniform sampler traceSamplers[TRACE_CAPACITY];
vec4 traceUV(uint instance, uint primitive, vec2 bary)
{
    TraceInstance record = traceInstances[instance];
    uint first = record.ranges.x + primitive*3u;
    TraceVertex a = traceVertices[record.ranges.y+traceIndices[first]];
    TraceVertex b = traceVertices[record.ranges.y+traceIndices[first+1u]];
    TraceVertex c = traceVertices[record.ranges.y+traceIndices[first+2u]];
    return a.uv*(1.0-bary.x-bary.y)+b.uv*bary.x+c.uv*bary.y;
}
vec4 traceSample(uint materialIndex, uint slot, vec4 uv)
{
    TraceMaterial material = traceMaterials[materialIndex];
    uint imageIndex = material.images[slot/4u][slot%4u];
    uint samplerIndex = material.samplers[slot/4u][slot%4u];
    if (imageIndex == 0xffffffffu || samplerIndex == 0xffffffffu)
    {
        // No descriptor access or sRGB conversion for an absent texture.
        return slot == 3u ? vec4(0.5,0.5,1.0,1.0) : vec4(1.0);
    }
    vec2 selected = drawSelectUV(material.values,slot,uv.xy,uv.zw);
    return drawDecodeSample(material.values,slot,
        textureLod(sampler2D(traceImages[nonuniformEXT(imageIndex)],traceSamplers[nonuniformEXT(samplerIndex)]),selected,0.0));
}
bool traceAccept(uint instance, uint primitive, vec2 bary, uint instanceCount)
{
    if (instance >= instanceCount || primitive >= traceInstances[instance].ranges.z)
    {
        return false;
    }
    uint material = traceInstances[instance].ranges.w;
    if ((traceMaterials[material].values.uvSets[1].z & 2u) == 0u)
    {
        return true;
    }
    float alpha = traceSample(material,0u,traceUV(instance,primitive,bary)).a * traceMaterials[material].values.albedo.a;
    return alpha >= traceMaterials[material].values.emissionAlpha.y;
}
vec3 traceGeometricNormal(uint instance, uint primitive, vec3 direction)
{
    TraceInstance record = traceInstances[instance];
    uint first = record.ranges.x+primitive*3u;
    vec3 a = traceVertices[record.ranges.y+traceIndices[first]].position.xyz;
    vec3 b = traceVertices[record.ranges.y+traceIndices[first+1u]].position.xyz;
    vec3 c = traceVertices[record.ranges.y+traceIndices[first+2u]].position.xyz;
    vec3 n = mat3(record.normal)*cross(b-a,c-a);
    n = dot(n,n) > 1e-16 ? normalize(n) : -direction;
    return dot(n,direction) > 0.0 ? -n : n;
}
#if defined(DRAW_TRACE_QUERIES)
struct TraceHit { bool hit; float distance; vec3 normal; };
TraceHit traceNearest(vec3 origin, vec3 direction, float minimum, float maximum, uint instanceCount)
{
    rayQueryEXT query;
    rayQueryInitializeEXT(query,traceScene,0u,255u,origin,minimum,direction,maximum);
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query,false) == gl_RayQueryCandidateIntersectionTriangleEXT &&
            traceAccept(rayQueryGetIntersectionInstanceIdEXT(query,false),rayQueryGetIntersectionPrimitiveIndexEXT(query,false),
                rayQueryGetIntersectionBarycentricsEXT(query,false),instanceCount))
        {
            rayQueryConfirmIntersectionEXT(query);
        }
    }
    TraceHit result;
    result.hit = rayQueryGetIntersectionTypeEXT(query,true) == gl_RayQueryCommittedIntersectionTriangleEXT;
    result.distance = maximum;
    result.normal = -direction;
    if (result.hit)
    {
        result.distance = rayQueryGetIntersectionTEXT(query,true);
        result.normal = traceGeometricNormal(rayQueryGetIntersectionInstanceIdEXT(query,true),
            rayQueryGetIntersectionPrimitiveIndexEXT(query,true),direction);
    }
    return result;
}
#endif // DRAW_TRACE_QUERIES
#endif
