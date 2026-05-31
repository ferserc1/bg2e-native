#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : enable

#include "lib/deferred_utils.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba16f) uniform image2D reflectionOutput;
layout(set = 0, binding = 2) uniform sampler2D g_Depth;
layout(set = 0, binding = 3) uniform sampler2D g_Normal;
layout(set = 0, binding = 4) uniform sampler2D g_Material;

layout(push_constant) uniform PushConstant {
    mat4 inverseViewProjection;
    vec3 cameraPosition;
    float maxRoughness;
    vec2 outputSize;
    uint sampleCount;
    uint frameIndex;
    float rayBias;
    float maxDistance;
    float roughnessSpread;
    uint padding0;
} pc;

layout(location = 0) rayPayloadEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

vec3 sampleGGXLike(vec2 xi, vec3 direction, float roughness) {
    float a = max(roughness * roughness, 0.001);
    float phi = 2.0 * 3.14159265359 * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));

    vec3 localSample = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(direction.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, direction));
    vec3 bitangent = cross(direction, tangent);

    return normalize(tangent * localSample.x + bitangent * localSample.y + direction * localSample.z);
}

void main() {
    ivec2 pixel = ivec2(gl_LaunchIDEXT.xy);

    if (pixel.x >= int(pc.outputSize.x) || pixel.y >= int(pc.outputSize.y)) {
        return;
    }

    vec2 uv = (vec2(pixel) + 0.5) / pc.outputSize;

    float depth = texture(g_Depth, uv).r;
    vec3 normalEncoded = texture(g_Normal, uv).rgb;
    vec3 normal = normalize(normalEncoded * 2.0 - 1.0);
    vec4 material = texture(g_Material, uv);

    float roughness = material.g;

    if (depth >= 1.0) {
        imageStore(reflectionOutput, pixel, vec4(0.0));
        return;
    }

    if (roughness > pc.maxRoughness) {
        imageStore(reflectionOutput, pixel, vec4(0.0));
        return;
    }

    if (length(normal) < 0.5) {
        imageStore(reflectionOutput, pixel, vec4(0.0));
        return;
    }

    vec3 worldPos = reconstructWorldPosition(uv, depth, pc.inverseViewProjection);
    vec3 viewDir = normalize(worldPos - pc.cameraPosition);
    vec3 reflectionDir = normalize(reflect(viewDir, normal));

    vec3 accumulatedColor = vec3(0.0);
    uint validHits = 0u;
    uint samples = max(pc.sampleCount, 1u);

    // Optimization. adjust the sample count in function of the roughness, to
    // minimize the number of samples in mirror-like surfaces
    float r = clamp(roughness / pc.maxRoughness, 0.0, 1.0);
    samples = max(1u, uint(ceil(float(pc.sampleCount) * r * r)));

    for (uint i = 0u; i < samples; ++i) {
        uint seed = uint(pixel.x) * 1973u
                  ^ uint(pixel.y) * 9277u
                  ^ (pc.frameIndex + 1u) * 26699u
                  ^ i * 104729u;

        vec2 xi = rand2(seed);

        vec3 rayDir = reflectionDir;
        if (roughness > 0.01) {
            rayDir = sampleGGXLike(xi, reflectionDir, roughness * pc.roughnessSpread);

            if (dot(rayDir, normal) <= 0.01) {
                rayDir = normalize(reflectionDir + normal * 0.1);
            }
        }

        payload.hitColor = vec3(0.0);
        payload.hitDistance = 0.0;
        payload.didHit = 0u;

        traceRayEXT(
            tlas,
            gl_RayFlagsOpaqueEXT,
            0xff,
            0,
            0,
            0,
            worldPos + normal * pc.rayBias,
            0.001,
            rayDir,
            pc.maxDistance,
            0
        );

        if (payload.didHit != 0u) {
            accumulatedColor += payload.hitColor;
            validHits++;
        }
    }

    if (validHits > 0u) {
        vec3 avgColor = accumulatedColor / float(validHits);
        imageStore(reflectionOutput, pixel, vec4(avgColor, 1.0));
    } else {
        imageStore(reflectionOutput, pixel, vec4(0.0));
    }
}