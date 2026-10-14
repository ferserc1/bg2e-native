# M01-11 — Alpha-aware query soft shadows without RT pipelines

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-02/M01-09/M01-10.

## Scope and deliverable

- `lib/include/bg2e/gpu/RayTracingMesh.hpp`
- `lib/include/bg2e/gpu/RayTracingScene.hpp`
- `lib/src/bg2e/gpu/vk/RayTracingMesh.cpp`
- `examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal`
- `shaders/src/glsl/lib/rt_material_data.glsl`
- `lib/include/bg2e/draw/pipeline/SoftShadowPass.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/RayTracingMesh.hpp](../../../../lib/include/bg2e/gpu/RayTracingMesh.hpp), [lib/include/bg2e/gpu/RayTracingScene.hpp](../../../../lib/include/bg2e/gpu/RayTracingScene.hpp), [lib/src/bg2e/gpu/vk/RayTracingMesh.cpp](../../../../lib/src/bg2e/gpu/vk/RayTracingMesh.cpp), [examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal](../../../../examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal), [shaders/src/glsl/lib/rt_material_data.glsl](../../../../shaders/src/glsl/lib/rt_material_data.glsl).

## Implementation work

1. Add primitive non-opaque geometry/instance flags and required query resource access on both backends. Build BLAS/TLAS through GPU commands; keep the high-level collector/material matching in draw.
2. Choose the smallest explicit geometry/UV/albedo-alpha table mechanism sufficient for cutout-aware queries, extending generic binding arrays/ranges if needed. Do not settle advanced RT pipeline architecture here.
3. Implement backend-specific equivalent soft-shadow kernels using per-light castShadows/sourceSize/shadowSamples and accepted triangle candidates. Transparent-hit filtering follows current behavior unless explicitly revised.
4. Gate query shadows using actual GPU query/acceleration/format capabilities; preserve unshadowed PBR fallback. No any-hit tracing pipeline, AO/GI/reflection or denoise allocation in this step.

## Verification and exit conditions

- A non-opaque cutout casts shadows through solid texels and not holes; source-size/sample changes alter softness on supported Vulkan/Metal paths.
- No-RT or forced-query-disabled rendering remains functional without BLAS/TLAS allocation requirements; no frame-by-frame global idle is introduced.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03-01 must review the actual query table/cutout contract before deciding advanced tracing addressing and pipeline extensions.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
