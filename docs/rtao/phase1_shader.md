# Phase 1: Compute Shader — `rt_ao.comp.glsl`

## File

`shaders/src/rt_ao.comp.glsl` (create)

## Purpose

Compute shader that reads G-buffers (Normal + Depth), reconstructs world position, and traces rays against the TLAS to compute ambient occlusion. Writes result to a `r8` storage image.

## Extensions Required

- `GL_ARB_shading_language_include` — for `#include` support
- `GL_EXT_ray_query` — for `rayQueryEXT` types and functions

## Descriptor Set Layout (set=0)

| Binding | Type | Name | Description |
|---------|------|------|-------------|
| 0 | `sampler2D` | `g_Normal` | World-space normals (R16G16B16A16_SFLOAT) |
| 1 | `sampler2D` | `g_Depth` | Depth buffer (D32_SFLOAT) |
| 2 | `accelerationStructureEXT` | `tlas` | Top-level acceleration structure |
| 3 | `image2D` (r8, writeonly) | `aoOutput` | Output AO image |

## Push Constants

```glsl
layout(push_constant) uniform PushConstant {
    mat4 inverseViewProjection;  // For world position reconstruction
    int sampleCount;             // Number of AO rays per pixel
    vec3 padding;                // Alignment to 80 bytes
} pc;
```

## Workgroup Size

```glsl
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
```

Dispatch: `ceil(width/8) x ceil(height/8) x 1`

## Algorithm

1. Compute pixel coordinate from `gl_GlobalInvocationID.xy`
2. Bounds check against `imageSize(aoOutput)`
3. Compute UV: `(pixelCoord + 0.5) / imageSize`
4. Sample depth. If `depth >= 1.0` (sky) → write `1.0` and return
5. Reconstruct world position using `reconstructWorldPosition(uv, depth, inverseViewProjection)`
6. Sample and decode normal: `texture(g_Normal, uv).xyz * 2.0 - 1.0`
7. For each sample (up to `sampleCount`):
   a. Generate pseudo-random direction in cosine-weighted hemisphere using hash function
   b. Build TBN basis from normal
   c. Transform random direction to world space
   d. Initialize ray query: origin = `worldPos + normal * 0.01`, direction = `rayDir`, tMax = `1.0`
   e. If no hit → increment AO counter
8. Normalize: `ao /= sampleCount`
9. Write to output image

## Key Implementation Details

- **Self-intersection prevention**: offset ray origin by `normal * 0.01` (same pattern as `lib/ray_tracing.glsl`)
- **Hash function**: `fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)` — standard GPU hash
- **Cosine-weighted hemisphere sampling**: better convergence than uniform sampling
- **Ray flags**: `gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT` — fast termination
- **Output format**: `r8` qualifier matches `VK_FORMAT_R8_UNORM` on the CPU side
- **Sky pixels**: depth >= 1.0 → AO = 1.0 (no occlusion)
- **maxRayDistance**: hardcoded to `1.0` in shader (can be made a push constant later)

## Reference Files

- `shaders/src/lib/ray_tracing.glsl` — existing ray query pattern (shadow rays)
- `shaders/src/lib/deferred_utils.glsl` — `reconstructWorldPosition` function (lines 42-66)
- `shaders/src/deferred_composite_rt.frag.glsl` — existing deferred RT shader with TLAS binding
