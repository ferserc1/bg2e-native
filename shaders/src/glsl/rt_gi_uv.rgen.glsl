#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "lib/deferred_utils.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba16f) uniform image2D giOutput;
layout(set = 0, binding = 2) uniform sampler2D uvWorldPosition;
layout(set = 0, binding = 3) uniform sampler2D uvWorldNormal;
layout(set = 0, binding = 4) uniform samplerCube irradianceMap;
layout(set = 0, binding = 5) uniform usampler2D uvValidMask;

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
    uint shadowSamples;
    uint useBlueNoise;
    uint useShadows;
} pc;

layout(location = 0) rayPayloadEXT GIPayload {
    vec3 hitDirectLight;
    vec3 hitAlbedo;
    vec3 hitNormal;
    vec3 hitPosition;
    uint didHit;
} payload;

vec3 uvRandomHemisphereDirection(vec3 normal, inout uint seed)
{
    float u1 = rand(seed);
    float u2 = rand(seed);
    float radius = sqrt(u1);
    float phi = 6.28318530718 * u2;
    vec3 localDirection = vec3(
        radius * cos(phi),
        radius * sin(phi),
        sqrt(max(0.0, 1.0 - u1))
    );
    vec3 reference = abs(normal.z) < 0.999
        ? vec3(0.0, 0.0, 1.0)
        : vec3(0.0, 1.0, 0.0);
    vec3 tangent = normalize(cross(reference, normal));
    vec3 bitangent = cross(normal, tangent);
    return normalize(mat3(tangent, bitangent, normal) * localDirection);
}

vec3 normalizeAgainstEnvironment(vec3 radiance, vec3 referenceIrradiance)
{
    return vec3(
        referenceIrradiance.r > 1.0e-5 ? max(radiance.r / referenceIrradiance.r, 0.0) : 0.0,
        referenceIrradiance.g > 1.0e-5 ? max(radiance.g / referenceIrradiance.g, 0.0) : 0.0,
        referenceIrradiance.b > 1.0e-5 ? max(radiance.b / referenceIrradiance.b, 0.0) : 0.0
    );
}

void main()
{
    ivec2 pixel = ivec2(gl_LaunchIDEXT.xy);
    ivec2 extent = imageSize(giOutput);
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }
    if (texelFetch(uvValidMask, pixel, 0).r == 0u)
    {
        imageStore(giOutput, pixel, vec4(0.0));
        return;
    }

    vec3 worldPosition = texelFetch(uvWorldPosition, pixel, 0).xyz;
    vec3 worldNormal = texelFetch(uvWorldNormal, pixel, 0).xyz;
    float normalLengthSquared = dot(worldNormal, worldNormal);
    if (normalLengthSquared <= 1.0e-12)
    {
        imageStore(giOutput, pixel, vec4(0.0));
        return;
    }
    worldNormal *= inversesqrt(normalLengthSquared);

    vec3 totalRadiance = vec3(0.0);
    for (uint sampleIndex = 0u; sampleIndex < pc.sampleCount; ++sampleIndex)
    {
        vec3 throughput = vec3(1.0);
        vec3 sampleRadiance = vec3(0.0);
        vec3 origin = worldPosition + worldNormal * pc.rayBias;
        vec3 currentNormal = worldNormal;

        for (uint bounce = 0u; bounce < pc.bounceCount; ++bounce)
        {
            uint seed = uint(pixel.x) * 1973u ^
                        uint(pixel.y) * 9277u ^
                        (pc.frameIndex + 1u) * 26699u ^
                        sampleIndex * 104729u ^
                        bounce * 48611u;
            vec3 rayDirection = uvRandomHemisphereDirection(currentNormal, seed);

            payload.didHit = 0u;
            payload.hitDirectLight = vec3(0.0);
            payload.hitAlbedo = vec3(1.0);
            payload.hitNormal = vec3(0.0, 1.0, 0.0);
            payload.hitPosition = vec3(0.0);
            traceRayEXT(tlas, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0,
                origin, 0.0001, rayDirection, pc.maxDistance, 0);

            if (payload.didHit == 0u)
            {
                sampleRadiance += throughput * texture(irradianceMap, rayDirection).rgb;
                break;
            }

            sampleRadiance += throughput * payload.hitDirectLight;
            throughput *= payload.hitAlbedo;
            origin = payload.hitPosition + payload.hitNormal * pc.rayBias;
            currentNormal = payload.hitNormal;
            if (bounce + 1u == pc.bounceCount)
            {
                sampleRadiance += throughput * texture(irradianceMap, currentNormal).rgb;
            }
        }
        totalRadiance += sampleRadiance;
    }

    vec3 averageRadiance = totalRadiance / float(pc.sampleCount);
    vec3 referenceIrradiance = texture(irradianceMap, worldNormal).rgb;
    vec3 normalizedGI = normalizeAgainstEnvironment(averageRadiance, referenceIrradiance);
    imageStore(giOutput, pixel, vec4(normalizedGI, 1.0));
}
