#version 460
#extension GL_EXT_ray_tracing : enable

layout(set = 0, binding = 0, rgba8)   uniform image2D outputImage;
layout(set = 0, binding = 2, rgba32f) uniform image2D accumImage;
layout(set = 0, binding = 1) uniform CameraData {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 misc; // x=frameCount, y=width, z=height
} cam;
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;

// Surface information returned by the closest-hit / miss shaders.
struct HitInfo {
    vec3  hitPos;
    vec3  normal;
    vec3  albedo;
    vec3  emission;
    float roughness;
    int   hit; // 1 = surface hit, 0 = ray escaped the scene
};
layout(location = 0) rayPayloadEXT HitInfo payload;

const int   MAX_BOUNCES = 8;
const float PI = 3.14159265359;

// PCG hash based RNG.
uint pcg(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
float rnd(inout uint state) { return float(pcg(state)) / 4294967295.0; }

// Cosine-weighted hemisphere sample around the normal n.
vec3 cosineHemisphere(vec3 n, inout uint state) {
    float r1 = rnd(state);
    float r2 = rnd(state);
    float phi = 2.0 * PI * r1;
    float r = sqrt(r2);
    vec3 up = abs(n.x) > 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * (cos(phi) * r) +
                     bitangent * (sin(phi) * r) +
                     n * sqrt(1.0 - r2));
}

void main()
{
    uint seed = (gl_LaunchIDEXT.x * 1973u + gl_LaunchIDEXT.y * 9277u +
                 uint(cam.misc.x) * 26699u) | 1u;

    // Sub-pixel jitter for anti-aliasing; each frame draws a fresh sample.
    vec2 jitter = vec2(rnd(seed), rnd(seed));
    vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + jitter;
    vec2 inUV = pixelCenter / vec2(cam.misc.y, cam.misc.z);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = cam.viewInverse * vec4(0, 0, 0, 1);
    vec4 target = cam.projInverse * vec4(d.x, -d.y, 1, 1);
    vec4 direction = cam.viewInverse * vec4(normalize(target.xyz), 0);

    vec3 rayO = origin.xyz;
    vec3 rayD = direction.xyz;

    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int b = 0; b < MAX_BOUNCES; ++b)
    {
        traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xff,
            0, 0, 0,
            rayO, 0.001, rayD, 10000.0, 0);

        if (payload.hit == 0)
        {
            // Ray escaped the box: closed Cornell box -> black background.
            break;
        }

        // Emitted light from the surface reaches the camera scaled by the
        // throughput accumulated along the path so far.
        radiance += throughput * payload.emission;

        vec3 n = payload.normal;

        // Glossy reflection model: roughness interpolates between a perfect
        // mirror (0) and a diffuse cosine-weighted bounce (1).
        vec3 refl = reflect(rayD, n);
        vec3 diff = cosineHemisphere(n, seed);
        vec3 newDir = normalize(mix(refl, diff, payload.roughness));
        if (dot(newDir, n) < 0.0)
        {
            newDir = diff;
        }

        throughput *= payload.albedo;
        rayO = payload.hitPos + n * 0.001;
        rayD = newDir;

        // Stop tracing paths that can no longer contribute meaningfully.
        float maxThroughput = max(throughput.r, max(throughput.g, throughput.b));
        if (maxThroughput < 0.001)
        {
            break;
        }
    }

    // Monte Carlo accumulation: running mean of the per-frame estimates.
    float frameCount = cam.misc.x;
    vec3 prev = imageLoad(accumImage, ivec2(gl_LaunchIDEXT.xy)).rgb;
    vec3 accum = (frameCount <= 1.0) ? radiance
                                     : mix(prev, radiance, 1.0 / frameCount);
    imageStore(accumImage, ivec2(gl_LaunchIDEXT.xy), vec4(accum, 1.0));

    // Reinhard tone mapping + gamma correction for the 8-bit output image.
    // Store as .bgr because the image is byte-copied into the BGRA8 swapchain.
    vec3 mapped = accum / (accum + vec3(1.0));
    mapped = pow(mapped, vec3(1.0 / 2.2));
    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(mapped.bgr, 1.0));
}
