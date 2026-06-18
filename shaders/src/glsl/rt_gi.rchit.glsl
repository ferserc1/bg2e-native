#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "lib/rt_material_data.glsl"
#include "lib/basic_lighting.glsl"
#include "lib/ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT GIPayload {
    vec3 hitDirectLight;
    vec3 hitAlbedo;
    vec3 hitNormal;
    vec3 hitPosition;
    uint didHit;
} payload;

layout(push_constant) uniform PushConstant {
    mat4 inverseViewProjection;
    vec3 cameraPosition;
    float rayBias;
    vec2 outputSize;
    uint sampleCount;
    uint bounceCount;
    uint frameIndex;
    float maxDistance;
    uint giLightCount;
    uint padding0;
} pc;

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 4) uniform samplerCube irradianceMap;

layout(std430, set = 2, binding = 0) readonly buffer GILightBuffer {
    LightData giLights[];
};

layout(set = 1, binding = 0) readonly buffer MaterialDataBuffer {
    RTMaterialData materials[];
};

layout(scalar, set = 1, binding = 1) readonly buffer VertexBuffer {
    RTVertex vertices[];
} vb[MAX_RT_OBJECTS];

layout(scalar, set = 1, binding = 2) readonly buffer IndexBuffer {
    uint indices[];
} ib[MAX_RT_OBJECTS];

layout(set = 1, binding = 3) uniform sampler2D albedoTex[MAX_RT_OBJECTS];

hitAttributeEXT vec2 attribs;

void main() {
    uint matIdx = gl_InstanceCustomIndexEXT;
    uint nmatIdx = nonuniformEXT(matIdx);

    vec3 bary = vec3(
        1.0 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );

    RTMaterialData mat = materials[matIdx];

    // Add submesh firstIndex so we read the correct triangle from the shared index buffer.
    uint base = mat.indexOffset + gl_PrimitiveID * 3u;

    uint idx0 = ib[nmatIdx].indices[base + 0u];
    uint idx1 = ib[nmatIdx].indices[base + 1u];
    uint idx2 = ib[nmatIdx].indices[base + 2u];

    RTVertex vert0 = vb[nmatIdx].vertices[idx0];
    RTVertex vert1 = vb[nmatIdx].vertices[idx1];
    RTVertex vert2 = vb[nmatIdx].vertices[idx2];

    vec2 uv = vert0.texCoord0 * bary.x
            + vert1.texCoord0 * bary.y
            + vert2.texCoord0 * bary.z;

    // Use geometry normal only — normal maps are not considered for GI bounces.
    vec3 objectNormal = normalize(
        vert0.normal * bary.x +
        vert1.normal * bary.y +
        vert2.normal * bary.z
    );
    vec3 worldNormal = normalize(mat3(gl_ObjectToWorldEXT) * objectNormal);

    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    vec2 scaledUV = uv * mat.albedoScale;
    vec3 texColor = texture(albedoTex[nmatIdx], scaledUV).rgb;
    vec3 surfaceAlbedo = mat.albedo.rgb * texColor;

    // Indirect ambient at the hit point from the irradiance map.
    vec3 irradiance = texture(irradianceMap, worldNormal).rgb;
    vec3 directLight = surfaceAlbedo * irradiance;

    // Direct contribution from lights configured to affect ray tracing.
    for (uint i = 0u; i < pc.giLightCount; ++i) {
        LightData light = giLights[i];
        float shadowFactor = 1.0;
        if (light.castShadows != 0) {
            shadowFactor = queryShadow(tlas, worldPos, worldNormal, light, 8);
        }
        directLight += shadowFactor * computeBasicLighting(light, worldPos, worldNormal, surfaceAlbedo);
    }

    payload.hitDirectLight = directLight;
    payload.hitAlbedo = surfaceAlbedo;
    payload.hitNormal = worldNormal;
    payload.hitPosition = worldPos;
    payload.didHit = 1u;
}
