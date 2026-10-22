# Incremental temporal accumulation improvements

Status: implementation plan only. No engine changes, compilation, shader compilation, or runtime validation have been performed for this plan.

## Product decision

Object transform changes must stop resetting the entire interactive scene history immediately, before object motion vectors are implemented. Temporary trails and stale AO/GI/reflection values are explicitly acceptable. Preserving convergence outside the changed area takes priority over strict temporal correctness.

Keep `freezeAtLimit` as a user preference. While objects move, temporarily allow rolling updates without changing `maxFrames`. After the last motion frame, allow another `3 * maxFrames` rendered scene frames, then freeze again. Repeated motion renews the window. This reduces stale frozen lighting; it does not guarantee exact convergence.

## Read and resume protocol

1. Read this file and [progress.md](progress.md). These are the only entry files the user needs to provide.
2. Read repository `AGENTS.md` and applicable nested instructions. Read the phase file named by `Next step` in progress.md, then the source files listed in that phase.
3. Run `git status --short`; preserve all unrelated modifications. Check the recorded work against current source rather than assuming the previous agent finished it.
4. Execute the exact next numbered step. Work sequentially; do not skip phase dependencies. A user request for one phase does not authorize silently executing all later phases.
5. Before editing implementation, mark the step `in_progress` in progress.md and record the intended files. After each completed step, record actual changes, checks, limitations, and the exact next step. If interrupted, record the first unfinished operation and partial edits.
6. Treat implementation completion and runtime verification as different facts. Compilation requires explicit user authorization. Without it, finish source review and leave runtime checks `not_run`, with an exact scenario for a future authorized run. Do not repeatedly request authorization or pretend a source review is execution.
7. A phase may be `implemented_unverified` and the next independent source step may proceed. Do not enable a capability-dependent default on the claim of native validation that has not happened. Record blocked implementation dependencies precisely.
8. Never use chat history as the only record of an API decision, test result, deviation, or remaining issue. Put it in progress.md and, if it changes the contract, the relevant phase file and this summary.

## Scope and mandatory constraints

- Work in the current `bg2e::draw` pipeline and its paired GLSL/Metal shaders. Do not migrate or modify a legacy `bg2e::render` API. Do not introduce high-level dependencies into `bg2e::gpu`, modify bake accumulation, or change vendor SDKs.
- Do not change any CMake file. New files must use existing auto-discovery. Follow repository GPL headers and C++/shader style. No public imgui dependencies.
- `skills.md` was requested by AGENTS.md but was not present during planning. Search for it again before engine edits. If still absent, record that fact and read the current draw architecture documentation; do not invent its contents. Current source is newer than some architecture descriptions.
- No compilation, shader compilation, native probe execution requiring a build, or performance benchmark without explicit user authorization. Static inspections and documentation checks are allowed.
- No sub-agents unless separately requested by the user or applicable instructions.
- Preserve existing public callers by adding optional trailing input fields/defaults; keep legacy shader-interface support or explicitly introduce a versioned interface. Do not feed new bindings to incompatible custom kernels.
- Preserve frame-slot ownership, ordered GPU submission, retained output owners, abandoned recording rejection, and actual scene-frame sequencing. No ordinary-frame `waitIdle`, immediate submissions, CPU image readback, or GPU counter readback.
- No requirement for RT hardware, AI accelerators, optical flow, subgroup operations, float atomics, or native FP16 arithmetic for these temporal techniques. Existing effect generation may independently require RT features.

## Phase order

| Phase | File | Independently useful result | Dependency |
|---|---|---|---|
| 0 | [phase_0_preserve_history.md](phase_0_preserve_history.md) | Transform-only edits preserve global history; freeze temporarily opens | None |
| 1 | [phase_1_local_validation.md](phase_1_local_validation.md) | Stable surface tokens; moving receivers reject locally; static receivers survive | Phase 0 |
| 2 | [phase_2_object_reprojection.md](phase_2_object_reprojection.md) | Rigid-object motion and previous-surface validation; shared FSR motion | Phase 1 |
| 3 | [phase_3_disocclusion.md](phase_3_disocclusion.md) | Validated 2x2 reprojection with confidence and edge handling | Phase 2 |
| 4 | [phase_4_adaptive_accumulation.md](phase_4_adaptive_accumulation.md) | Per-signal adaptive weights, clipping, and luminance moments | Phase 3 |
| 5 | [phase_5_integration_and_tuning.md](phase_5_integration_and_tuning.md) | Integration audit, measured cost/quality, release defaults and documented limits | Phase 4 |

Phase 0 must remain useful and shippable even if all later work is postponed. Do not hold it for perfect disocclusion or reflected motion. Skinning, morph targets, virtual reflection motion, adaptive ray counts, and full denoiser-library integration are explicitly deferred extensions, not implicit requirements of Phase 5.

## Non-negotiable architecture decisions

### Full scene identity is not history invalidation policy

`sceneFingerprint()` is used by frozen offscreen jobs. It must still change when any transform changes. Keep the full cached serialization and the full reference serialization equivalent, including transforms, view, and history generation. Introduce a separate policy comparison for interactive history reset; do not weaken full scene equality to implement Phase 0. Fingerprint queries must not consume pending motion/change notifications.

### Surface identity is persistent

