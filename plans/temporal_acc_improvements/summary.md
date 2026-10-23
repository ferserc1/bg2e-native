# Incremental temporal accumulation improvements

Status: `implemented_unverified`. Source implementation is finished. The user cancelled all further agent tests and owns the remaining manual validation; await their report. Do not run builds, shader compilation, source checkers, native probes, scenarios or benchmarks without a new explicit request. Preserve [prior native evidence](validation_2026_10_23/README.md): Vulkan results there are MoltenVK on this Mac, not validation of the user's intended Vulkan target. No additional implementation is scheduled; address reported defects if requested.

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

Use `(nodeId, submeshIndex)` as the CPU correspondence key. `RenderQueue::objectId` is assigned from extraction position and must not become a persistent identity. Use monotonic nonzero `uint32_t` surface tokens on GPU, never encode IDs in float/half. Token zero is uncovered/invalid. Replace tokens on geometry/material/texture revision changes in later phases, not on rigid transform changes. Do not recycle tokens during a renderer lifecycle; exhaust safely with a drained reset, never wrap to an old token. P1.1 retains the cursor across ordinary history resets and destroys it only during RendererDeferred::cleanup after its existing device drain and per-slot snapshot release; exhaustion throws and requires that cleanup before a fresh lifecycle.

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

P1.5 adds optional GPU token/actual-acceptance views and raster/chronology probe sources. All Phase 1 steps are implemented_unverified; native acceptance is pending and replay remains default off. No claim is made about indirect-light changes on stationary receivers.

P2.2 uses four-color versioned metadata shaders with retained motion/previous geometry. P2.3 consumes expected previous geometry through optional draw.temporal.v3; incapable/legacy moving domains retain Phase 0 fallback and static domains retain P1 token validation. Reflected transforms reject locally; replay remains default off. See phase_2_object_reprojection.md for ABI and compatibility details.

P2.3 adds a separately versioned 352-byte v3 camera-depth consumer and diagnostics, retains v1/v2/custom fallback, and uses previous-camera-space geometric tolerance. Continuous camera reprojection requires the full supported metadata path; explicit cuts and conservative projection resets remain. Replay is still default off; compilation/native/visual/performance acceptance is pending. P2.4 opaque motion sharing is now implemented_unverified; next: P3.1 only.

P2.4 adds optional borrowed/retained opaque motion/token input to UpscalerInput and a separately gated fullscreen FSR selector. Nonzero finite object vectors keep internal pixel units; all other pixels retain camera/depth motion. FSR records completion/projection chronology and derives dispatch jitter from the recorded projection. Bridge/SDK unchanged, replay default off, transparent/refraction motion approximate; compilation/native acceptance remains pending.

P2.5 completes source regression/oracle coverage, separate opt-in motion/previous-geometry display views, a two/three queued-slot scalar/HDR source fixture and a reproducible standard-library source/math checker. The Phase 2 plan contains the final acceptance matrix; all C++/shader compilation and native formats/pixels/SDK/visual/performance checks remain not_run. At the P2.5 handoff, Phase 3 was not started; subsequent source completion is recorded below.

P3.1 defines `draw.temporal.footprint.v1` as an algorithm contract with an executable
standard-library reference, not a registered shader ABI. Candidate-center token
mapping, individual rejection and normalized support are specified and oracle
checks pass. Production v3 sampling remains nearest until subsequent integration;
P3.2 is next. P3.3 signal/count/confidence reduction and all native acceptance
remain pending; replay remains default off.

P3.2 adds paired v3 positive previous-camera depth validation with unchanged world
and normal tolerances. The nearest candidate uses a reusable predicate; P3.3 will
apply it to the full footprint with correct signal/count/confidence reduction.
No optional tolerance widening or ABI/default change. Source/math checks pass;
compilation and native/visual/performance checks remain not_run. Next: P3.3.

P3.3 integrates the validated bilinear footprint in paired built-in v3 kernels:
individual retained-token/geometry/signal checks, normalized valid weights,
confidence-premultiplied reflection spatial reduction, valid-black GI, minimum
history count and internal geometric support. Temporal blend/freeze/rolling rules,
v1/v2/custom ABI and default-off replay remain. Source/math checks pass; compilation,
native/visual/performance acceptance remain not_run. Next: P3.4 only.

P3.4 adds ordered rejection colors and partial/full footprint support to optional
built-in v3 LocalHistory captures; legacy/custom behavior and display ABI remain.
A native scalar footprint fixture and expanded reference oracle cover masks,
rejection priorities, invalid inputs and edge cases. Source/math checks pass;
C++/shader/native/visual/performance checks remain not_run. No new persistent image,
default or ordinary synchronization/readback. Next: P3.5 only.

