#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : enable

#include "lib/deferred_utils.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba16f) uniform image2D giOutput;
layout(set = 0, binding = 2) uniform sampler2D g_Depth;
layout(set = 0, binding = 3) uniform sampler2D g_Normal;
layout(set = 0, binding = 4) uniform samplerCube irradianceMap;

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

layout(location = 0) rayPayloadEXT GIPayload {
    vec3 hitDirectLight;
    vec3 hitAlbedo;
    vec3 hitNormal;
    vec3 hitPosition;
    uint didHit;
} payload;

void main() {
    ivec2 pixel = ivec2(gl_LaunchIDEXT.xy);

    if (pixel.x >= int(pc.outputSize.x) || pixel.y >= int(pc.outputSize.y)) {
        return;
    }

    vec2 uv = (vec2(pixel) + 0.5) / pc.outputSize;

    float depth = texture(g_Depth, uv).r;
    if (depth >= 1.0) {
        imageStore(giOutput, pixel, vec4(0.0));
        return;
    }

    vec3 normalEncoded = texture(g_Normal, uv).rgb;
    vec3 normal = normalize(normalEncoded * 2.0 - 1.0);
    // The depth >= 1.0 test above already discards background/sky pixels, so any
    // pixel reaching this point has a valid surface. A previous magnitude-based
    // guard (length(normalEncoded) < 0.5) wrongly rejected valid normals pointing
    // into the negative octant (Nx+Ny+Nz < -1.5), producing black GI patches.

    vec3 worldPos = reconstructWorldPosition(uv, depth, pc.inverseViewProjection);

    vec3 totalRadiance = vec3(0.0);

    for (uint s = 0u; s < pc.sampleCount; ++s) {
        vec3 throughput = vec3(1.0);
        vec3 sampleRadiance = vec3(0.0);
        vec3 origin = worldPos + normal * pc.rayBias;
        vec3 currentNormal = normal;

        for (uint b = 0u; b < pc.bounceCount; ++b) {
            uint seed = uint(pixel.x) * 1973u
                      ^ uint(pixel.y) * 9277u
                      ^ (pc.frameIndex + 1u) * 26699u
                      ^ s * 104729u
                      ^ b * 48611u;

            vec3 dir = randomHemisphereDirection(currentNormal, seed);

            payload.didHit = 0u;
            payload.hitDirectLight = vec3(0.0);
            payload.hitAlbedo = vec3(1.0);
            payload.hitNormal = vec3(0.0, 1.0, 0.0);
            payload.hitPosition = vec3(0.0);

            traceRayEXT(
                tlas,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0, 0, 0,
                origin,
                0.001,
                dir,
                pc.maxDistance,
                0
            );

            if (payload.didHit != 0u) {
                // Accumulate direct light from this bounce, attenuated by accumulated throughput.
                // hitDirectLight = surfaceAlbedo * (irradiance + direct_lights), already includes
                // the bounce surface albedo for color bleeding.
                sampleRadiance += throughput * payload.hitDirectLight;

                // Propagate albedo of the hit surface to the next bounce for color bleeding.
                // With cosine-weighted sampling the PI from the Lambert BRDF and the PDF cancel,
                // leaving the albedo as the only throughput multiplier.
                throughput *= payload.hitAlbedo;
                origin = payload.hitPosition + payload.hitNormal * pc.rayBias;
                currentNormal = payload.hitNormal;
            } else {
                // Ray missed all geometry: sample IBL to capture sky/environment contribution.
                vec3 envLight = texture(irradianceMap, dir).rgb;
                sampleRadiance += throughput * envLight;
                break;
            }
        }

        totalRadiance += sampleRadiance;
    }

    vec3 avgRadiance = totalRadiance / float(pc.sampleCount);
    // Alpha=1: mark as valid (used by the temporal accumulator HDR path)
    imageStore(giOutput, pixel, vec4(avgRadiance, 1.0));
}
