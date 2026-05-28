# Step 3: RT Reflection Shaders — Revised

## Objective

Create the three ray tracing shaders for reflections: ray generation, miss, and closest-hit.

This revised version changes the ray payload validity field from `bool didHit` to `uint didHit`. This is safer for payload layout across raygen, miss and closest-hit shaders.

## Files to Create

### `shaders/src/rt_reflections.rgen.glsl`

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

#include "lib/deferred_utils.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba16f) uniform image2D reflectionOutput;
layout(set = 0, binding = 2) uniform sampler2D g_Depth;
layout(set = 0, binding = 3) uniform sampler2D g_Normal;
layout(set = 0, binding = 4) uniform sampler2D g_Material;
layout(set = 0, binding = 5) uniform sampler2D g_Albedo;
layout(set = 0, binding = 6) uniform sampler2D g_FresnelFlags;

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

uint hash(uint x) {
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    return x;
}

vec2 rand2(uint seed) {
    return vec2(
        float(hash(seed)) / float(0xffffffffu),
        float(hash(seed + 1u)) / float(0xffffffffu)
    );
}

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
    vec3 normal = normalize(texture(g_Normal, uv).rgb);
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
```

### `shaders/src/rt_reflections.rmiss.glsl`

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.hitColor = vec3(0.0);
    payload.hitDistance = 0.0;
    payload.didHit = 0u;
}
```

### `shaders/src/rt_reflections.rchit.glsl`

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.didHit = 1u;
    payload.hitDistance = gl_HitTEXT;

    // First implementation: visible debug reflection only.
    // This validates RT pipeline, SBT, TLAS traversal and image output.
    // Real material lookup can be added in a later iteration.
    float attenuation = 1.0 - clamp(gl_HitTEXT / 50.0, 0.0, 1.0);
    payload.hitColor = vec3(0.5) * attenuation;
}
```

## Design Notes

### Purpose of This First Shader Set

This shader set is intentionally infrastructure-first.

It is expected to produce visible but not physically correct reflections. The closest-hit shader currently returns a simple gray value attenuated by hit distance. That is acceptable for this step because the goal is to validate:

- ray tracing pipeline creation
- SBT creation and binding
- TLAS traversal
- G-buffer reconstruction
- multi-ray dispatch
- RGBA reflection image output
- alpha mask generation
- later temporal accumulation and composite integration

### Alpha Convention

The output image is `VK_FORMAT_R16G16B16A16_SFLOAT`:

```text
RGB = traced reflection radiance
A   = validity mask
```

```text
A = 1.0 -> at least one valid reflection hit contributed
A = 0.0 -> no RT reflection; composite should fall back to cubemap
```

### Roughness Threshold

Pixels with:

```glsl
roughness > pc.maxRoughness
```

must write `vec4(0.0)` and skip ray tracing.

Cubemap fallback is not done in the RT shader. It is done in the composite shader.

### Payload Layout

Use `uint didHit`, not `bool didHit`, in all three shaders.

All payload declarations must match exactly:

```glsl
layout(location = 0) rayPayloadEXT / rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;
```

## Verification

After this step:

- The three `.glsl` files exist in `shaders/src/`.
- They compile to SPIR-V with Vulkan 1.2 target.
- No C++ code uses them yet.
- The engine runs identically to before.
