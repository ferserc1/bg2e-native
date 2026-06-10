#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "lib/rt_material_data.glsl"

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

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

    uint triIdx = gl_PrimitiveID;

    uint idx0 = ib[nmatIdx].indices[triIdx * 3 + 0];
    uint idx1 = ib[nmatIdx].indices[triIdx * 3 + 1];
    uint idx2 = ib[nmatIdx].indices[triIdx * 3 + 2];

    RTVertex vert0 = vb[nmatIdx].vertices[idx0];
    RTVertex vert1 = vb[nmatIdx].vertices[idx1];
    RTVertex vert2 = vb[nmatIdx].vertices[idx2];

    vec2 uv = vert0.texCoord0 * bary.x
            + vert1.texCoord0 * bary.y
            + vert2.texCoord0 * bary.z;

    RTMaterialData mat = materials[matIdx];
    vec2 scaledUV = uv * mat.albedoScale;

    vec3 texColor = texture(albedoTex[nmatIdx], scaledUV).rgb;

    payload.hitColor = mat.albedo.rgb * texColor;
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}