Use `(nodeId, submeshIndex)` as the CPU correspondence key. `RenderQueue::objectId` is assigned from extraction position and must not become a persistent identity. Use monotonic nonzero `uint32_t` surface tokens on GPU, never encode IDs in float/half. Token zero is uncovered/invalid. Replace tokens on geometry/material/texture revision changes in later phases, not on rigid transform changes. Do not recycle tokens during a renderer lifecycle; exhaust safely with a drained reset, never wrap to an old token.

### Metadata is generated in a separate raster pass

Keep the existing six-color G-buffer contract and its depth index intact. Add a draw-level temporal metadata pass with one integer token attachment in Phase 1; Phase 2 adds motion and previous-surface attachments. This avoids assuming nine simultaneous G-buffer color attachments. Check each attachment format/usage and the combined limits. Unsupported/custom passes retain Phase 0 behavior; never reintroduce global transform resets as a fallback.

### Histories follow rendered work

Presentation-only frames do not count. Per-key temporal sequence and submitted recording ownership are authoritative. Previous transforms must come from the matching recorded/submitted scene state, not an arbitrary previous `update()` or the last reuse of the current slot. Rejected/abandoned recordings are not valid previous states. Multiple frames may be queued in one ordered graphics stream; do not require CPU completion before consuming a submitted predecessor.

### Physical changes and correspondence changes differ

Motion vectors identify the same receiver. They cannot by themselves detect changing AO, indirect light, or reflected content on a stationary receiver. Keep finite rolling updates during interaction and then add signal adaptation. In Phase 0 retain global resets for material, topology, membership, light, environment, settings, and lifecycle changes. Phase 1 does not relax those policies. Later expansion of local edit policy requires explicit documented cases and tests; it is not necessary to solve transform motion.

## Source map

- `lib/src/bg2e/draw/RendererDeferred.cpp`: signature decision in update; beginFrame; opaque and transparent composition; resetHistories; fullTemporalSignature; sceneFingerprint.
- `lib/include/bg2e/draw/RendererDeferred.hpp`: targets, state, lifecycle, optional helpers.
- `lib/src/bg2e/draw/TemporalSignatureCache.{hpp,cpp}`: immutable snapshots, exact cached equality, transform bytes, reference parity.
- `lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp` and `lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp`: per-key histories, uniforms, settings, submission ownership.
- `shaders/src/draw/lib/temporal.{glsl,metal}` and `shaders/src/draw/temporal_accumulation/*`: paired compute kernels.
- `lib/src/bg2e/draw/pipeline/EffectSettings.cpp`: temporal reflection/settings metadata.
- `lib/include/bg2e/draw/RenderQueue.hpp`, `lib/src/bg2e/draw/RenderQueue.cpp`: extraction IDs and world transforms.
- `lib/include/bg2e/draw/ObjectDataBinding.hpp`, corresponding source, `pipeline/GBuffer.cpp`, `shaders/src/draw/raster/gbuffer.*`, `shaders/src/draw/lib/bindings.*`: reproduce material coverage exactly.
- `lib/include/bg2e/draw/Upscaler.hpp`, `lib/src/bg2e/draw/FsrUpscaler.cpp`, `shaders/src/draw/upscale/motion.*`: camera-only FSR path to extend in Phase 2.
- `tests/draw/{temporal,temporal_invalidation,upscaling,update_synchronization,transparent_regions,resource_pools}/`: existing manual probes and contracts; no general test runner is assumed.
- `doc/architecture/MainLoop_render_draw_gpu.md`, `doc/architecture/draw_advanced_lighting.md`, `doc/architecture/draw_offscreen_rendering.md`: architectural context.

## Cost and hardware envelope

Phase 0 adds CPU comparisons and a small per-key scheduling state; no extra render targets or RT work. Later phases add a metadata raster pass and bandwidth, then extra history taps and moment images. Performance figures are unknown until measured on real hardware; do not claim millisecond savings.

At 1920x1080, one R32_UINT, R32F, or RG16F image is about 8.29 MB; one RGBA16F is 16.59 MB. At 3840x2160 these sizes quadruple. Count current/retained/in-flight images, every active signal/view/layer, and temporary-pool high-water allocation. Do not describe one image's size as the total feature cost. Metadata can have lower signal-resolution consumers but is generated at internal receiver resolution.

Provide diagnostic views for token, motion, rejection cause, confidence, effective history length, update window, and adaptive weight. Use existing GPU timing and CPU profiling facilities. Never add synchronous per-frame GPU telemetry readback.

## Validation policy and acceptance

Each phase defines source checks and native scenarios separately. A missing backend, shader, format, SDK, or unavailable probe is `unsupported`/`not_run`, never a pass. Use both Vulkan and Metal where available. Do not modify existing unrelated documentation changes: the checkout already had many modified README/API/architecture files at planning time.

The mandatory qualitative acceptance is that continuous object movement no longer repeatedly destroys convergence in the static background, even when enhanced metadata is unavailable. Later phases must improve receiver reuse and disocclusion without breaking that guarantee. Global generation staying constant alone is insufficient evidence of pixel correctness.

Reference material for implementation reasoning: [AMD temporal denoising](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/denoiser/) and [NRD integration guide](https://github.com/NVIDIA-RTX/NRD/blob/master/README.md). These are conceptual references, not dependencies or authorization to import their code.
