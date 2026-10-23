# Phase 5: integration audit, measurements and release defaults

Dependency: Phase 4 source work complete. Entry: P5.1. No implicit authorization to build or benchmark. This phase completes integration of the listed techniques; it does not introduce deformation history or advanced reflection motion.

## P5.1 — audit every producer and consumer

1. Search all TemporalInput, TemporalView, beginFrame, UpscalerInput, freezeAtLimit, sceneFingerprint, signature reset and settings serialization callers.
2. Confirm legacy callers retain valid defaults and custom v1 kernels do not receive incompatible layouts. Document explicit-surface/world-profile behavior separately from camera-depth built-in enhancement.
3. Verify opaque metadata is preserved for FSR and every transparent temporal layer consumes its own coverage and retains immutable ownership. Cross-check temporal layer keys against extraction IDs; membership changes still globally reset, so no stale key can migrate to another object.
4. Check frame-slot reuse, two/three queued scenes, skipped presentation-only updates, update without record, same-recording ordering, failed submit, abandoned recordings, resize, scene detach, cleanup and device loss handling. Existing submission and ownership contracts must not weaken.
5. Inspect all global reset callers. Transform-only motion must preserve history even with enhanced formats unavailable, SDK unavailable, metadata disabled, custom GBuffer, or unsupported deformation. Document local rejection/fallback quality explicitly.
6. Do not edit already modified unrelated API/architecture files as a cleanup exercise. Limit documentation updates to the actual new contracts and reconcile existing user edits carefully.

## P5.2 — prepare reproducible validation scenarios

Use existing manual probe hosts/examples; no new CMake target. Store a concise scenario matrix and probe invocation instructions in tests/draw/temporal/README.md (preserving unrelated content). Required configurations:

- Static scene; moving cube; rotating/scaling cube; foreground revealing wall; thin alpha-cutout geometry.
- Camera pan/rotation, combined camera/object motion, explicit cut, unchanged jitter-only scene.
- Static receiver with changed AO; indirect light change due to moving object; reflected moving object on static mirror.
- Opaque plus multiple transparent layers, glass approximation, unchanged neighbor, insertion/removal, effective material and light changes.
- Native and reduced internal resolution, odd dimensions, low/high effect quality, maxFrames=1/32/1024, freeze false/true, repeated interaction before/after window expiration.
- No-metadata fallback, no-moments fallback, custom v1 kernel, unsupported SDK, invalid identity diagnostic, aborted recording, retained earlier outputs.

For each scenario record expected global reset, expected local rejection, acceptable artifact, required diagnostic view and backend status. Do not mark all scenarios passed after running only signature probes.

## P5.3 — perform only authorized native checks

If build/run authorization is absent, complete static integration work, record these checks `not_run`, and leave performance/release-validation tasks pending. Do not mark the entire plan verified.

When authorized, record actual device, driver, backend, configuration, resolution, effect settings, frame slots and exact build/probe commands. Compile paired shader profiles and run signature/settings/temporal/upscaling/synchronization/transparent probes on available Vulkan and Metal hosts. Distinguish unsupported environments from failures and passing native tests.

Verify global generation, per-pixel history/diagnostics and resulting images together. Reproduce artifacts on both adaptation-disabled and adaptation-enabled paths. Keep source-only checks separate from measurements.

## P5.4 — measure cost without changing workload

1. Compare Phase 0 fallback, local token validation, object reprojection, 2x2 rejection, adaptive neighborhood, and adaptive moments using controls/diagnostic toggles that do not change ray count, resolution or spatial denoise quality. Do not reset every frame while profiling.
2. Disable reference-signature serialization, debug outputs, validation overlays and readbacks for timing. Warm up allocations/shaders and collect a fixed sequence of at least 300 actual scene frames, reporting warm-up policy and median/p95 frame and pass times.
3. Measure CPU signature/correspondence preparation, metadata raster, temporal compute, FSR and total scene work with existing profiling. Record image/pool high-water bytes and active history/view/layer counts. Include static, object-motion and many-transparent-layer cases.
4. Compare equal hardware/backend/workload. Do not turn an unmeasured architecture estimate into a benchmark. Report unavailable timestamp support clearly.
5. Explain resource cost as bandwidth/storage versus existing RT effect generation. No claim that fewer global resets automatically reduce total GPU time: the original code still records effects, and added metadata/filtering has a real cost.

## P5.5 — optimize only demonstrated bottlenecks

Apply at most one focused optimization per measured comparison; preserve a baseline and repeat affected checks. Candidates, in this order:

