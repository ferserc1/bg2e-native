# M03-01 — Deferred decision checkpoint for advanced RT backends

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M01/M02 reviews; do not assume prior RT choices.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

This checkpoint can revise later steps substantially. Keep the product goals fixed while adapting real primitive/table/shader choices.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-11 actual query checkpoint input

Inspect [query-shadows.md](../milestone_01_deferred_renderer/query-shadows.md), SoftShadowPass and paired draw/query/soft_shadows kernels. M01 uses compute queries on both backends, generic BLAS opaque + per-instance Geometry/ForceOpaque/ForceNonOpaque, a dense TLAS-order table, CPU-derived per-cutout triangle UV0/UV1, and eight statically bound albedo image/sampler pairs with an explicit capacity error. It does not provide arbitrary full-material lookup, GPU vertex/index address tables or tracing-pipeline hit shaders. Reevaluate addressing/limits/reuse for advanced RT and bake here rather than accidentally extending this minimum table as a settled architecture. Keep canonical object IDs separate from filtered TLAS instance order and preserve per-slot lifetime/noncoherent buffer flushing.

## M01-16 reviewed baseline

Decision owner remains the project lead after concrete backend/table options are prepared. M01 provides only dense UV/alpha query lookup with eight distinct image/sampler pairs and explicit overflow; neither that limit nor its layout is an approved arbitrary hit-material architecture. Record chosen resource addressing, candidate filtering, capacities, transparency and native stage arrangement before M03-02/M04 depend on it. See [M01 review](../milestone_01_deferred_renderer/review.md).
