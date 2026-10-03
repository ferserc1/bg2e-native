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

#ifndef RT_MATERIAL_DATA_GLSL
#define RT_MATERIAL_DATA_GLSL

#include "uniforms.glsl"

#define MAX_RT_OBJECTS 256

#ifndef RT_MATERIAL_SET
#define RT_MATERIAL_SET 1
#endif

// Must match C++ geo::Vertex / VertexPNUUT struct layout (52 bytes)
struct RTVertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord0;
    vec2 texCoord1;
    vec3 tangent;
};

// Must match C++ RTMaterialData struct layout (56 bytes)
struct RTMaterialData {
    vec4 albedo;          // base::Color (rgba)
    vec2 albedoScale;
    uint indexOffset;     // firstIndex of the submesh in the shared index buffer
    float lightEmission;
    uint lightEmissionChannel;
    uint lightEmissionInvert;
    vec2 lightEmissionScale;
    uint lightEmissionUVSet;
    float alphaCutoff;
};

layout(scalar, set = RT_MATERIAL_SET, binding = 0) readonly buffer MaterialDataBuffer {
    RTMaterialData materials[];
};

layout(scalar, set = RT_MATERIAL_SET, binding = 1) readonly buffer VertexBuffer {
    RTVertex vertices[];
} vb[MAX_RT_OBJECTS];

layout(scalar, set = RT_MATERIAL_SET, binding = 2) readonly buffer IndexBuffer {
    uint indices[];
} ib[MAX_RT_OBJECTS];

layout(set = RT_MATERIAL_SET, binding = 3) uniform sampler2D albedoTex[MAX_RT_OBJECTS];
layout(set = RT_MATERIAL_SET, binding = 4) uniform sampler2D lightEmissionTex[MAX_RT_OBJECTS];

float sampleRTLightEmission(sampler2D tex, vec2 uv, RTMaterialData mat)
{
    float value = texture(tex, uv * mat.lightEmissionScale)[mat.lightEmissionChannel];
    if (mat.lightEmissionInvert != 0)
    {
        value = 1.0 - value;
    }
    return value * mat.lightEmission;
}

vec2 rtHitAlbedoUV(uint matIdx, uint primitiveId, vec3 bary, out uint nmatIdx)
{
    nmatIdx = nonuniformEXT(matIdx);
    RTMaterialData mat = materials[matIdx];
    uint base = mat.indexOffset + primitiveId * 3u;
    uint idx0 = ib[nmatIdx].indices[base];
    uint idx1 = ib[nmatIdx].indices[base + 1u];
    uint idx2 = ib[nmatIdx].indices[base + 2u];
    RTVertex vert0 = vb[nmatIdx].vertices[idx0];
    RTVertex vert1 = vb[nmatIdx].vertices[idx1];
    RTVertex vert2 = vb[nmatIdx].vertices[idx2];
    vec2 uv = vert0.texCoord0 * bary.x + vert1.texCoord0 * bary.y + vert2.texCoord0 * bary.z;
    return uv * mat.albedoScale;
}

bool rtAlphaTestHit(uint matIdx, uint primitiveId, vec3 bary)
{
    RTMaterialData mat = materials[matIdx];
    if (mat.alphaCutoff <= 0.0) return true;
    uint nmatIdx;
    vec2 uv = rtHitAlbedoUV(matIdx, primitiveId, bary, nmatIdx);
    return texture(albedoTex[nmatIdx], uv).a * mat.albedo.a >= mat.alphaCutoff;
}

bool rtAlphaTestCandidate(rayQueryEXT rq)
{
    uint matIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false);
    uint primId = rayQueryGetIntersectionPrimitiveIndexEXT(rq, false);
    vec2 b = rayQueryGetIntersectionBarycentricsEXT(rq, false);
    return rtAlphaTestHit(matIdx, primId, vec3(1.0 - b.x - b.y, b.x, b.y));
}

bool _shadowRayTestCutout(accelerationStructureEXT tlas, vec3 origin, vec3 dir, float tMax)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, 0.001, dir, tMax);
    while (rayQueryProceedEXT(rq))
    {
        if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT &&
            rtAlphaTestCandidate(rq))
        {
            rayQueryConfirmIntersectionEXT(rq);
        }
    }
    return rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT;
}

float queryShadowCutout(accelerationStructureEXT tlas, vec3 worldPos, vec3 normal, LightData light, int maxSamples)
{
    vec3 toLight;
    float tMax;
    if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        toLight = -normalize(light.direction);
        tMax = 1000000.0;
    } else {
        toLight = light.position - worldPos;
        tMax = length(toLight);
        toLight = normalize(toLight);
    }
    vec3 origin = worldPos + normal * 0.01;
    int samples = light.shadowSamples > maxSamples ? maxSamples : light.shadowSamples;
    if (samples <= 1) return _shadowRayTestCutout(tlas, origin, toLight, tMax) ? 1.0 : 0.0;
    float angle = light.type == LIGHT_TYPE_DIRECTIONAL
        ? light.sourceSize * 3.14159265 / 180.0 : atan(light.sourceSize, tMax);
    int occluded = 0;
    for (int i = 0; i < samples; ++i)
    {
        vec2 seed = worldPos.xz * 1000.0 + vec2(float(i) * 13.7, float(i) * 7.3);
        if (!_shadowRayTestCutout(tlas, origin, _shadowRandomDirection(toLight, seed, angle), tMax)) ++occluded;
    }
    return 1.0 - float(occluded) / float(samples);
}

float hardShadowCutout(accelerationStructureEXT tlas, vec3 worldPos, vec3 normal, LightData light)
{
    vec3 toLight;
    float tMax;
    if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        toLight = -normalize(light.direction);
        tMax = 1000000.0;
    } else {
        toLight = light.position - worldPos;
        tMax = length(toLight);
        toLight = normalize(toLight);
    }
    return _shadowRayTestCutout(tlas, worldPos + normal * 0.01, toLight, tMax) ? 1.0 : 0.0;
}

bool queryAOCutout(accelerationStructureEXT tlas, vec3 worldPos, vec3 normal, vec3 rayDir,
    float radius, float bias, out float hitDistance)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas, gl_RayFlagsTerminateOnFirstHitEXT,
        0xFF, worldPos + normal * bias, 0.001, rayDir, radius);
    while (rayQueryProceedEXT(rq))
    {
        if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT &&
            rtAlphaTestCandidate(rq))
        {
            rayQueryConfirmIntersectionEXT(rq);
        }
    }
    bool hit = rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT;
    hitDistance = hit ? rayQueryGetIntersectionTEXT(rq, true) : radius;
    return hit;
}

#endif