1. Share immutable receiver metadata owners across signals of the same view/layer invocation; never share across distinct receiver layers.
2. Avoid allocating moments when adaptive moments are disabled; avoid metadata work when no active consumer needs it.
3. Reuse/retire temporary images through existing pool ownership instead of allocating ordinary frame resources repeatedly.
4. Reduce expected-previous-position/normal representation only with measured memory pressure and pixel evidence. Keep the four-output reference path for comparison. Do not casually replace previous-position validation with 2D vectors alone.
5. Restrict transparent metadata raster regions only if clearing/outside validity and private-depth semantics match full-extent output in regression scenarios.

Do not fuse metadata into the six-target GBuffer, share AO/GI/reflection statistical moments, add per-frame CPU readback, or expand GPU APIs solely for speculative optimization. Such changes need a separate scoped plan.

## P5.6 — finalize defaults, limitations and progress

1. Phase 0 no-global-transform-reset and freeze-window behavior are mandatory defaults. Preserve authored freeze preference and the fixed 3*maxFrames tail.
2. Enable supported built-in local identity/object reprojection only after capability checks; fallback always preserves Phase 0. Document unverified backend status if native access was unavailable.
3. Choose adaptive defaults based on recorded noise/lag evidence; without native evidence keep adaptiveEnabled=false and expose the opt-in path with honest status. Do not claim tuning was validated.
4. Document remaining limits: skinning/morphs/custom deformation; transparent/refraction composited motion; reflection virtual motion; lighting changes after freeze without a reported interaction; exact indirect influence; in-place edits not represented by existing revisions; unsupported formats/interfaces.
5. Record that the freeze window reacts to detected scene transform changes, including animation, not merely mouse/key events. Programmatic edits must use the same observation path; unrelated UI gestures must not thaw the renderer.
6. Update progress.md with every phase's implemented/source-checked/runtime status, measured results or missing validation, final APIs, deferred work and the first remaining action. If source implementation is complete but native tests are pending, final state is `implemented_unverified`, next action is the first named native scenario requiring authorization. Use `complete_verified` only for the actual recorded validation scope, explicitly listing unavailable backends.

## Deferred extensions (not steps in this plan)

- Previous skinned/morphed vertex positions and deformation-aware normals.
- Hit-distance/virtual-surface motion for reflections, separate fast/slow radiance histories and advanced chromatic clipping.
- Adaptive ray/sample allocation driven by variance.
- Replacing the custom temporal path with an external denoiser SDK.
- Broader local material/light/topology edit invalidation or exact transport-dependency tracking.

Start a separate design for any of these; do not leave the current incremental implementation unfinished while pursuing them.

## P5.1 source audit record (2026-10-23)

Status: `implemented_unverified`. This is a source integration audit, not backend,
visual, submission-failure or performance acceptance. P5.2 is next.