P3.5 completes Phase 3 source review and analytical cost accounting. Existing
CPU/GPU temporal timing remains; explicit available diagnostics gain separate
capture/display scopes through the existing profiler. The phase file contains
per-unique-image memory formulas, ownership/high-water caveats, source acceptance
and a controlled future nearest/four-tap comparison. Reference/source/cost checks
pass; all native/visual/measured-cost acceptance remains not_run. Replay stays off.
Next: P4.1 only in phase_4_adaptive_accumulation.md; preserve the validated
nonadaptive footprint, default-off adaptation and literal freeze outside the
motion window. P4.2–P4.6/statistics/clipping/moments are not part of that next step.

P4.1 adds default-off adaptive settings with strict atomic validation, reflected
catalog/legacy-payload support and cached/full serialization parity. Optional
separately versioned v4 (368-byte uniform, same bindings) adds support-based
bounded rolling weights while preserving literal freeze and exact disabled v3
behavior. V1/v2/v3/custom compatibility, fallback/ownership and default-off replay
remain. Reactivity is zero and sigma reserved; there are no neighborhood/clipping/
moment images or trust-count changes yet. Source/reference checks pass; compilation
and all C++/shader/native/visual/performance acceptance remain not_run.
Next exact step: P4.2 only in phase_4_adaptive_accumulation.md.

P4.2 adds paired v4-only 3x3 raw-current neighborhood statistics with same-token,
finite coverage/current-depth/normal validation, conservative reconstructed-world
consistency, uniform moments/bounds/count and explicit insufficient-support flag.
GI black and reflection confidence/miss semantics stay separate. Internal results
are not consumed until P4.3 and may be optimized away; no output, alpha/count,
ABI/image/default or ownership change is claimed. Reference/source checks pass;
all compilation/native/visual/performance acceptance remains not_run.
Next: P4.3 only; variance-gated clipping/reactivity and trust count, no P4.4 moments.

P4.3 consumes current statistics for variance-gated history clipping and unclipped
reactivity in v4. HDR confidence stays separate; scaled RGB is bounded to finite
FP16 range, with chromatic/target-luminance limitations documented. Adaptive count
now represents fractional effective trust, distinct from nominal CPU frame count;
saturated freeze and exact disabled v3 behavior remain. Source/reference checks
pass; native/visual/performance acceptance remains not_run. No ABI or image added.
Next: P4.4 only, optional moments with bounded log-HDR statistic and same validated
footprint/alpha/freeze, preserving neighborhood-only fallback and custom contracts.

P4.4 adds optional separately versioned v5 RG16F moment transport with format/status
fallback to neighborhood-only v4. Visibility and log-HDR statistics are bounded;
color/moments share accepted taps and alpha, while moment-domain confidence stays
separate from linear clipping. Per-key owners/pruning/freeze and custom resource
contracts remain. Extra image cost is 4 bytes per signal pixel per unique retained
moment image, not total feature memory. Source/reference checks pass; compilation
and native/visual/performance checks remain not_run. Defaults stay off.
Next exact step: P4.5 source regressions and acceptance scenarios only.

P4.5 completes adaptive regression/reference sources and native visual acceptance
specification. New matched scalar/HDR and v4/v5 queued fixtures cover signal,
confidence, moment/key retention, drops, support and fallback; half-quantized
sequence references pass. Controlled convergence/noise figures are model-only.
Production sources/defaults are unchanged in this step. Native/visual/performance
acceptance remains not_run, with explicit per-case/scene matrix and host entries.
Next exact step: P4.6 only, opt-in adaptive diagnostics/cost review and P5.1 handoff.

P4.6 completes Phase 4 source work with optional canonical v4/v5 metric-capture
families and CurrentWeight/Reactivity/Variance/EffectiveCount views (IDs 6–9).
Captures preserve core/custom layouts and history mode, select the exact current
signal/layer and expose bounded visual encodings; missing capture is amber.
Analytical image/fetch cost, ownership/settings/defaults/limits and native matrix
are documented. Reference/source checks pass; all compilation/native/visual/cost
validation remains not_run. Replay/adaptive defaults stay off. Phase 4 is
implemented_unverified, not native-accepted. Next exact source step: P5.1 only.

