# M03-01 — Deferred decision checkpoint for advanced RT backends

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M01/M02 reviews; do not assume prior RT choices.

## Scope and deliverable

- `doc/api/gpu/RayTracingPipeline.md`
- `doc/tutorials/gpu/13_ray_tracing_pipeline.md`
- `lib/include/bg2e/gpu/RayTracingPipeline.hpp`
- `lib/include/bg2e/gpu/ResourceSet.hpp`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/rt-design.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/api/gpu/RayTracingPipeline.md](../../../../doc/api/gpu/RayTracingPipeline.md), [doc/tutorials/gpu/13_ray_tracing_pipeline.md](../../../../doc/tutorials/gpu/13_ray_tracing_pipeline.md), [lib/include/bg2e/gpu/RayTracingPipeline.hpp](../../../../lib/include/bg2e/gpu/RayTracingPipeline.hpp), [lib/include/bg2e/gpu/ResourceSet.hpp](../../../../lib/include/bg2e/gpu/ResourceSet.hpp).

## Implementation work

1. Re-read current GPU RT documentation/source and inspect actual M01 query material/geometry access. Record implemented capabilities rather than rely on aggregate rayTracingSupported.
2. Compare explicit Vulkan raygen/miss/closest-hit/any-hit tables with equivalent Metal compute/intersection kernels, candidate rejection and resource addressing. Keep shader authoring/bindings native and explicit.
3. Prepare a concrete design for effect contracts, material tables, cutout support, resource arrays/packed data and shader libraries, including complexity/limits and reuse by bake.
4. Resolve deliberately deferred architectural choices with the user if needed after the concrete comparison. Record the selected path and update M03-02–M03-10 and M04/M08 before implementing advanced pipelines.

## Verification and exit conditions

- The design accounts for arbitrary engine geometry/material lookup, non-opaque candidates and independent Vulkan/Metal execution without inventing a native Metal RT pipeline.
- Any unresolved architectural choice is explicit and dependent implementation waits for its resolution; no speculative table framework is built first.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

This checkpoint can revise later steps substantially. Keep the product goals fixed while adapting real primitive/table/shader choices.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
