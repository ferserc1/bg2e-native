# M03-02 — GPU tracing stages and shader data access selected by checkpoint

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M03-01 accepted design; M01 query primitives.

## Scope and deliverable

- `lib/include/bg2e/gpu/Common.hpp`
- `lib/include/bg2e/gpu/RayTracingPipeline.hpp`
- `lib/include/bg2e/gpu/ResourceSet.hpp`
- `lib/include/bg2e/gpu/ShaderLib.hpp`
- `lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp`
- `lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Common.hpp](../../../../lib/include/bg2e/gpu/Common.hpp), [lib/include/bg2e/gpu/RayTracingPipeline.hpp](../../../../lib/include/bg2e/gpu/RayTracingPipeline.hpp), [lib/include/bg2e/gpu/ResourceSet.hpp](../../../../lib/include/bg2e/gpu/ResourceSet.hpp), [lib/include/bg2e/gpu/ShaderLib.hpp](../../../../lib/include/bg2e/gpu/ShaderLib.hpp), [lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp](../../../../lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp), [lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp](../../../../lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp).

## Implementation work

1. Implement only the required generic extensions from the checkpoint: Vulkan any-hit/groups/stage visibility, resource arrays or buffer ranges/addressing and native handlers where needed.
2. Preserve explicit per-backend ShaderBinding indices and add matching ShaderLib stage loading only when actual shaders need it; Metal uses separate equivalent kernels and nullable stages as documented.
3. Validate material/geometry table capacity, primitive device/format/shader-stage requirements and acceleration build/read dependencies.
4. Keep scene traversal, material policy, effect settings and sampling algorithms in draw; GPU extensions remain callable with primitive resources alone.

## Verification and exit conditions

- A small primitive validation dispatch/candidate test demonstrates actual hit data/cutout behavior on supported backends and rejects incompatible resources.
- Existing query-soft-shadow and RT demonstration paths still work; unavailable advanced support is reported independently of the raster baseline.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03-04–M03-06 and bake passes depend on these exact data-access/stage contracts; amend them if implementation differs from the checkpoint.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