| Producer / consumer | Source finding |
|---|---|
| TemporalInput / TemporalView / beginFrame | Existing aggregate fields keep their defaults; sceneMotion and diagnostic flags are trailing/defaulted. Three-argument beginFrame callers remain valid; custom overrides must accept/forward the fourth argument. Acquired serial/slot/scene sequence is checked before recording. Presentation-only work does not consume the per-key motion tail. |
| v1–v5 shader families | Base/custom v1 layout remains 208 bytes. Optional v2/v3/v4/v5 layouts are selected only with their matching interface and resources. v4/v5 share 368-byte uniforms; moments and metric bindings are optional distinct pipelines. Missing optional pipelines fall back without enabling adaptive defaults. Existing Phase 2–4 source guards cover layout/settings contracts. |
| Explicit world surface | World-profile inputs remain v1/v2; rigid v3, adaptive v4 and moments v5 require camera-depth, an affine view, reproject enabled and the previous-position/normal pair. Legacy camera rejection remains active outside rigid reprojection. Do not promise enhanced world-profile reprojection. |
| Settings / signatures / fingerprints | Adaptive tuple serialization, validation and both signature paths agree. Defaults remain adaptiveEnabled=false and temporalSurfaceEnabled=false. Transform policy omits only instance transforms; full sceneFingerprint remains transform-sensitive for frozen offscreen jobs. Effective material/light/environment/membership changes retain global reset behavior. |
| Opaque / transparent / FSR | Renderer retains opaque metadata before transparent G-buffer reuse. Each transparent extraction layer has its own retained metadata map entry and exact temporal layer key. FSR receives only the opaque motion/token owners and preserved opaque depth. Layer keys use extraction IDs while correspondence uses stable node/submesh identities; changed membership resets history before a stale layer can migrate. |
| Recording chronology | Accumulator allows an ordered predecessor within the same command recording; different unsubmitted/abandoned predecessors are rejected. Correspondence is deliberately stricter: its prior frame must have been submitted. Multiple scene records in one command therefore do not obtain rigid predecessor reuse through the renderer; normal RenderLoop records one scene per command. No claim of same-recording multi-scene rigid reuse is made. |
| Slots / queued scenes / update-only | Surface acquisition drains the acquired slot before retirement. Immutable captured view/scene correspondence is resolved at record time, and previous/next signal/moment/metadata owners survive slot reuse. Two/three-slot fixture sources exist but have not run. Update-only work does not publish a recorded predecessor. Presentation-only frames do not increment the scene sequence. |
| Submit / abandonment / resize / teardown | RenderLoop increments scene count after successful queue.submit. Submission-completion state excludes a different unsubmitted command from subsequent history. Resize/scene attachment/cuts invalidate generation and correspondence. Existing stopped-boundary drains and cleanup ownership remain; no new per-frame wait, readback or submission was added. Failed submit/device loss propagate through existing error/cleanup paths; automatic device-loss recovery is not implemented or asserted. |
| Unsupported metadata / SDK / custom GBuffer / deformation | Availability changes select legacy/local fallback without globally resetting for rigid transforms. Custom incompatible replay drops enhanced metadata. FSR optional motion failure uses camera vectors; unavailable/failed upscaling uses spatial output. Skinning/morph/custom vertex deformation has no exact prior geometry contract; token/rigid metadata cannot repair it. Transparent/refraction motion and reflected virtual motion remain approximations. |

Reset caller review and scoped fixes:

- Model editor mouse callbacks now let renderer signatures classify changes instead
  of invalidating on every mouse event. Hover/selection alone does not explicitly
  reset; rigid manipulation follows the transform policy. Explicit camera cut,
  reset/frame-selection and general scene edits retain their resets.
- Model editor numeric node matrices now use queueTransformEdit, a typed queued
  operation guarded by owner, document generation, node membership, transform
  availability and finite matrix elements. It marks document/lightmap sources dirty
  and requests a scene frame without notifyEdited, scene attachment or history reset.
  Light-node transforms still change packed light data and reset through signatures.
- Generic queueNodeEdit/notifyEdited remains conservative, including partially
  failed material/geometry/component edits. Composer input already avoids an explicit
  reset; its generic edits still reset. The manipulation example's onChanged belongs
  to DrawMaterialEditor, so its material reset remains. Light-pose, baked-resource,
  geometry reload, settings-import and general arbitrary edit callbacks retain resets.
  This is not a promise that arbitrary application callbacks preserve history.
- No production render/gpu architecture, CMake, bake algorithm, authored freeze
  setting, sample/ray count or release default was changed in this step.

Validation: integration_source_checks.py passed, plus phase4_source_checks.py,
phase3_source_checks.py and temporal_surface/phase2_source_checks.py. These are
source assertions and independent Python arithmetic only. C++/shader compilation,
Vulkan/Metal probes, pixels, SDK behavior, queued execution, editor interaction,
failed submission/device loss and performance remain `not_run`.

## P5.2 protocol preparation record (2026-10-23)

Prepared the current validation entry point in tests/draw/temporal/README.md:
18 scenario rows with global-reset/local-rejection/artifact/diagnostic expectations
and independent Vulkan/Metal not_run states; deterministic warmup/motion/tail/renewal
schedule; F0/L2/R3/A4/M5/U comparisons; size/quality/maxFrames/freeze configuration
axes; existing exact probe invocation map and per-run evidence record. Existing
README sections were preserved as historical source handoffs. No target was added.

Documentation checks verified all 18 rows, required axes, probe symbols against
existing headers and local links; git diff --check passed. This step changes only
documentation and makes no native, SDK, editor, visual or timing claim. Next is P5.3;
without explicit compilation/execution authorization its native checks stay not_run.
P5.4–P5.6 remain pending; default-off adaptation/replay settings remain unchanged.

## P5.3 authorization and native status record (2026-10-23)

Authorization review completed; native acceptance remains pending. The request to
continue the next step does not override AGENTS.md's explicit compilation restriction
or this plan's separate native-execution requirement. The no-authorization branch
of P5.3 applies: retain not_run results and preserve the prepared protocol. No build,
shader compiler, native fixture, example, editor, SDK dispatch or benchmark was run.
No hardware capability was tested, so no backend is classified unsupported or passed.

