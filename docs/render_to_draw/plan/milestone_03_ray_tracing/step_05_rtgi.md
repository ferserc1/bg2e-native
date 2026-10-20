# M03-05 — Public RTGI and equivalent backend kernels

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-01–M03-03; shared effect surface/resource contracts.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp`
- `shaders/src/glsl/rt_gi.rgen.glsl`
- `lib/include/bg2e/draw/pipeline/RTGlobalIllumination.hpp`
- `shaders/src/draw/rtgi/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp](../../../../lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp), [shaders/src/glsl/rt_gi.rgen.glsl](../../../../shaders/src/glsl/rt_gi.rgen.glsl).

## Implementation work

1. Implement GI using the accepted Vulkan RT stage arrangement and equivalent Metal compute/intersection shaders; share logical material/light/environment inputs, not shader source.
2. Expose sample/bounce/quality/bias/distance/shadow-sample/noise accessors and replaceable compatible components/shaders.
3. Preserve RTAO versus RTGI mode selection and production hit-material baseline, including emission, albedo UV/cutouts and environment/light participation.
4. Produce a typed HDR indirect signal with defined neutral/unsupported output. Keep scene traversal and tables coherent with raster edits and reserve surface-profile reuse for bake.

## Verification and exit conditions

- GI and emission/bounce effects are visible on supported backends; cutouts and nested transforms agree with query/raster geometry.
- Disabled/unsupported GI restores ordinary IBL/baked-map composition and settings availability without corrupting requested configuration.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-07/M03-08 supply HDR history/denoise; M04/M08 consume the effect independently of a camera renderer.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M03-01 checkpoint handoff — accepted by lead continuation

Follow the accepted [M03-01 design](rt-design.md) and M03-04 public tracing/surface resources: Vulkan raygen iterative transport, miss and one closest-hit/any-hit triangle group; Metal compute iterative transport with nearest committed intersection queries. Share logical hit/material/light/environment semantics, not shader source. Preserve geometric GI hit normals, per-slot UV/color/channel/emission semantics and explicit LOD0 baseline. Specify linear HDR signal normalization/replacement and validity so disabled/unsupported restores IBL/baked composition. Default tracing excludes both blended transparent classes; no implicit opaque conversion or transmission implementation.

## M03-02 actual primitive contract

Read [rt-gpu-data.md](rt-gpu-data.md) with the later [M07 capacity amendment](../milestone_07_bg2e_composer/tracing-capacity-16384.md). Choose the exact device-clamped capacity up to 16384 after reserves, not small compiled effect profiles. Vulkan specializes ID0; Metal uses separate argument record buffers8/9. Keep complete arrays, retained owners, retired/drained updates and warning/neutral-texture overflow behavior. Primitive probe profiles are not the engine ABI. No global material ownership, buffer-address or array graphics-stage infrastructure is implied.

## M03-03 actual noise input

Reuse [utils::BlueNoise](blue-noise.md): CPU layer-major rank data and an explicitly shared RGBA8 color 2D atlas, with uint4 metadata and paired bg2eBlueNoise2/bg2eHashNoise2 helpers under draw/lib. Logical layers are stacked in Y; physical arrayLayers is one. Do not copy production sampler2DArray bindings or assume general image-array support. Effects own rendered-view/sample keys and disabled/missing-noise variants without texture access; the utility does not advance presentation/slot state or reserve tracing-table capacity implicitly. Retain image/sampler owners and metadata snapshots through completion.

## M03-04 actual implementation handoff

Use [rtao.md](rtao.md): public RTAmbientOcclusion, SurfaceInputs camera/explicit profiles and TracingSceneResources/TracingSnapshot now exist. Reuse dense IDs, retained material tables, scalar R16 output and explicit renderedSample keys. AO has independent quality dimensions via ExtentDomain::Scaled. No history/filtering or visible controls are implemented here. Legacy SoftShadowPass retains its separate eight-cutout-pair limit; reconcile that consumer during integration/review. Native pipeline GLSL users may include tracing.glsl without DRAW_TRACE_QUERIES; compute-query users define it first.

## M03-05 delivered contract

See [rtgi.md](rtgi.md) for actual APIs, native stages, ABI, linear HDR validity/composition and ownership. Compilation/runtime validation remains with the project lead.

## Later global default-policy amendment

The project lead directed that a default draw renderer selects GI and creates an
enabled GI component. The current headers set that policy in
`RendererConfiguration` and `RTGISettings`; the original implementation default
described above is historical. Explicit modes/settings and availability fallback
remain authoritative. See the [current review amendment](review.md#later-default-policy-amendment).