P5.1 integration audit (2026-10-23): reviewed v1–v5 compatibility, world-profile
fallback, exact opaque/transparent/FSR ownership, recorded-scene chronology and reset
callers. Model editor mouse input and numeric transforms now preserve rigid-motion
history; general edits remain conservative. Integration and Phase 2–4 source checks
passed. Native/editor/queued-failure/performance acceptance remains not_run. See the
Phase 5 audit ledger for same-recording correspondence and deformation limitations.
Next exact source step: P5.2 only; no implicit build/runtime authorization.

P5.2 validation protocol (2026-10-23): tests/draw/temporal/README.md now records
18 scenarios, expected global/local behavior and acceptable limitations, per-backend
not_run states, matched fallback/v2/v3/v4/v5/SDK modes, deterministic scene schedules,
resolution/quality/maxFrames/freeze axes, existing probe calls and an evidence template.
Probe names, local links and matrix coverage were checked without compilation or native
execution. Next: P5.3 authorization/status gate only; measured tuning remains pending.

P5.3 status review (2026-10-23): applied the explicit no-authorization branch.
The Phase 5 ledger records eight native categories as not_run independently for
Vulkan and Metal; no device, driver, SDK capability, native commands or results were
established. First future authorized baseline: runTemporalProbe(engine, shaderRoot)
and S01/F0 after an authorized platform build, followed by the full P5.2 protocol.
Next: P5.4 measurement authorization/evidence gate only. Native acceptance, actual
measurements and release validation remain pending; the plan is implemented_unverified.

P5.4 measurement preparation (2026-10-23): recorded matched modes/workloads,
warm-up and 300-scene sampling, median/p95 and missing-timestamp rules, profiler
scope limits and unique-image/pool accounting. All six measurement categories remain
not_run for Vulkan and Metal. No benchmark or bottleneck evidence exists; no speedup
or memory improvement is claimed. Next: P5.5 evidence gate only, preserving source
and defaults in the absence of a demonstrated bottleneck. Native acceptance and
actual measurements remain outstanding.

P5.5 evidence review (2026-10-23): all five optimization candidates are deferred;
no measured baseline/bottleneck or native pixel/ownership acceptance exists. The
Phase 5 ledger records evidence needed to reopen each candidate and the required
one-change matched retest. No implementation/default changes or performance claims.
Next: P5.6 source/default/limitation handoff; native validation and actual measurements
remain pending, with implemented_unverified preserved.

P5.6 final handoff (2026-10-23): defaults and exact transform/settling policy were
checked against source; API documentation consolidates rigid/deformation, indirect/
reflected/glass, frozen-lighting, unreported-edit and fallback limitations. Detected
scene motion includes animation/programmatic transforms, not UI gestures. Phase 2–4
and integration Python source/reference checks passed again; no compilation/native
execution occurred. Final state implemented_unverified; resume at the first explicitly
authorized P5.3 legacy baseline and S01/F0, then the full matrix and measurements.

## User-directed transparent depth correction (2026-10-23)

The user reported multi-GB growth when enabling transparent faces and requested
restoring read-only use of opaque depth. This supersedes the private transparent
depth seeding policy above: the transparent GBuffer and temporal metadata replay
load the acquired slot's preserved opaque depth with depth writes disabled.
Transparent metadata retains shared ownership of that image rather than allocating
a depth image per item. Per-layer signal histories and color metadata remain.
Depth-based transparent consumers use preserved opaque depth; this is a change
from reconstructing a separately written transparent surface. Visual effects and
actual memory savings require user validation. No build or native check was run.

## Shared transparent effect domain correction (2026-10-23)

The user's latest rendering contract supersedes per-submesh transparent histories
and metadata. RendererDeferred now has two effect domains per signal/view: opaque
0 and shared transparent 1 (TransparentTemporalLayer). All transparent and
solid-transparent items are stably sorted by projected camera depth of uploaded
bounds centers. A single ordered GBuffer receiver replay chooses the last covered
surface per pixel while loading opaque depth without writes. Shadows, AO, GI,
reflections, temporal accumulation and denoising then run once for the complete
transparent layer. Temporal metadata also has one shared transparent image set.

Ordered per-item color/refraction composition reuses those effects. This adds a
receiver raster replay but no per-object screen images or signal histories.
Standalone TemporalAccumulator clients retain arbitrary view/layer keys. Renderer
diagnostics use 0/1; legacy nonzero diagnostic values alias transparent 1. Custom
GBuffer replacements must support the trailing RasterFrame::transparentItems batch.
Depth continues to follow the opaque read-only contract from the preceding fix.
Source implementation is complete; no build, source-test suite, native probe or
VRAM/performance measurement was run. Updated fixtures remain unverified.