| Native validation category | Existing entry / evidence required | Vulkan | Metal |
|---|---|---|---|
| Engine and paired shaders | Authorized platform build; actual commands, compiler output and shader deployment | not_run | not_run |
| Settings and reset/signature parity | Settings codec and temporal invalidation probes; actual assertions/results | not_run | not_run |
| v1/v2/v3/v4/v5 scalar/HDR and diagnostics | Temporal, metadata, reprojection, footprint and adaptive probe variants; actual pixels/format status | not_run | not_run |
| Slots, chronology, retained owners | Queued and correspondence probes; submissions, counts and retained-output evidence | not_run | not_run |
| Raster/cutout/transparent receivers | Temporal surface probe and S04/S10; per-layer coverage, opaque ownership and clean images | not_run | not_run |
| Motion and SDK/fallback | Opaque motion probe plus separate SDK-enabled/disabled scenario; actual SDK/device/status | not_run | not_run |
| Editor and visual convergence | S01–S18 at the specified configuration axes; generation, local decisions and ordinary images together | not_run | not_run |
| Controlled faults / teardown | Suitable existing host; distinguish abandoned recording from failed submit/device loss | not_run | not_run |

Device/GPU, driver, native backend availability, SDK availability, build configuration,
shader deployment and actual native commands/results are **not recorded: no run**.
The macOS workspace and shader sources are not proof of Metal/Vulkan capability.
No evidence artifacts or native timings were produced. Previously passing Python
source/reference checks and P5.2 documentation checks retain only their original
scope; they do not change any native cell above or any S01–S18 result.

First future authorized native action: use the existing initialized/stopped host and
its paired shaderRoot after an explicitly authorized platform build. Record exact
build/deployment commands and device/driver/backend data, then call
`bg2e::draw::tests::runTemporalProbe(engine, shaderRoot)` for the legacy baseline
and run S01/F0 static convergence with generation and pixel evidence. A false return
means unavailable support, not pass; preserve the reason and continue only with
supported combinations. Then follow the complete P5.2 invocation map and matrix,
including separate backend records, rather than treating the baseline as full
acceptance. If only one backend is available, record the other as unsupported only
after an actual capability check with its reason; otherwise keep it not_run.

Next sequential step: P5.4 measurement authorization/evidence gate. Without explicit
benchmark authorization and actual matched native runs, median/p95, memory high-water
and CPU/GPU pass comparisons remain not_run. Do not fabricate measurements, tune
release defaults or perform speculative P5.5 optimization. P5.3 runtime acceptance
and P5.4–P5.6 performance/release acceptance remain open; the plan is not verified.

## P5.4 measurement preparation and evidence status (2026-10-23)

Protocol/status review recorded; measurement execution remains `not_run`. No
benchmark, native run, compilation, instrumentation change or optimization was
performed. P5.3 has no native acceptance results and no explicit benchmark permission
has been supplied. Both backends retain not_run, rather than zero cost or unsupported.

Future matched experiment, after explicit authorization and native correctness checks:

- Use the P5.2 saved scenes and F0/L2/R3/A4/M5 modes on the same GPU, backend, driver,
  build, output/internal/signal sizes, frame-slot count, rays, noise, denoise settings,
  camera/object trajectory and active signals. Include static, moving opaque object
  and many-transparent-layer workloads. Fix and report the transparent layer count.
- Warm up each configuration for at least max(64,maxFrames) actual scene records,
  including representative allocations and motion; exclude these from statistics.
  Then replay the same saved 300-scene-record schedule in every mode. Presentation-only
  frames do not count. Use separate matched freeze=false/true runs; do not reset each
  frame or change authored freeze/maxFrames to simulate motion. Use P5.2's longer
  3*maxFrames schedule for freeze correctness; 300 frames cannot verify its 1024 case.
- Disable reference-signature comparisons, diagnostics, validation overlays and
  fixture readbacks. Capture correctness images in a separate untimed run. Record
  actually active interfaces and fallback reasons; absent optional modes are not
  interchangeable measurements. Keep SDK state fixed, or run separate matched SDK
  on/off comparisons with the same scene schedule.
- Report CPU and GPU separately; for each label record valid sample count, median
  and nearest-rank p95 (sorted sample ceil(.95*n), one-based). Obtain at least 300
  valid scene-total samples; missing/dropped pass timestamps stay explicitly missing.
  Report sample counts per pass/signal/layer and use per-scene aggregates for total
  pass cost. Do not pool arbitrary per-layer events or sum nested timing scopes.
- Existing renderer.temporalSurface and effect.temporal.record GPU scopes identify
  metadata and temporal work; renderer.reconstruction covers upscaling/fallback.
  renderer.recordLinear and renderer.update are broader CPU intervals. There is no
  dedicated scope in TemporalCorrespondence.cpp or TemporalSignatureCache.cpp:
  report isolated preparation cost as unavailable from these labels unless an
  existing suitable capture can separate it. Never rename inclusive CPU update time
  as isolated correspondence/signature time or reconstruction as pure SDK time.
- Record active history/view/layer counts and actual signal/receiver extents. Report
  unique image owners, queued/predecessor/exported owners and pool high-water bytes
  without double-counting shared metadata. Separate content-byte estimates from
  measured allocator/driver residency; unavailable allocation telemetry is missing,
  not zero. Retained debug outputs from correctness runs must not contaminate timing.

The v3 implementation already combines rigid reprojection with four-tap filtering;
there is no production UI toggle for nearest versus four-tap. An isolated
object-reprojection/2x2 cost claim requires the recorded Phase 3 matched baseline
artifact in the same host; otherwise mark that incremental comparison unavailable.
Likewise A4 includes current-neighborhood response: no independent neighborhood-only
runtime toggle is asserted. Use actual supported modes, not artificial zero-cost
subtraction or analytical fetch counts presented as measured bandwidth.

| Measurement / evidence | Vulkan | Metal |
|---|---|---|
| Device/driver/build/configuration and authorized commands | not_run | not_run |
| Warm-up and >=300-scene matched static/motion/transparent traces | not_run | not_run |
| CPU scene/update/signature/correspondence median/p95 and scope limits | not_run | not_run |
| GPU metadata/temporal/reconstruction/scene median/p95 and timestamp support | not_run | not_run |
| Per-key counts/extents and measured image/pool high-water/residency | not_run | not_run |
| F0/L2/R3/A4/M5 and isolated legacy-baseline comparison artifacts | not_run | not_run |

No speedup, memory reduction or demonstrated bottleneck can be concluded. Temporal
metadata/filtering/moments add storage and bandwidth while original effects still
record their ray work; fewer global resets alone do not prove lower frame time.
Previous analytical byte/fetch estimates remain estimates, not benchmark results.

Next: P5.5 evidence gate only. With no measured bottleneck, preserve the current
implementation and record optimization deferred; do not speculatively change pools,
metadata representation/sharing, replay coverage or default selection. Actual P5.4
measurements and P5.3 native acceptance remain outstanding.

## P5.5 measured-bottleneck decision (2026-10-23)

Evidence review completed: optimization is **deferred**, not implemented or measured.
All six P5.4 evidence categories remain not_run on both backends. There is no
matched baseline, measured hot path, peak allocation result or native pixel result
supporting a change. Analytical image costs and Python reference results do not
satisfy this step's measured-bottleneck requirement.

| Candidate, in plan order | Decision now | Evidence required before one focused change |
|---|---|---|
| Share immutable metadata within one view/layer | deferred | Show duplicate owners/allocations or bandwidth cost for the same receiver; prove no cross-layer sharing and retained-owner correctness. |
| Avoid unused moments / metadata replay | deferred | Demonstrate actual unused allocations or replay in the selected active mode; separate pool-retained images from new work and verify optional-mode fallback. |
| Pool/reuse temporary images | deferred | Measure allocation churn and pool high-water with slot/in-flight/exported owners; preserve acquisition retirement and immutable earlier outputs. |
| Compress previous geometry representation | deferred | Demonstrate memory pressure and establish pixel-error/disocclusion/normal baselines before altering the four-output reference path. |
| Restrict transparent replay regions | deferred | Show material replay cost at fixed layer count; verify clearing, outside coverage, private depth and opaque/transparent isolation against full-extent output. |

No candidate is selected and no speedup or allocation reduction is claimed. Preserve
current resource ownership, representation, pool behavior, replay coverage, shaders,
ray/sample counts and authored/default settings. Do not merge metadata into the
six-target GBuffer, share statistical moments across signals or add per-frame CPU
readbacks/new GPU APIs to work around missing evidence.

Reopen this decision only after the authorized P5.3 correctness baseline and P5.4
matched experiment provide an attributable bottleneck. Save the baseline artifact,
make at most one focused optimization, rerun the affected native ownership/pixel
checks and repeat the same workload/timing/memory capture. Record negative or neutral
results as such; do not broaden optimization scope to manufacture a benefit.

Only plan documentation changed for this review. Documentation consistency and
whitespace checks passed; no source/native test or benchmark was rerun. Next: P5.6
source/default/limitation handoff, retaining implemented_unverified and an exact
future authorized native action. P5.3 acceptance, P5.4 measurements and any justified
P5.5 optimization remain outstanding; this decision does not certify release quality.

## P5.6 final source/default handoff (2026-10-23)

Final state: `implemented_unverified`. Source work and non-executing Phase 5 handoffs
are finished; runtime acceptance and release/performance validation are not complete.
No C++/shader compilation, native fixture, editor, SDK or benchmark was run.

| Decision / API contract | Final source disposition |
|---|---|
| Phase 0 transform policy | Global history survives instance-transform-only changes; full fingerprints retain transforms. Real animation/programmatic transforms use the same observation path; UI events alone do not thaw. Material/light/environment/membership and explicit cuts retain resets. |
| Freeze / defaults | Temporal enabled=true, maxFrames=32, freezeAtLimit=false. Authored values remain intact during motion; each recorded key rolls during motion plus exactly 3*maxFrames subsequent valid consecutive scene records. Saturated per-pixel trust is still required to freeze. |
| Optional replay / adaptation | temporalSurfaceEnabled=false and adaptiveEnabled=false remain unchanged. Adaptive tuple stays .03125/.25/3; it is not empirically tuned. Compatible v1 fallback remains mandatory; availability gates v2–v5 and independent optional diagnostics/moments. |
| Final input / ownership APIs | beginFrame adds defaulted sceneMotion; TemporalInput appends optional metadata/diagnostic flags; TemporalView captures view/projection. UpscalerInput retains strong opaque motion/token owners. Exact view/signal/layer keys and immutable previous/current owners remain separate. |
| Settings / observability | Validated adaptive setter/codec and cached/reference signatures agree; historyGeneration, availability/status and optional token/acceptance/motion/geometry/metric displays provide scoped evidence, not a visual pass. |
| Optimization / measurements | P5.4 actual timing/residency remains not_run; all P5.5 candidates deferred. No default capability enablement, speculative optimization or measured improvement claim. |

Limitations are consolidated in doc/api/draw/TemporalAccumulator.md: rigid transforms
exclude skinning/morph/custom deformation; glass/refraction and reflected virtual
motion are approximate; exact indirect influence is not tracked; frozen lighting can
stay stale without observed motion or a reported edit; in-place changes absent from
existing revisions need explicit invalidation. Unsupported formats/interfaces and
world/custom-v1 fallback retain their limits. Renderer correspondence needs a submitted
predecessor; controlled fault recovery is not certified. General app edit callbacks
may still reset; model-editor typed transform/input paths are separately guarded.

Final checks rerun: Phase 2 source contracts/arithmetic, Phase 3 source/footprint
reference, Phase 4 references/diagnostic contracts and P5.1 integration source guards
all passed. Documentation defaults/resume links and diff whitespace checked. These
are source/Python checks only; both Vulkan and Metal remain not_run for all native
categories. No device/backend capability or performance was established.

First remaining action is P5.3 native baseline, requiring explicit build/run
authorization: use the existing platform build/deployed paired shaders, record actual
device/driver/backend/commands, run runTemporalProbe(engine, shaderRoot) in a stopped
initialized host, and execute S01/F0 static convergence with generation and pixel
evidence. Continue the full P5.2 matrix afterward. Then perform authorized P5.4
matched measurements; reopen P5.5 only for a demonstrated bottleneck. Do not cycle
through status-only P5.3–P5.6 again or mark complete_verified without actual results.

## Authorized native subset (2026-10-23, after P5.6)

The user's explicit runTemporalProbe/S10-F0 authorization supersedes historical
no-authorization records for those checks only. Debug bg2e and paired GLSL/Metal
shaders compiled. runTemporalProbe passed on Metal and Vulkan/MoltenVK; S10/F0 passed
scoped raster/AO temporal assertions on Metal, while Vulkan had raster success with
AO capability unavailable (partial, exit 3). No FSR/SDK/other configuration matrix
or benchmark was run. Full commands, logs, snapshots, host correction and exact
128x64/two-slot/32-frame-limit scope: [native record](validation_2026_10_23/README.md).
State remains implemented_unverified. Next remaining scenario S01/F0 requires
separate authorization; do not rerun passed baselines without a reason.
