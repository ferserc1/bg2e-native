# Temporal accumulation implementation progress

## Resume pointer

- Plan: [summary.md](summary.md)
- Current phase: Source implementation finished; remaining validation is user-owned manual work.
- Next step: **Wait for the user's manual validation report. No agent tests scheduled.**
- State: `implemented_unverified`
- Last completed plan step: authorized runTemporalProbe and scoped S10/F0; see validation_2026_10_23/README.md
- Next exact action: review the user's reported manual results when supplied. Do not run builds, shader compilation, source checkers, native probes, scenarios or benchmarks unless the user explicitly requests them again. Preserve prior evidence without treating Vulkan/MoltenVK on this Mac as validation of the user's intended Vulkan environment.

## Authorization and workspace

- Current user instruction: all remaining agent tests cancelled; the user will validate manually and report results. Previous run authorization is revoked for further executions.
- Historical authorization: runTemporalProbe and S10/F0 explicitly authorized by user; necessary native host/library/shader compilation included, no CMake source edits or benchmarks. Earlier no-authorization records below are historical.
- Prior authorization history: P5.6 final source/default/limitation handoff requested 2026-10-23; no compilation/native/benchmark authorization. P5.5 measured-bottleneck evidence review requested 2026-10-23; no compilation/native/benchmark authorization. P5.4 measurement protocol/status review requested 2026-10-23; no benchmark/build/native authorization. P5.3 authorization/status review requested 2026-10-23; native compilation/execution still not authorized. P5.2 documentation/scenario preparation requested 2026-10-23; no build/runtime authorization. P5.1 integration audit and scoped model editor transform-path fixes requested 2026-10-23; no compilation/native execution. P4.6 adaptive diagnostic/cost/source review requested 2026-10-23; completed without compilation. P3.1 specification/oracle completed previously. P2.5 source implementation completed previously. Previous P1.5 work: Added optional renderer/accumulator diagnostics, paired diagnostic kernels and native/internal probe sources; updated API/architecture/plan docs. No compilation/shader compilation/runtime authorization; no CMake changes.
- Compilation/shader compilation: authorized as necessary for the named native checks; Xcode Debug bg2e and paired shaders built successfully on 2026-10-23. No blanket future build/matrix authorization inferred.
- Native runtime: named subset executed; exact scope/results in validation_2026_10_23/README.md. Benchmarks: not_run.
- CMake changes: prohibited unless explicitly requested.
- skills.md: repository search repeated on 2026-10-23; still absent. Only root AGENTS.md was found; no nested instructions were found. Fallback architecture documents read: doc/architecture/MainLoop_render_draw_gpu.md, draw_advanced_lighting.md, and draw_offscreen_rendering.md. Current source is authoritative where those documents describe older behavior.
- P2.5 implementation-start `git status --short`: existing modified/untracked P2.1–P2.4 source/shader/probe/documentation paths; preserved. Earlier clean-checkout records refer to their original steps.

## Fixed decisions

- Object-motion trails are accepted in Phase 0; no global reset for transform-only edits, including fallback paths.
- Keep full transform-sensitive scene fingerprints and exact cached/reference serialization parity.
- Freeze preference remains unchanged; temporarily roll during motion and for 3 * maxFrames subsequent rendered scene frames.
- No changes to maxFrames for thawing; no invalidating setters during interaction.
- Stable CPU key is nodeId/submesh; GPU token is a nonzero monotonic uint32.
- Separate metadata raster pass; preserve the six-color G-buffer and depth index. Experimental renderer replay requires temporalSurfaceEnabled=true (default false until native validation/default tuning); availability/status are separate from the request. P1.4 token consumers are implemented; optional v3 motion/previous-geometry consumption is implemented_unverified in P2.3.
- Phase 0 camera reset policy remains unchanged. Phase 2 enables continuous-camera reprojection on supported built-in paths.
- Global material/light/environment/topology/membership reset policy remains unless explicitly scoped otherwise in a later approved change.
- Deferred: deformation history, virtual reflection motion, adaptive ray counts, external denoiser integration.

## Phase ledger

| Phase | Implementation | Source checks | Vulkan runtime | Metal runtime |
|---|---|---|---|---|
| 0 | implemented_unverified (P0.1–P0.5 source work finished) | P0.5 source review; native probe compiled | runTemporalProbe passed (scoped) | runTemporalProbe passed (scoped) |
| 1 | implemented_unverified (P1.1–P1.5 source complete) | P1.5 static ABI/source review passed | not_run | not_run |
| 2 | implemented_unverified (P2.1–P2.5 source complete) | P2.5 static review/source-math checks passed | not_run | not_run |
| 3 | implemented_unverified (P3.1 specification; P3.2–P3.5 source complete) | P3.5 source/math/cost audit passed | not_run | not_run |
| 4 | implemented_unverified (P4.1–P4.6 source complete) | P4.6 source/reference/diagnostic/cost audit passed | not_run | not_run |
| 5 | P5.1 implemented_unverified; P5.2 protocol prepared; P5.3/P5.4 execution not_run; P5.5 deferred; P5.6 source handoff finished | Phase 2/3/4 + integration source/reference checks passed; native host built | S10/F0 raster pass, AO unsupported; rest pending | S10/F0 scoped raster/AO temporal pass; rest pending |

## Current step work record

- User cancelled all remaining agent testing and will perform manual validation.
- No further builds, shader compilation, source checks, native probes or benchmarks
  are scheduled or authorized by prior permission. Await the user's result report.
- Source implementation and existing native evidence are preserved. Overall status
  remains implemented_unverified; no unexecuted scenario is marked passed.
- Prior Vulkan results were Vulkan/MoltenVK on Apple M4 Max, not acceptance on the
  user's intended Vulkan target. Actual target-environment validation is user-owned.

## P0.5 final verification and handoff (2026-10-22)

Phase 0 status: `implemented_unverified`. The source implementation and regression
fixtures are ready for an authorized native validation session. This status does
not assert successful compilation, backend support, visual convergence, exact local
invalidation, object reprojection, trail removal or measured performance.

| Checklist | Source evidence and result |
|---|---|
| Full identity preserved | Exact equals/cache prepare bodies match HEAD. Both serializers preserve the original full byte order; world matrices are omitted only for policy payloads. sceneFingerprint body is unchanged, including view/generation. Passed source comparison. |
| Reset policy and reference modes | historyCompatible is exact equality minus instance transforms. Independent full/reduced baselines and accepted-state fallback cover mode switches. Motion is detected by node/submesh buckets, not persistent extraction IDs. Passed source review; native parity assertions not_run. |
| Authored settings and bounded counts | No motion-driven setter calls. maxFrames bounds, count calculation, generation checks and freeze setters match HEAD. Only effective uniform freeze changes. Passed source comparison. |
| Exact tail | Motion stores 6 for maxFrames=2; six subsequent static records roll while storing 5,4,3,2,1,0; the seventh can freeze. Renewed motion restarts at 6. Limits 1/1024 bound the window at 3/3072. Reviewed arithmetic and fixture pixel expectations; native pixels not_run. |
| Per-key rendered chronology | Counter belongs to History and is computed after original submitted/consecutive/camera/profile/generation validation. beginFrame only stores slot motion, never decrements a history. Skipped/abandoned/reset predecessors contribute no countdown. Original validation and publication code preserved. Passed source comparison/review. |
| Camera and retained owners | Accumulator camera rejection and renderer recordLinear body unchanged. Invocation previous/next ownership, output owners and slot handling unchanged. FSR code and its camera-only vectors untouched. Passed source comparison. |
| Paired shader compatibility | TemporalUniforms remains 208 bytes with flags at offset 192; GLSL/Metal consume counts.z as before. No shader changes or new resources/bindings. Passed source review. |
| Synchronization and scope | No new production waitIdle, immediate submit, CPU image/counter readback, render target, pass or queue behavior. Existing fixture-only submissions/readbacks stay in tests. No CMake, GPU, legacy render, bake or vendor edits. Passed diff/source comparison. |
| Probe integrity | Existing GPL headers and runtime_error messages preserved, obsolete motion-reset assertion replaced. New include targets, source delimiters and plan-entry links checked. Largest-bound final pixel tolerance distinguishes an extra rolling update. Passed static checks only. |
| Native and visual acceptance | Compilation, shaders, native probes, wall/cube visual scenario and timing not_run on both Vulkan and Metal. No host/device/format support was established. |

Application integration review (P0.5 historical findings; model editor input/numeric
transform paths are superseded by the P5.1 fixes above and its phase-file audit):

- `apps/model_edit/src/AppDelegate.cpp::queueInput` still explicitly invalidates
  after every accepted input callback, including gizmo work and caught failures.
  Its `EditorActions.cpp::notifyEdited` also invalidates and reattaches the scene
  after general/partial edits. Those paths can still globally reset during an
  interactive transform operation. They were not removed because their callbacks
  mix arbitrary edits, camera/picking and failure handling; deleting them wholesale
  would weaken existing conservative behavior.
- `apps/bg2e_composer/src/AppDelegate.cpp::queueInput` currently does not call
  invalidateHistories; it tracks manipulation/camera changes for dirty/lightmap
  state. This differs from model_edit and must not be described as one shared reset
  behavior. Other composer general edit/reset paths still require case-by-case review.
- `DefaultRenderLoopDelegate::applyUpdates` does not automatically invalidate every
  queued operation. Its direct reset is the final-color value path. Effect settings
  callbacks and queued settings imports in RenderSettingsPreferences preserve resets,
  including partial failures. Manipulation example callbacks also cover light poses,
  material edits and arbitrary operations; their explicit resets remain authoritative.
- Consequently, the core renderer's transform-preserving policy is source-complete,
  while end-to-end interactive acceptance remains pending. Validate through a
  transform-only scene update without explicit invalidation first, then separately
  exercise actual editor/gizmo paths and record any integration resets. A future
  application fix must distinguish the event's mutation kind and failure semantics.

Exact future authorized validation:

1. Build the engine and paired shaders with the existing platform setup only after
   explicit permission. In a stopped initialized host call runTemporalProbe and
   runTemporalInvalidationProbe with its shaderRoot. Report false/unavailable as
   unsupported, not pass. Run the offscreen fixture with backend argument `vulkan`
   or `metal` and second argument `deferred`; its frozen-motion job must reject its
   second sample with unchanged global history generation.
2. On each available backend, converge a static wall with a moving cube in front.
   Move only the cube continuously without an explicit application reset. Static
   background convergence must survive globally, with metadata absent (Phase 0).
   With freeze enabled, stop and inspect exactly 3*maxFrames subsequent rendered
   scene frames before refreezing; insert presentation-only gaps and renew motion.
3. Repeat with freeze disabled, multiple transparent/view/signal keys, no active RT
   effect, unavailable optional temporal/FSR capability, abandoned recordings and
   update-without-record. Material/texture/mesh/membership/light/projection/explicit
   edits must retain resets. Frozen offscreen mutation must still reject.
4. Separately exercise model_edit/composer/manipulation inputs and distinguish
   application invalidations from core signature resets. Record actual backend,
   device, format/kernel status, diagnostics and visible AO/GI/reflection/FSR trails.
   Accepted trails and finite-window approximation do not count as exact convergence.

Next source step is P1.1 only. Phase 1 metadata generation and enhanced defaults
are not implemented or enabled by this handoff.

## P0.1 integration-boundary audit (2026-10-22)

### Full identity, reset policy, and fingerprints

- `TemporalSignatureCache::prepare` retains immutable camera/lights/environment/settings and ordered instance components. Its fast path compares worldMatrix bytes; its slow path matches `(nodeId, submeshIndex)` buckets and calls `next->equals(*_latest)` to reuse an exact snapshot. These comparisons MUST remain transform-sensitive. `TemporalSignatureState::equals` currently compares geometry, transform and effective material components as well as the global components.
- `TemporalSignatureState::flattened` serializes camera; each instance's geometry prefix, material identity, transform, material values, RT inclusion and textures; then lights/environment/settings. Its lazy byte cache is full identity, not the future reduced reset payload.
- `RendererDeferred::update` is the policy boundary: after extracting/sorting the slot queue it calls `prepareTemporalSignature`, compares against `_historyState` using `equals`, optionally compares `fullTemporalSignature` against `_historySignature` or the accepted state's flattened bytes, validates full-byte AND reset-decision parity, then calls `countResetCauses` and `resetHistories` if changed. Finally it publishes `_historyState` and the optional full reference baseline. P0.2 must change only the reset-policy comparison and add motion observation plus a separate reduced serialization/baseline. Reuse-disabled and validation-enabled paths must make the same decision.
- `countResetCauses` currently labels differing transform bytes as `temporal.reset.objectMotion`; `update` increments `temporal.signatureResets`. P0.2 must count transform-only changes separately as observations, preserving other reset causes.
- `RendererDeferred::sceneFingerprint` calls `prepareTemporalSignature(_queue)` when reuse/validation needs it, returns flattened or full reference bytes, checks exact parity, then appends view and history generation. It can advance the derived cache `_latest` but does not publish `_historyState`. Preserve that separation. `_queue` is the last extracted queue: this method does not re-traverse the live graph. A transform edit is captured by the next extraction, whereas live effective settings can be observed by an intervening fingerprint call. Do not claim live-graph mutation polling from this API.
- `fullTemporalSignature` includes ordered extraction object IDs, submesh/node IDs, mesh/BLAS/material identities, world matrices, effective packed materials, RT inclusion, texture/sampler pointers, lights, environment, camera configuration/extents/upscaler requirements and effect/noise/temporal/virtual denoise settings. Keep its exact existing format. The reduced reference must omit only instance world matrices. Camera view remains outside the global payload; projection and camera identity remain inside.
- `DefaultOffscreenApplicationDelegate::render` calls the base update before `active.sceneFingerprint()`. For zero timestep it rejects empty signatures and changes from `_sceneSnapshot`, including transforms and history generation, before recording the linear sample. `prepareFrame` separately freezes renderer generation, scene identity, serialized settings and output extent and explicitly invalidates at job initialization. No weakening of these checks is allowed.
- Other fingerprint callers are the temporal-invalidation probe; base `Renderer` supplies the virtual contract. Repository-wide searches found no other production consumers. `equals` is used for cache publication and renderer policy; `flattened` is used for reference fallback/validation and fingerprints.

### Explicit resets and interaction callbacks

- `resetHistories` increments `_historyGeneration`, invalidates the accumulator and resets the upscaler. Direct callers are `update` and `invalidateHistories`; `resetUpscaler` delegates to `invalidateHistories`. Preserve attachScene, resize, environment replacement, explicit camera mode/parameter changes and lifecycle paths. Cleanup separately clears accepted/cache baselines and owners.
- `DefaultRenderLoopDelegate::enqueueRendererUpdate` queues arbitrary operations and requests scene work. `applyUpdates` drains conservatively when required, executes through `withRenderer`, then requests scene work; it does NOT unconditionally invalidate arbitrary queued operations. Its direct invalidation is in the bounded final-color update path. Do not remove it as a transform fix.
- `RenderSettingsPreferences::bind` installs effect-settings changed callbacks that invalidate. `enqueue` applies arbitrary validated settings and explicitly invalidates even on partial application failure. Neither is proven transform-only; preserve both. Scene/renderer replacement paths must retain their existing safety behavior.
- Repository-wide searches also found explicit invalidations in apps/model_edit, apps/bg2e_composer, examples/draw/{10_deferred_renderer,11_manipulation,12_lightmaps}, and app/DrawUv2SafeReload. They are outside the core comparison boundary and must not be removed wholesale. Concrete integration issue: `model_edit::AppDelegate::queueInput` unconditionally invalidates after its event callback, including gizmo manipulation; the same callback also handles camera/picking and catches errors. `notifyEdited` additionally invalidates and reattaches the scene for general edits. A later scoped integration change must distinguish transform-only events from camera cuts, arbitrary edits and failures. P0.1 changes none of these callers.

### Scene chronology, freeze, and later source locations

- `RendererDeferred::update` calls temporal `beginFrame(slot, frameNumber, sceneFrameNumber)` once per acquired scene update. `recordLinear`'s `resolveSignal` calls `record` with the same scene sequence/generation and `{signal, view=0, layer}` for AO/GI/reflections. Opaque layer is zero; transparent layers use `(objectId << 32) | submeshIndex`; offscreen transparent regions can skip a layer entirely. P0.3/P0.4 must account for active per-key sequences rather than presentation or slot reuse.
- `TemporalAccumulator::beginFrame` rejects backward sequence/duplicate slot serial, prepares the slot and prunes stale keys. `record` rejects another unsubmitted/abandoned recording, duplicates, nonconsecutive predecessors, generation/profile/extent/camera changes, then retains previous/next owners per invocation. Submitted predecessors may be consumed before CPU completion on the ordered stream. `invalidate` clears logical histories but retained slot invocations survive; cleanup drains. Preserve these contracts when adding per-key update-window state.
- Freeze currently goes directly from `_settings.freezeAtLimit` into `TemporalUniforms::counts.z` in `record`. Both `draw/lib/temporal.glsl` and `.metal` freeze a valid saturated pixel with oldWeight=1; otherwise weight rolls with the unchanged maxFrames. `setFreezeAtLimit`, `setMaxFrames` and enabled/threshold setters invalidate; do not use them for temporary thaw. EffectSettings reflection and both full/cached signature settings serialize the authored preference. P0.3 should alter only the effective per-record freeze flag and state.
- `RenderQueue::enqueue` assigns objectId from `_items.size()+1`, records node identifier/submesh/worldMatrix, and validates finite affine invertible transforms. The cache/reference require unique nonempty node/submesh only when temporal correspondence is active; otherwise the cache admits duplicate buckets. New motion detection must preserve that policy and cannot use extraction IDs as persistent identity. Sorting transparent queues does not redefine persistent identity.
- GBuffer has six color outputs and depth at index 6; recordLinear copies that depth and repeats receiver rendering per transparent layer. No new attachment is needed in Phase 0. ObjectDataBinding implementation is in `lib/src/bg2e/draw/DataBinding.cpp` (there is no ObjectDataBinding.cpp). Its public header retains slot snapshots and does not expose imgui.
- FsrUpscaler uses camera-only motion via paired `draw/upscale/motion` fragments, frame-slot motion targets and last scene-frame continuity. Keep its camera/reset policy intact in Phase 0. New object-motion data belongs to Phase 2.
- Read all three probe READMEs: temporal, temporal_invalidation and upscaling. The invalidation probe currently explicitly expects object motion to increase global generation; update that expectation in the later planned probe step. Existing README statements that objects globally reset describe current behavior, not completed improvements. No probe was run.

### Intended remaining Phase 0 files and checks

- P0.2: TemporalSignatureCache.{hpp,cpp}, RendererDeferred.{hpp,cpp}; retain full identity and add independent compatibility/motion operations and reduced reference baselines.
- P0.3/P0.4: pipeline/TemporalAccumulator.{hpp,cpp}, RendererDeferred call/slot plumbing as needed. Preserve custom virtual callers via compatible defaults or equivalent explicit input; no shader ABI change is needed for the existing freeze uniform.
- Later Phase 0 probe/documentation step: tests/draw/temporal/probe.hpp, tests/draw/temporal_invalidation/probe.hpp and their READMEs; relevant architecture/API documentation only where final implemented behavior changes. Re-evaluate application input paths above before claiming interactive acceptance. No settings callback removals are currently justified.
- Static audit: repository-wide symbol search, source read, source-map correction and clean initial git status. No CMake mutation, compilation, shader compilation, native execution, readback or benchmark was performed.
- Future authorized validation: on Vulkan and Metal, converge a static background to freeze, continuously move one object, stop, count exactly 3 * maxFrames subsequent rendered scene frames, then verify freeze returns; insert presentation-only gaps, renew motion during the window, query full/cached fingerprints, toggle reuse/validation, and verify non-transform edits still reset. Include multiple transparent keys, unsupported temporal/FSR fallback and abandoned recording cases. Runtime status remains not_run.

## Completion log

Plan authored on 2026-10-22 after source inspection. Only eight plan Markdown files were created. Local Markdown links and sequential phase step IDs were checked successfully. No implementation or runtime result is claimed.

Append one record per completed step with this exact information:

```text
Step ID and date:
State: completed | implemented_unverified | blocked
Files and symbols changed:
Behavior/API decisions:
Static checks and results:
Build authorization and commands, if any:
Runtime backend/device/scenarios and actual results, or not_run:
Remaining limitations/failed checks:
Next step ID, phase file, and first concrete action:
```

## Resume consistency rule

Update the resume pointer, phase ledger, current work record, and completion log together after every step. Do not leave the pointer on a completed step. When changing a contract, update the phase document and fixed decisions too. Record user steering here before dependent implementation; never infer permission to build from permission to implement.

Step ID and date: P0.1, 2026-10-22
State: completed
Files and symbols changed: plans/temporal_acc_improvements/progress.md only; no source symbols changed.
Behavior/API decisions: existing full identity and accepted policy baseline are distinct responsibilities; preserve current full serialization and explicit general-edit resets. Integration boundaries and application-level reset risk are documented above.
Static checks and results: audited required symbols/callers, three probe READMEs and fallback architecture documents; skills.md absent; initial checkout clean. Source-map implementation correction recorded. git diff --check passed.
Build authorization and commands, if any: none; compilation/shader compilation not authorized and not run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run; exact future scenarios recorded above.
Remaining limitations/failed checks: audit only; global transform resets and authored freeze behavior are still unchanged. No runtime or performance claims. Broad model-editor input invalidation remains a later integration concern.
Next step ID, phase file, and first concrete action: P0.2, phase_0_preserve_history.md; mark in_progress and implement separate history compatibility/stable-key motion comparisons plus a reduced reference baseline while preserving equals/flattened/full fingerprint semantics.

Step ID and date: P0.2, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- lib/src/bg2e/draw/TemporalSignatureCache.hpp and .cpp: added historyCompatible, hasTransformChanges, historyPolicyBytes and shared serialize helper; removed objectMotion from global reset causes.
- lib/include/bg2e/draw/RendererDeferred.hpp and lib/src/bg2e/draw/RendererDeferred.cpp: update uses history compatibility for resets, stores Targets::sceneMotion, records temporal.objectMotionObservations, and retains separate full/reduced reference baselines; added historyPolicySignature and shared reference serialization helper; cleanup clears both baselines.
- plans/temporal_acc_improvements/progress.md: authorization, completion, limitations and resume pointer.
Behavior/API decisions:
- equals, exact cache reuse and full fingerprints retain all transform bytes. Compatibility omits only instance transforms and retains ordered geometry/object IDs, material identity/effective data, textures, RT inclusion and global components.
- Motion matches node/submesh buckets, retaining existing admission rules for duplicate/empty identity when correspondence is inactive. Active correspondence still rejects these during prepare/reference serialization. Membership changes do not produce synthetic motion. Only history-compatible changes are published as motion observations; simultaneous globally invalidating edits still reset.
- Reference-only reset decisions use the reduced reference payload. Validation checks full-byte parity, reduced-byte parity, full equality/change parity and reduced reset-decision parity separately. Enabling reference/validation after cached mode uses the accepted state's matching full/reduced serialization; fingerprint queries never replace that accepted state.
- The slot motion observation is the integration point for P0.3; no accumulator interface, authored freeze preference, shader ABI, callback reset or camera policy changed in this step.
Static checks and results:
- git diff --check passed.
- Python source comparisons against HEAD confirmed equals and sceneFingerprint function bodies are unchanged, both full serialization orders are preserved, and shared reduced paths omit only instance transform bytes. An initial comparison assertion failed because the comparison sliced a leading newline inconsistently; normalization corrected the check and it passed without implementation changes for that issue.
- Manual source review covered initial/static/transform/material/membership paths, mode switches, independent lazy byte caches, accepted-baseline publication, reset cause accounting and cleanup. Unique stable keys match independently of extraction IDs; inactive duplicate buckets preserve prior ordering semantics. These are source checks, not executed C++ behavior tests.
Build authorization and commands, if any: none; no compilation, shader compilation or native probe execution.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run. Future authorized P0.4 scenarios: translation/rotation/scale/tiny edits preserve global generation but change full fingerprints in cached/reference/validation modes; query fingerprints between edits/updates; switch modes repeatedly; verify material/texture/mesh/membership/light/projection/explicit invalidation still resets, including unsupported effect fallback.
Remaining limitations/failed checks: P0.3 thaw scheduling is not implemented; frozen saturated pixels can remain frozen during motion. Existing P0.4 probe assertion expecting a global object-motion reset is now obsolete and has intentionally not been changed ahead of that step. Application-level explicit input resets remain an integration concern. No pixel-correctness, native support or performance result is claimed.
Next step ID, phase file, and first concrete action: P0.3, phase_0_preserve_history.md; mark in_progress, inspect accumulator subclasses, forward slot sceneMotion using a compatible defaulted input, and add per-History rollingFramesRemaining with submission-aware consecutive predecessor handling.

Step ID and date: P0.3, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp: beginFrame gains trailing bool sceneMotion=false; Slot retains the motion flag; History retains uint32_t rollingFramesRemaining initialized to zero. Header documents migration for custom overrides.
- lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp: beginFrame stores sceneMotion; record computes the next per-key settling counter and effectiveFreeze after all existing predecessor validation. Added named MotionSettlingFrameMultiplier=3. The existing uniform freeze field receives effectiveFreeze.
- lib/src/bg2e/draw/RendererDeferred.cpp: forwards the P0.2 slot motion observation once in beginFrame before effect recording.
- progress.md and summary.md: resume/status update; no phase contract change.
Behavior/API decisions:
- A motion record stores 3*maxFrames and rolls. Each subsequent valid consecutive static record decrements its own predecessor counter; the record decrementing 1 to 0 still rolls. The following static record may freeze, according to the unchanged authored preference and per-pixel count/rejection.
- A fresh, incompatible, invalidated, missing, skipped or abandoned predecessor contributes no countdown. Only the new History carries a countdown; beginFrame never decrements it. Presentation-only work makes no accumulator call. An update without record cannot publish a countdown. Ordered submitted predecessors remain eligible before CPU completion; all prior submission/sequence rejection and retained output ownership code is unchanged.
- Ordinary three-argument callers remain source-compatible and default to no motion. Custom subclasses overriding beginFrame must adopt the four-argument virtual signature and forward sceneMotion if using base record; this is documented in the header. No such draw subclasses were found in the repository. Binary consumers must rebuild against the changed C++ class interface.
- Both existing GLSL and Metal kernels use counts.z for effective freeze. No shader source, binding, uniform size/layout, authored setting/reflection, maxFrames setter/weighting, camera rejection, bake code, synchronization or readback behavior changed.
Static checks and results:
- git diff --check passed.
- Static Python source comparison against HEAD confirmed that stripping the intended input/slot/transition/uniform changes restores the original accumulator .cpp exactly. This checks that existing predecessor validation, camera rejection, count calculation, settings setters, resource ownership and synchronization code remained unchanged. Renderer forwarding and trailing default input were also checked in source.
- Manual transition review: maxFrames=2 stores 6 on motion; six static records store 5,4,3,2,1,0 and roll; the seventh can freeze. Renewed motion stores 6 again. maxFrames=1 permits three static tail records, and maxFrames=1024 bounds the counter at 3072. With authored freeze=false, effectiveFreeze always remains false. Each signal/view/layer reads only its own validated predecessor, including after rejection. These are source/arithmetic reviews, not executed C++ tests.
- Inspected paired shader freeze branches; both consume the existing uniform flag. Repository search found no draw accumulator overrides. No CMake files were changed.
Build authorization and commands, if any: none; no compilation or shader compilation was requested or run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run. Future authorized runs must validate exact thaw/refreeze pixels, renewal, independent keys, limits 1/1024, presentation gaps, update-without-record, sequence gaps and abandoned recordings using the P0.4 scenarios.
Remaining limitations/failed checks: native and visual validation pending; P0.4 regression fixtures are not yet updated. Accepted Phase 0 AO/GI/reflection/FSR trails and application-level explicit input resets remain. No guaranteed pixel correctness or performance claim.
Next step ID, phase file, and first concrete action: P0.4, phase_0_preserve_history.md; mark in_progress and update temporal_invalidation/probe.hpp motion-generation expectations, then add the specified temporal-window/fingerprint/ownership scenarios and current-limit documentation.

Step ID and date: P0.4, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- tests/draw/temporal_invalidation/probe.hpp: SignatureTemporalProbe captures actual beginFrame motion; transform scenarios now require unchanged generation, changed full fingerprints and one motion observation. Translation, rotation, scale and cumulative tiny motion exercise all four reuse/validation combinations with pre-update queries and repeated mode switches. Added loaded mesh replacement reset; retained material/texture/membership/light/projection/explicit-reset assertions.
- tests/draw/temporal/probe.hpp: TemporalWindowProbe exposes protected recorded countdown state for fixture checks. Added actual scalar/GI/reflection thaw/tail/refreeze pixel assertions, renewal, independent keys, skipped-key and sequence handling, begin-without-record, maxFrames 1/1024 full tails, unchanged rolling weighting, explicit/external resets and unsubmitted/abandoned motion rejection. Existing static freeze, immutable outputs and unrelated signal/rejection assertions remain.
- tests/draw/offscreen/main.cpp: FrozenMotionProbe/checkFrozenMotion added to optional deferred branch. A real two-sample zero-timestep default delegate completes sample zero, mutates a loaded drawable at that stopped boundary and must reject sample one using the actual frozen-scene guard, with unchanged renderer history generation.
- Relevant probe READMEs, TemporalAccumulator/TemporalInvalidation API docs, and draw_advanced_lighting architecture updated for transform-preserving policy, effective freeze scheduling, custom override migration, exact frozen fingerprints and accepted AO/GI/reflection/FSR trails. Plan summary/progress updated.
Behavior/API decisions:
- No production behavior or API changed in this step. The source fixtures observe recorded CPU history through a test subclass and pixels through the existing stopped native-probe readback path; no live per-frame telemetry, synchronous production work, CMake target or framework was introduced.
- The 1024-bound fixture reuses two signal inputs and submits 3072 one-pixel tail frames. It is a correctness scenario, not a performance measurement. Its final binary16 tolerance is 0.0001, which distinguishes the expected frozen 0.25 from the 0.25048828125 value of one unwanted 1/1024 update toward 0.75.
- The offscreen case exercises the real delegate exception rather than treating fingerprint inequality alone as evidence that frozen-job validation executed. Source-only implementation is not native acceptance.
Static checks and results:
- git diff --check passed. Reviewed modified probe chronology, predecessor lifetime/slot handling, explicit current includes, overrides and real guard text against source. setMesh automatically rebuilds a loaded drawable; no redundant reload is added.
- Static Python checks passed for balanced delimiters after stripping comments/literals, existing full GPL headers, bg2e include targets, preservation of original runtime_error assertion messages, and new documentation link resolution. The obsolete object-motion global-reset expectation is deliberately replaced; unrelated require assertions remain.
- Python binary16 arithmetic verified the largest-bound pixel oracle's distinguishing tolerance. This executes only a test-oracle calculation; it is not compilation, shader execution or engine validation.
- Manual numerical review checked maxFrames=2 scalar/HDR weights, six tail records and seventh-frame freeze, maxFrames=1 replacement on every alternating tail record, renewed counters and independent-key gaps. No C++ fixture was executed.
Build authorization and commands, if any: none; compilation/shader compilation not authorized and not run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run. Future authorized execution: run temporal and temporal_invalidation probes in a stopped initialized host with paired shaders; run offscreen fixture with selected backend and second argument deferred. The 1024 case is intentionally longer. Then visually move one cube before a converged static wall with freeze enabled and disabled, including fallback and application input paths.
Remaining limitations/failed checks: all native assertions are unverified source; no backend/device or visual/performance pass is claimed. General editor/application invalidation callbacks and scene reattachment remain separate integration concerns. Accepted Phase 0 trails and approximate finite-window recovery are documented; object vectors/local receiver identity are still later phases.
Next step ID, phase file, and first concrete action: P0.5, phase_0_preserve_history.md; mark in_progress, review the complete Phase 0 implementation/probe diff and stated callback limitations, run only authorized static checks, and record the precise verification/handoff status before moving the pointer to Phase 1.

Step ID and date: P0.5, 2026-10-22
State: implemented_unverified
Files and symbols changed: progress.md and summary.md only; no production or probe symbols changed during final review.
Behavior/API decisions: Phase 0 source work is finished, with the authored freeze preference, transform-sensitive full identity, global non-transform resets and per-key submission-aware settling window preserved. Application explicit invalidations remain authoritative. End-to-end editor/gizmo and visual acceptance are not claimed. Independent Phase 1 source work may proceed under the plan without native acceptance claims.
Static checks and results: git diff --check passed. Exact source comparison against HEAD confirmed equals/cache prepare, sceneFingerprint, recordLinear and global reset hooks unchanged; full serializer byte order preserved; reduced compatibility differs only by transforms; accumulator source differs only by input/slot/counter/effective-freeze additions. Reviewed mode baselines, node/submesh matching, countdown off-by-one example, paired uniform flags/layout, retained ownership and callback limitations. Fixture delimiter/include/license/original-message checks, plan entry links and binary16 largest-bound oracle sensitivity passed. No CMake/GPU/legacy-render/bake/vendor/shader edits. Detailed checklist above.
Build authorization and commands, if any: none; no compilation or shader compilation run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run; wall/cube/background convergence, exact thaw/refreeze, fallback/custom paths, frozen offscreen mutation and native probes not_run. No device/format support or timing result established. Exact future validation scenarios above.
Remaining limitations/failed checks: no static failure found; C++/shader/native behavior remains unverified. Model editor general input and scene-reattachment callbacks can still globally reset; broad resets were not silently removed. Trails/stale indirect lighting/FSR artifacts remain accepted approximate behavior. No object correspondence metadata/reprojection or guaranteed trail elimination.
Next step ID, phase file, and first concrete action: P1.1, phase_1_local_validation.md; on the next implementation request mark in_progress, record exact files, then establish renderer-private persistent node/submesh correspondence records using captured immutable components and valid recorded/submitted predecessors. Do not start P1.2 in the same step.

Step ID and date: P1.1, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- New lib/src/bg2e/draw/TemporalCorrespondence.hpp and .cpp: renderer-private TemporalInstanceRecord, TemporalCorrespondenceFrame and TemporalCorrespondence with prepare/record/publish/invalidate operations.
- lib/include/bg2e/draw/RendererDeferred.hpp: private state forward declarations/owner and separate immutable preparedCorrespondence/recordedCorrespondence per slot.
- lib/src/bg2e/draw/RendererDeferred.cpp: prepares correspondence from the captured signature in update; resolves predecessor at recordLinear entry; publishes only after successful linear recording; resetHistories invalidates recorded chronology; cleanup releases the allocator after the existing drain and _targets.clear.
- phase_1_local_validation.md records exact reset/publication/flag semantics; summary.md and progress.md updated.
Behavior/API decisions:
- Stable key is (nodeId,submeshIndex). Registry retains captured const geometry/material owners. Resource equality ignores extraction objectId and transforms, preserving tokens for rigid motion/order shifts. Material source identity/effective values/texture-sampler bytes or mesh/BLAS/RT inclusion changes mint a new token; existing global reset policy remains unchanged.
- uint32 tokens are monotonic/nonzero. uint64 allocation cursor permits UINT32_MAX once and then holds UINT32_MAX+1 as an exhausted sentinel. Exhaustion throws before conversion/wrap; failed preparation commits neither partial maps nor cursor updates. Removal/ambiguous admission drops keys without recycling tokens. Empty/duplicate identities keep the existing active-correspondence signature error; inactive fallback skips ambiguous records and marks identitiesValid=false.
- Prepared values capture current world and identity changes but never publish scene chronology. Recorded values are separate immutable copies resolved against the latest successfully recorded linear scene only if its recording was submitted, its sequence is exactly current-1 and both prepared/previous generations match the current generation. No CPU completion wait and no frame-slot predecessor shortcut. Update-only/fingerprint calls cannot become predecessors. Unsubmitted, abandoned, reset, missing, repeated, backward or gapped chronology gives local invalid correspondence, not a global reset.
- previousWorld is copied from the matching submitted recorded record only when token/resources match. Otherwise it is current world with previousValid=false. Rigid motion preserves token/previous matrix but sets moved and correspondenceValid=false for the Phase 1 static-only rule; a subsequent stationary submitted scene can restore validity. Preparation change flags survive global resets and recording also checks captured predecessor resources.
- Logical resetHistories clears only recorded chronology and preserves registry/cursor. Exact allocator reset is destruction of _temporalCorrespondence in RendererDeferred::cleanup, after its existing device waitIdle and per-slot _targets.clear. Exhaustion recovery requires that drained lifecycle cleanup. No new ordinary-frame synchronization was added. No public token/debug API was introduced.
Static checks and results:
- git diff --check passed. Static Python checks confirmed checked-before-cast monotonic cursor, no allocator reset in invalidate, no recorded-state access/publication in prepare, submitted/consecutive/generation gates in record, no completed/waitIdle/immediateSubmit/readPixels calls in private correspondence, and cleanup drain -> slot release -> state destruction ordering.
- Source comparison against HEAD confirmed sceneFingerprint body unchanged; removing only the new private hooks restores recordLinear and resetHistories to their original bodies. Phase 0 signature/accumulator code was not edited in this step. New GPL headers, local bg2e include targets and balanced source delimiters checked.
- Manual source-path review: static and moving keys, effective resource replacement, ordered extraction ID shifts, removal/reinsertion, inactive ambiguous identities, multiple prepared frames, update-without-record, submitted-but-in-flight predecessor, abandoned/failed linear recording, sequence/generation gaps, exact UINT32_MAX boundary and failed preparation rollback. No C++ scenario executed.
Build authorization and commands, if any: none; compilation/shader compilation not requested or run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run; no capability/format probe run. Future authorized P1.5 scenarios must exercise stable tokens, resource replacement/removal, extraction-ID insertion, two slots and ordered in-flight work, discarded/abandoned work, retained snapshots and exhaustion safety. Until metadata exists, this step changes only CPU bookkeeping and cannot demonstrate pixel rejection or visible quality improvement.
Remaining limitations/failed checks: uncompiled/unexecuted CPU implementation; no native/pixel/performance pass. No metadata pass (P1.2), opaque/transparent domain integration (P1.3), enhanced history input (P1.4) or token diagnostics/probes (P1.5). Phase 0 trails and application explicit reset limitations remain. No automatic drained token rollover occurs on an ordinary frame.
Next step ID, phase file, and first concrete action: P1.2, phase_1_local_validation.md; mark in_progress on the next implementation request, read GBuffer/ObjectDataBinding material and coverage helpers plus GPU format/raster conventions, and implement only the separately gated token raster pass with integer color and private depth using paired shaders and no CMake edits.

Step ID and date: P1.2, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- New pipeline/TemporalSurfacePass.hpp/.cpp: TemporalSurfaceInstance, TemporalSurfaceResult, optional pass initialization/status, acquired-slot beginFrame, retained full-extent receiver replay, and drained cleanup.
- New shaders/src/draw/temporal_surface/surface.vert/frag.{glsl,metal}: paired built-in vertex replay and integer token fragments with material coverage.
- draw/all.hpp exports the pass; phase_1_local_validation.md documents format/contract gates, uniform ABI, lifetime and standalone API boundary. summary.md/progress.md advance the pointer.
Behavior/API decisions:
- Exact built-in GBuffer type/canonical contract required; custom/subclass contracts return unavailable. Capability checks cover one color attachment, aggregate color bytes, 2D size limits, R32_UINT Sampled|ColorAttachment and matching receiver depth DepthStencil|Sampled|TransferDst. Missing native shaders/pipeline resources latch unavailable until explicit cleanup/reinitialization. No automatic retry, renderer default, global transform reset, or metadata consumer is enabled.
- A 16-byte uint4 metadata uniform uses Vulkan set 2/binding 0 and Metal fragment buffer 8; frame/object sets 0/1 preserve drawBindingLayout compatibility, with no push constants. The GPU token remains uint32. Integer token image clears to zero, no blending; private depth clears to one, with Back culling/LessOrEqual writes/test. Invalid correspondence draws zero rather than discarding covered foreground depth.
- Vertex sources match built-in GBuffer exactly apart from Metal relative include. Material UV selection, texture sampling, decode helpers, authored albedo-alpha multiplication and alpha-test/transparent-zero-alpha predicates match original coverage. ObjectData, original GBuffer/shaders and shared helpers remain unchanged.
- Standalone opaque replay draws its original queue order with private depth. Single transparent replay can seed a separate private depth from preserved matching sampled/transfer-source opaque depth, then draws only that item; token always clears full extent and regional scissor is ignored. This is API readiness only; RendererDeferred domain integration remains P1.3.
- Each invocation gets retained/pool-leased images, per-instance temporal uniform resources and drawn snapshot owners; duplicate view/layer in a serial rejects. Returned image owners prevent reuse while retained. Caller-owned frame/object binding rings must live through completion. Ordinary record has no CPU wait/submission/readback; cleanup drains before resource release. Metadata/actual receiver depth comparison remains mandatory in P1.4.
Static checks and results:
- git diff --check passed. Python source checks confirmed exact paired vertex equality (relative include normalized), exact alpha-discard predicate equality, shared UV/sample/decode use, current include targets, GPL headers, balanced delimiters, integer uniform/clear/output declarations, format/aggregate-limit/type gates, no regional scissor and resource retention before rendering begins.
- Manual binding review: retained sets 0/1 match existing layout prefix; metadata set 2/binding 0 maps to Metal buffer 8 without frame/object collisions. GLSL fragment includes only shared struct/helper definitions and explicitly declares object/albedo/metadata resources, avoiding the original light set 2 declaration. Metal output explicitly marks uint color(0).
- Source review corrected image-usage constants to const (the GPU bitwise operators are not constexpr). No compiler was invoked. Original engine renderer/correspondence/accumulator logic, GBuffer contract, bindings, camera/fingerprint policy and CMake were not edited in this step.
Build authorization and commands, if any: none; no C++/shader compilation or native probe execution.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run; formats/kernel capabilities not_run. Future authorized P1.5 checks: equal-depth draw-order ownership, UV0/UV1/scaled alpha holes, transparent private-depth seeding/full clears, moved/new token-zero occluders, retained images across slots and custom/unsupported fallback. No hardware-support or pixel-parity pass claimed.
Remaining limitations/failed checks: uncompiled/unexecuted pass/shaders; no renderer invocation (P1.3), enhanced temporal input/rejection (P1.4), diagnostics/native scenarios (P1.5), or visible convergence improvement. The exact built-in shader contract assumes the deployed shaders are the original paired sources; unknown deformation requires custom fallback. Returned metadata depth must be checked by consumers in P1.4. Phase 0 artifacts/application-reset limitations remain.
Next step ID, phase file, and first concrete action: P1.3, phase_1_local_validation.md; mark in_progress on next implementation request, initialize/status-gate the optional pass from the current built-in receiver, record opaque metadata after opaque GBuffer and separate full-extent transparent metadata from preserved opaque depth before each corresponding temporal invocation, and retain opaque output separately. Do not introduce the v2 temporal consumer in the same step.

Step ID and date: P1.3, 2026-10-22
State: implemented_unverified
Files and symbols changed:
- RendererDeferred.hpp/.cpp: private TemporalSurfacePass owner, opaqueTemporalSurface and per-layer transparentTemporalSurfaces owners per slot, frame-latched replay flag, pass initialize/beginFrame/cleanup hooks, recorded node/submesh-to-current-ID mapping and opaque/transparent metadata replay sites.
- RendererDeferred public diagnostics: temporalSurfaceEnabled/setTemporalSurfaceEnabled, temporalSurfaceAvailable, temporalSurfaceStatus and status refresh helper.
- RenderSettingsPreferences.cpp: writable experimental Diagnostics opt-in and read-only Capabilities availability/status in the existing renderer settings target.
- phase_1_local_validation.md, summary.md and progress.md: exact integration/default/lifetime contract and resume pointer.
Behavior/API decisions:
- Experimental replay defaults off. Explicit setTemporalSurfaceEnabled(true), or its queued Diagnostics property, opts in; actual runtime availability remains separately gated by P1.2. No unverified native capability/default claim. Setter itself does not invalidate; generic settings-import resets remain unchanged. Requested state is latched at scene update so receiver domains within a frame cannot mix later UI toggles.
- Pass initializes once after the actual GBuffer replacement and frame/object bindings. Unavailable/custom receiver status remains explicit; unsupported recording falls back to Phase 0 without global transform resets or ordinary retry. Inactive-policy ambiguous/missing persistent identities disable the whole frame's metadata with a fallback reason, while existing active signature diagnostics are unchanged.
- One metadata beginFrame per acquired scene update retires pass invocations even when disabled. Slot metadata owners clear only at that retired scene slot. UI-only frames do not advance this state. Opaque metadata is generated after opaque GBuffer and before opaque effects/composition at internal receiver extent.
- Transparent processing keeps existing far-to-near order, inspection/region skipping, original GBuffer rewrite and layer key. Each non-skipped layer separately records full-extent token/private depth from preserved original opaque depth before composite. It stores its own strong image owners in the layer map; opaque metadata is never overwritten or reused as the transparent receiver. GPU timing uses the existing facility without readback.
- Original resize drain/replacement semantics remain; pass pooled images follow receiver extent dynamically. Lifecycle cleanup releases/drains pass invocations before frame/object binding teardown. The matching pair will be copied into history owners by P1.4. Current TemporalInput/v1 resolveSignal/FSR input/signatures/reset policy are intentionally unchanged; this step does not perform local rejection or improve pixels yet.
Static checks and results:
- git diff --check passed. A temporary pre-edit source snapshot supported exact comparisons: resolveSignal's v1 input/body, full fingerprint/reference signatures, prepareTemporalSignature, resetHistories and resize bodies are unchanged by P1.3. Original UpscalerInput remains unchanged.
- Static Python checks confirmed GBuffer -> opaque replay -> opaque composite order; skip guards -> item GBuffer rewrite -> transparent replay -> matching-layer composite; preserved opaque-depth argument; internal image extents; separate opaque/per-layer owners; default-off request; prepared-identity gate and pass retirement even when disabled; lifecycle drain before metadata cleanup/binding teardown.
- Current include targets, source delimiters and original license headers checked. Manual source review covered exact draw/frame descriptor set compatibility, metadata uniform bindings, per-slot/pooled ownership, full-extent transparent replay, skipped layers, resize and partial initialization cleanup. No new ordinary waitIdle/immediateSubmit/readPixels or CMake/GPU/legacy-render/bake/shader changes.
Build authorization and commands, if any: none; no C++/shader compilation or native execution requested/run.
Runtime backend/device/scenarios and actual results, or not_run: Vulkan not_run; Metal not_run; capabilities/kernel support not_run. Future authorized P1.5 scenarios must enable replay explicitly and check opaque metadata survives all transparent passes, separate/full-clear transparent domains, retained owners across slots/in-flight work, skipped region layers, resized internal extents, custom/unsupported fallback and source-versus-pixel depth agreement. v2 acceptance tests await P1.4.
Remaining limitations/failed checks: no static failure found, but new integration/pass/shaders remain uncompiled/unexecuted. Metadata is generated only on opt-in and is not consumed by the legacy accumulator. No object-motion reprojection, local rejection, native visual acceptance or performance result. General application reset/reattachment callbacks and Phase 0 trails remain as documented.
Next step ID, phase file, and first concrete action: P1.4, phase_1_local_validation.md; mark in_progress on next request, document the enhanced versioned shader binding layout and optional trailing metadata owner fields, then wire exact receiver-domain pairs into corresponding histories with depth/token rejection while retaining v1/custom/no-metadata compatibility.


Step ID and date: P1.4, 2026-10-22
State: implemented_unverified
Files: TemporalAccumulator.hpp/.cpp, RendererDeferred.cpp, shared temporal.glsl/metal,
eight new temporal_accumulation_v2 profiles, temporal/probe.hpp and README, API and plan docs.
Behavior: optional trailing retained pair; separate 288-byte v2 ABI with integer/depth
bindings 13–15 (Metal 11–13), initialization-only failure fallback; malformed pair
validation; exact opaque/transparent domain lookup; current depth agreement and
nonzero nearest reprojected token equality. Missing metadata uses unchanged v1.
Safe missing-predecessor binding uses current typed token with previous-valid flag
false. No dummy depth initialization/submission is needed because enhanced dispatch
always has a current pair. Retained History owns the consumed pair. Default replay
remains off; injected v1 accumulators do not opt in. P0 motion/reset policy unchanged.
Verification: git diff --check and static ABI/resource/profile/source checks passed.
Added source-only 3x3 receiver / 2x2 signal native probe, token-local/zero rejection,
depth mismatch and previous-UV out-of-bounds expectations. No CMake changes.
Build authorization: none. C++/shader compilation not_run. Vulkan/Metal runtime,
format support, pixel quality and performance not_run; no native acceptance claimed.
Remaining: all new code/shaders uncompiled/unexecuted; P1.5 broader diagnostics,
transparent/coverage/lifetime scenarios and authorized native acceptance pending.
Next step: P1.5 in phase_1_local_validation.md; add diagnostics and broader probes.

P1.5 started 2026-10-22: optional GPU token/restart diagnostic views through renderer reflection, native surface/chronology probe sources and source ABI review. No compilation/runtime authorization; all prior changes preserved.


Step ID and date: P1.5, 2026-10-22
State: implemented_unverified
Files/symbols:
- TemporalAccumulator.hpp/.cpp: trailing diagnoseAcceptance, TemporalDiagnosticView,
  optional independent diagnostic pipeline initialization/status, retained acceptance
  image and GPU-only recordDiagnostic. V1/v2 accumulation ABI remains unchanged.
- RendererDeferred.hpp/.cpp and RenderSettingsPreferences.cpp: reflected view/signal/
  exact-layer controls, availability, synchronized metadata snapshots, frame-latched
  options and spatial reconstruction for active diagnostic output. Replay defaults off.
- Shared temporal GLSL/Metal add conditional actual-acceptance storage; eight new
  temporal_accumulation_diagnostic profiles and paired temporal_diagnostic/view compute.
- temporal/probe.hpp/README: exact restart/acceptance colors, maxFrames=1 and malformed
  metadata pairs. New temporal_surface correspondence_probe.hpp, probe.hpp and README.
- API/architecture and plan/progress/summary docs updated; next source step P2.1.
Behavior/ABI:
- Diagnostic accumulator uses 288-byte v2 uniforms plus RGBA16 storage binding 16 /
  Metal texture 14. It stores covered/accepted decisions independently of sample count.
- Display pass uses 16-byte uint4 buffer 1, uint token texture 0, float acceptance
  texture 1, sampler 0 and RGBA16 storage texture 2 (Vulkan bindings 0/1/2/3/4).
- Safe absent float capture binding and explicit validity flag produce amber instead
  of falsely labeling unavailable histories as accepted. Actual decision colors are
  red restart/first, green accepted, black uncovered. Token colors are display hashes.
- Captures allocate only for requested LocalHistory key; display dispatch/images only
  for active views. Retained invocation owners protect metadata/history/display images.
  Optional diagnostic initialization failure preserves core v2/v1; custom v2 families
  do not silently select canonical diagnostic accumulation kernels. No ordinary wait,
  immediate submission, readback or CPU pixel/counter processing was introduced.
Source verification (passed; no compiler/runtime used):
- git diff --check; exact pre-step shader comparison after removing only diagnostic
  conditionals proves both base v1/v2 helper math unchanged. All four paired diagnostic
  profile wrappers match v2 with only the diagnostic macro added. CPU sizeof assertions
  remain v1=208/v2=288, diagnostic uniform=16. Reviewed integer and storage mappings.
- Exact source comparison preserves accumulator reset/maxFrames/freeze/position/normal
  setters and renderer resetHistories. Existing global material/light reset fixtures
  untouched. No GPU/legacy-render/vendor/CMake edits. Checked new GPL headers, local
  include targets, delimiters, frame-latched options, exact-key selection, pool owners,
  no public imgui types and no ordinary synchronization/readback.
Probe sources (all unrun):
- Internal correspondence: insertion/extraction ID shift, two submeshes, prepared vs
  recorded transforms, submitted incomplete predecessor model without waits, abandoned
  recording, retained snapshots, reset/material/removal/reinsertion token behavior.
- Native raster fixture: close parallel foreground/wall, two rasterized submeshes,
  extraction-ID shifts from insertion with conservative membership reset,
  two ordered slots submitted before fixture wait/readback, moving foreground exposes
  wall with actual local restart while neighbors accept, retained earlier metadata/
  diagnostics across slot reuse, abandoned scene, alpha holes vs actual GBuffer depth,
  separate transparent full clears and private opaque-depth occlusion, custom fallback,
  metadata disabled. Signal/normals are synthetic; depth/tokens use actual raster pass.
- Native metadata fixture: odd 3x3 receiver/2x2 signal plus diagnostic correctness at
  maxFrames=1 and malformed partial/extent/type-format pairs. Existing Phase 0 and
  material/light/projection/membership assertions remain mandatory.
Build authorization/commands: none. C++/shader compilation not_run.
Runtime/format results:
| Capability/scenario | Vulkan | Metal |
|---|---|---|
| R32_UINT sampled/color token | not_run | not_run |
| D32 sampled/private depth/transfer destination | not_run | not_run |
| D16 optional depth | not_run (not covered by new native fixture) | not_run (not covered) |
| R16 scalar, RGBA16/RGBA32 sampled/storage history | not_run | not_run |
| RGBA16 acceptance/display storage | not_run | not_run |
| CPU/native probes and visual convergence/freeze/fallback | not_run | not_run |
| Performance, peak retained memory, real hardware overlap | not_run | not_run |
Remaining limitations: all new source/shaders uncompiled/unexecuted; no native format,
visual, correctness or performance pass claimed. Manual UV0/UV1/scaled alpha,
equal-depth draw-order, skipped transparent regions, resize, real AO/GI/reflection
and actual editor/gizmo paths remain pending as documented in the probe README.
Stable identity does not validate changing indirect illumination on stationary
receivers. Replay remains default off. Object reprojection and FSR object motion
remain Phase 2 work; Phase 1 moving receivers intentionally restart.
Next step: P2.1, phase_2_object_reprojection.md; document conventions and capture
immutable previous submitted scene camera state without adding motion attachments yet.


P1.5 compilation follow-up (2026-10-22): user-reported compilation of
RenderSettingsPreferences.cpp failed because reflection does not support uint64_t.
Replaced the reflected temporalDiagnosticLayer member property with a string
accessor using exact decimal formatting and checked uint64 from_chars parsing.
The public C++ API and packed layer key remain uint64; malformed/overflow input
rejects before mutation. Updated API/probe docs. Source review and diff whitespace
checks passed; no compilation was run by the agent. The user's build failure is
recorded evidence, not a successful native validation. Next plan step stays P2.1.

P2.1 started 2026-10-22: capture immutable camera recording state in TemporalCorrespondence.hpp/.cpp and RendererDeferred.cpp; extend correspondence probe and API/phase documentation. No motion attachments, compilation or CMake changes. skills.md search repeated: absent; root instructions and draw architecture fallback read. Initial git status clean.


Step ID and date: P2.1, 2026-10-22
State: implemented_unverified
Files: TemporalCorrespondence.hpp/.cpp, RendererDeferred.cpp, correspondence_probe.hpp,
RendererDeferred API, temporal_surface README, phase_2/summary/progress documentation.
Behavior: immutable current/previous TemporalView and internal extent captured from
slot camera state at recording; preceding submitted scene completion retained, no
CPU completion requirement or recursive frame retention. Existing per-instance
snapshots now reject nonfinite/singular/non-affine matrices locally. Camera validity,
projection/extent, generation and consecutive scene checks gate previous state.
Phase 1 static-only receiver acceptance remains; no motion shader/FSR changes.
Checks: git diff --check passed; manually reviewed camera latch after upscaler prepare,
record-time resolution, ownership, exact projection comparison and finite inverse
checks. Probe source covers camera snapshot immutability, incomplete submission,
unsubmitted/abandoned recording, sequence gap, invalid camera and singular instance.
No compiler, shader compiler, probe execution or benchmark run. Vulkan/Metal not_run.
No CMake, GPU, bake or legacy render changes. skills.md absent on repeated search;
read current draw architecture fallback. Native future validation: execute internal
correspondence fixture, then test jittered two/three ordered slots and abandoned
recordings; motion pixel acceptance awaits P2.2/P2.5. No native success claimed.
Next: P2.2 in phase_2_object_reprojection.md; dedicated metadata uniforms/attachments.

P2.2 started 2026-10-22 (in_progress): TemporalSurfacePass.hpp/.cpp, paired dedicated surface shaders, RendererDeferred integration, source probe and contract docs. Preserve current P2.1 dirty paths. skills.md still absent; root instructions and current draw architecture fallback read. No compile/CMake/runtime authorization.


Step ID and date: P2.2, 2026-10-22
State: implemented_unverified
Files: TemporalSurfacePass.hpp/.cpp; five new surface_v2 GLSL/Metal shader/helper
files; RendererDeferred.cpp; temporal_surface/probe.hpp and new rigid_probe.hpp;
API/phase/summary/progress/probe documentation. Existing P2.1 dirty paths preserved.
Behavior: four separately retained sampled/color attachments (uint token, RG16F
motion, RGBA32F previous position, RGBA16F previous normal), original private depth
and full coverage/transparent semantics preserved. Capability/aggregate gates and
initialization-only versioned shader failure fallback. Dedicated 288-byte uniform
has explicit/asserted offsets, Vulkan set 2 vertex binding 0/fragment binding 1,
Metal vertex buffer 9/fragment buffer 8. Distinct Metal indices avoid ResourceSet
cross-stage entry collisions. Original GBuffer and v1 surface shaders unchanged.
Recorded previousValid/model and current/previous unjittered cameras feed metadata;
current jittered frame binding still rasterizes. Per-fragment divides, finite/w/
half-range checks; GBuffer-matched mapped normal transformed to previous space.
Nonfinite/singular/non-affine/reflected models reject locally. Supported movement
keeps stable tokens. Nonuniform affine math implemented; mapped-normal native visual
acceptance pending. No deformation/skin/custom vertex support introduced.
Temporary integration boundary: moving metadata cannot feed the unchanged P1
consumer safely. Result.legacyCompatible gates renderer domains with valid moving
receivers to Phase 0/v1 until P2.3; static domains keep P1 metadata. No global reset
or ordinary GPU wait/submission/readback added. New images remain owned per-slot/
invocation, separately for opaque/transparent domains. No FSR integration yet.
Checks passed: git diff --check; exact preservation of original surface shaders;
paired v2 uniform offsets, include targets, GPL headers, delimiters and fragment
finite/half-range/sign checks. Reviewed CPU/backend stage bindings, color capability
limits, normal-map alpha/UV coverage, full clears, retained owners and renderer
camera/model chronology. Independent standard-library numerical oracle passed odd
101x73 pixel translation sign (-10.1,-3.65), previous point recovery and rotated/
nonuniform normal recovery. Initial optional numpy attempt unavailable; no dependency
installed, successful arithmetic check used Python standard library only. This is
math/source verification, not execution of the C++/shader implementation.
Probe sources: native moved-token expectation now nonzero; new independent
rigidSurfaceReference and source assertions for translation/depth, rotated/
nonuniform normals and stationary unjittered motion. All C++ fixtures uncompiled/unrun.
Build authorization/commands: none. C++ and shader compilation not_run.
Vulkan/Metal native formats, pixels, in-flight ownership, visual convergence and
performance: not_run. Replay remains default off. No CMake/GPU/bake/vendor changes.
Remaining: P2.3 consumer transport and diagnostic acceptance, P2.4 FSR sharing,
P2.5 expanded native regressions. Reflected transforms conservatively unsupported;
nonuniform mapped normals and perspective interpolation need native verification.
Next: P2.3 in phase_2_object_reprojection.md; add compatible versioned consumer
inputs for expected previous geometry and remove the temporary moving-domain gate.

P2.3 started 2026-10-22 (in_progress): TemporalAccumulator.hpp/.cpp, renderer domain inputs, new versioned rigid GLSL/Metal compute families and diagnostics, source fixtures and API/plan docs. Preserve all existing P2.1/P2.2 dirty/untracked files. skills.md still absent; root instructions and draw architecture fallback reviewed. No compilation, native execution, CMake changes or FSR work authorized.


Step ID and date: P2.3, 2026-10-22
State: implemented_unverified
Files/symbols:
- TemporalAccumulator.hpp/.cpp: trailing optional previousPosition/previousNormal
  owners and legacyCompatible flag; independent reprojection v3 interface/prefix,
  pipelines/status, per-history metadataMode, strict pair validation, 352-byte
  uniform, previous view matrix, v3 camera-only compatibility selection and captures.
- Ten new shader files: temporal_reprojection.glsl/.metal plus camera scalar/HDR
  v3 production/diagnostic wrappers. V1/v2 shared helpers and wrappers unchanged.
- RendererDeferred.cpp: provision owned v3; exact-domain full pair snapshots;
  replace blanket moving-domain gate with v3/capability admission; recordLinear
  consistently uses captured slot camera for surfaces, effects/denoise and transparent
  projection/order. Injected legacy components retain original capability fallback.
- Native source fixture temporal/reprojection_probe.hpp; malformed pair cases in
  temporal/probe.hpp; explicit token-only scope in temporal_surface/probe.hpp;
  previous-camera tolerance CPU oracle in rigid_probe.hpp; API/plan/probe docs.
Behavior/ABI decisions:
- draw.temporal.v3 requires optional v2 support; initializes/fails independently and
  never retries on ordinary frames. Default external configs remain v1, existing
  v2/custom families receive only their own bindings/uniform sizes. Renderer replay
  remains default off. V3 defines camera_scalar/camera_hdr only, with explicit
  CameraDepth + reproject + affine view + full pair selection.
- V3 appends previousView at byte 288 (352 total) to exact v2 prefix. Sampled binding
  17/18 maps to Metal texture 15/16. Diagnostics retain storage 16/Metal14. C++/Metal
  assert offsets and GLSL explicitly fixes previousView offset. The v1=208/v2=288
  layouts remain unchanged, including diagnostic display/capture ABI.
- Reproject expected previous world point through previous jittered VP. Compare
  previous stored position/normal against expected previous position/normalized
  normal; tolerance is absolute + relative*length((previousView*expectedPoint).xyz).
  Require finite/valid metadata and history, positive/zero-to-one previous clip,
  strict UV bounds before conversion, nonzero matching tokens, previous coverage
  and current metadata-vs-actual receiver depth agreement. Reject locally to raw
  signal while storing current guides; no motion texture consumed yet.
- Only compatible v3 bypasses legacy 0.001 camera-motion rejection; exact projection,
  generation, sequence, extent/profile and explicit cut resets remain. Metadata
  mode switches reset only that key. Incapable/world-explicit/legacy moving inputs
  use Phase 0/v1, static inputs retain v2. Reflection/GI/freeze/count math unchanged.
- All consumed metadata owners live with History and invocation chains; recorded
  previous views remain immutable. No ordinary wait, immediate submit, readback,
  new global motion reset, GPU/high-level dependency, CMake/vendor/bake/FSR edits.
Source checks passed:
- git diff --check. Exact comparison to /tmp/bg2e-p23-before proves v1/v2 shared
  shader helpers, raw sample/coverage math, blend/confidence/count/freeze suffix,
  accumulator beginFrame/authored setters/rolling countdown and renderer reset/
  fingerprint/camera setter unchanged. Checked v3 profile wrappers, shader resource
  map, uniform offsets, stage/device formats, includes/GPL/delimiters and no added
  ordinary synchronization/submission/readback. Reviewed cleanup and failure fallback.
- Independent Python standard-library arithmetic passed expected previous point vs
  changed current point, prior jittered UV-to-odd receiver/reduced signal mapping,
  simultaneous view/object translation, average .25/.75=.5 and previous-camera
  tolerance .02+.01*1.5=.035. This is source/math review, not execution of new code.
Source fixture coverage (all unrun):
- New v3 fixture uses 3x3 receiver / 2x2 signal, camera translation .12 plus current
  depth/normal changes, expected .5 transport, token/normal/invalid-w/out-of-bounds/
  depth rejection, retained output, v2/v3 switch, cut/projection reset. Existing v2
  fixture adds six malformed previous-image-pair cases, validated without v3 opt-in.
- Surface fixture explicitly keeps its synthetic stationary v2 consumer; full v3
  raster/jitter/normal-map, HDR and queued two/three-slot acceptance await P2.5.
Build authorization/commands: none. C++/shader compilation not_run.
Vulkan/Metal native formats, kernels, pixels, visual quality and performance: not_run.
Limitations: no native pass claimed, affine mapped-normal approximation/reflection
limitations from P2.2 remain, release replay still off, FSR still camera-only.
Next: P2.4 in phase_2_object_reprojection.md; inspect SDK bridge motion scaling and
add retained opaque object-aware motion selection with full camera/depth fallback.

P2.4 started 2026-10-22 (in_progress): Upscaler.hpp/FsrUpscaler.hpp/.cpp, RendererDeferred.cpp, optional paired opaque-motion selection shaders, upscaling source probes and API/plan docs. Preserve existing P2.1–P2.3 dirty/untracked files. Root instructions and draw architecture fallback reviewed; skills.md still absent. Inspected unchanged local FidelityFXBridge and vendored SDK motion scaling/jitter/depth/reprojection code. No compilation/CMake/native/benchmark authorization.


Step ID and date: P2.4, 2026-10-22
State: implemented_unverified
Files/symbols:
- Upscaler.hpp/.cpp: trailing UpscalerOpaqueMotion borrowed motion/token + matching
  optional owners, strict pair/device/type/extent/usage/mip/layer/format validation.
  Original six UpscalerInput fields and aggregate callers preserved.
- FsrUpscaler.hpp/.cpp: optional motion_opaque fullscreen pipeline and read-only
  availability/status, per-retired-slot invocation owners, recorded completion/
  projection admission and jitter recovered from the captured projection. Retains
  original camera pass/SDK target, independent initialization failure fallback.
- RendererDeferred.cpp: only opaqueTemporalSurface.motion/token + owners forwarded
  to final UpscalerInput, after composition but captured before transparent work.
- Two new motion_opaque.frag GLSL/Metal shaders and native upscaling source fixture;
  API/renderer/probe/phase/summary/progress docs updated. Prior dirty work preserved.
Behavior/ABI decisions:
- Fullscreen sampled binding 0/1/2 = opaque depth/uint token/RG16F object motion;
  samplers 3/4/5, Metal textures/samplers 0/1/2. Original two-matrix push remains
  128 bytes, offsets 0/64. Same existing per-slot RG16F complete motion target.
- Valid nonzero token + finite half-range vector selects object pixels unchanged,
  including valid zero motion; other pixels run unchanged camera/depth shader math,
  including sky. Reset frames use the camera pass. Optional shader/format failure
  latches camera-only without disabling FSR or ordinary retries. Replay stays off
  unless explicitly requested; no new default or transparent vector substitution.
- Local unchanged bridge/SDK sources confirm previous-current internal pixels:
  bridge motionVectorScale=(1,1), SDK divides by renderSize, callback multiplies
  vectors by reciprocal size and history UV adds them. HDR/auto-exposure flags only,
  no jitter-cancellation/display-motion/reversed-depth flags. No bridge/vendor edits.
- Provided owners retained before optional draw, across exceptions/submission and
  until acquired slot retirement. No sampled raw metadata alone reaches the SDK;
  only the generated complete motion image is dispatched. Unowned borrowed inputs
  remain caller-owned through completion. Consistent extent/type/device/sample
  validation occurs even on optional unavailability; malformed pairs throw.
- FSR retains previous recording owner and last recorded projection. Different
  unsubmitted/abandoned history, gaps/projection changes reset without GPU waits;
  submitted incomplete or same-recording ordered work needs no CPU completion.
  Record-time reset preserves rasterized jitter; dispatch jitter derives from input
  jitteredProjection*inverse(projection), avoiding mutable later prepare state.
  Transform-only movement and metadata presence do not reset SDK history. Explicit
  cuts/lifecycle/settings/projection/gap reset behavior remains. No FSR transparency/
  refraction/reactive-mask/reflected-motion correctness claim is introduced.
Source checks passed:
- git diff --check. /tmp/bg2e-p24-before comparisons prove old paired camera shaders
  and exact camera fallback math, FidelityFXBridge and TemporalAccumulator unchanged.
  Renderer reset/fingerprint/camera-setter policies unchanged. Checked paired selector
  bindings, push assertions, integer token sampling, finite vector selection,
  validation, owner retention before reads, retired-slot release, cleanup/failure
  paths and absence of ordinary waits/immediate submits/readback/completion polling.
- Inspected local SDK host scaling, shader callbacks, reproject sign and context
  flags. Independent standard-library matrix arithmetic verified jitter recovery
  and stationary jitter-free camera motion at 101x73 and 5x3, plus internal pixel
  vector to SDK UV conversion exactly once. Checked source includes/GPL/delimiters
  and preserved six-field aggregate prefix. No CMake/GPU/vendor/bake edits.
Source fixture (not executed): runOpaqueMotionProbe uses direct fullscreen passes
without requiring the SDK/RT; odd 5x3 mixed vectors, UINT32_MAX token, valid zero,
NaN/Inf/token-zero fallback, ordinary/sky depth, camera sign/units and eight malformed
pair/owner/format/extent/usage cases. Actual SDK queued-slot retention, abandoned
recordings, interleaved preparation, resize and visual trails remain native work.
Build authorization/commands: none. C++/shader compilation not_run.
Vulkan/Metal shader/pixel probes, actual SDK dispatch and visual/performance: not_run.
No native support/quality pass claimed. Metal FSR remains unsupported; paired selector
shader source/native fixture support is separate. Transparent motion stays approximate.
Next: P2.5 in phase_2_object_reprojection.md; finish Phase 2 regression sources and
source acceptance, keeping native checks not_run unless explicitly authorized.

P2.5 started 2026-10-22 (in_progress): expand rigid/correspondence/consumer source fixtures, add missing motion/previous-position/normal diagnostic views using a separate optional display kernel, audit Phase 2 interfaces/ownership/defaults and write the source acceptance/native matrix. Intended files: temporal_surface/rigid_probe.hpp, correspondence_probe.hpp, temporal/reprojection_probe.hpp and queued fixture, TemporalAccumulator/RendererDeferred diagnostics and settings, paired display shaders and API/plan/probe docs. Preserve all existing P2.1–P2.4 dirty/untracked paths. skills.md still absent; root instructions and draw architecture fallback reviewed. No compilation, shader compilation, native execution, benchmarks or CMake changes authorized.


Step ID and date: P2.5, 2026-10-22
State: implemented_unverified; Phase 2 source work complete, Phase 3 not_started.
Files/symbols:
- temporal_surface/rigid_probe.hpp: finite/affine/singular/mirror-aware independent
  current-world-point oracle; RH zero-to-one perspective, stationary jitter,
  production camera parity, uniform/nonuniform rotated normals, simultaneous motion,
  previous depth, post-interpolation divides and odd/reduced texel-center references.
- correspondence_probe.hpp: runTemporalCorrespondenceChronologyProbe models two/
  three slot rings, submitted incomplete predecessors, prepared-only updates,
  new/revised instances, retained snapshots, cuts, projection/extent gaps, abandonment.
- New temporal/queued_reprojection_probe.hpp: native source fixture for two/three
  ordered submission batches, synthetic 3x3 receiver/2x2 scalar/GI/reflection guides,
  means/confidence/valid-black, motion-window rolling with authored freeze, retained
  outputs, aborted/update-only history and local token/display rejection scenarios.
- TemporalAccumulator/RendererDeferred headers/sources, RenderSettingsPreferences:
  append Motion/PreviousPosition/PreviousNormal enum views and optional motion owner;
  separate surface display availability/kernel with unchanged old display ABI,
  exact-domain latched selection and strict selected input validation. Histories
  drop unused motion ownership; actual display invocations retain it through completion.
- Two new temporal_diagnostic/surface paired compute shaders: signed bounded motion,
  previous-world-position/normal displays; zero/invalid/nonfinite data black. No core
  accumulation/raster/FSR shader changes or permanent confidence textures.
- Existing surface probe abandonment corrected: shared_ptr<const RecordingCompletion>
  cannot call abandon; release the unsubmitted command wrapper that owns that transition.
  No build failure/success observed. Camera-only MotionVectorGenerator comment corrected
  to require separate correspondence/local rejection rather than global motion reset.
- New phase2_source_checks.py; API/probe/summary/progress/phase docs and final native matrix.
Contract/ABI:
- Existing enum 0/1/2 and all aggregate prefixes retained; views append 3/4/5, metadata
  motion owner appends after legacyCompatible. Separate optional surface display uses
  old 16-byte uint4, uint token/selected float texture and RGBA16 output bindings.
  New display initialization failure leaves token/acceptance/core v1/v2/v3 unchanged.
- Motion maps xy=clamp(.5+xy/64), B=.5; world position .5+.5*xyz/(1+abs(xyz));
  previous normal .5+.5*xyz. Invalid/token-zero black; geometry requires w>.5.
  Missing input unavailable, wrong selected format/device/extent/usage rejected.
  Replay/effects remain explicit, default replay off. Off adds no GPU display work.
- Exact previous slots/snapshots, capability/versioned shader fallback, per-key mode
  checks, finite geometry, Phase 0 rolling/freeze, reflection confidence and all full
  scene identity/invalidation/reset rules remain as in P2.1–P2.4. No CMake/GPU/vendor/
  bake/legacy-render work or ordinary synchronization/readback introduced.
Checks actually run and passed:
- python3 tests/draw/temporal_surface/phase2_source_checks.py (standard library only):
  shader/CPU binding/profile/uniform source contracts, integer IDs, paired coordinate/
  normal/coverage rules, preserved blend/count/freeze suffix, FSR conventions, default-
  off/source includes/GPL/delimiters/direct sync absence; independent matrix/jitter,
  rotated uniform/nonuniform normals, odd sign/UV, previous-camera tolerance,
  perspective interpolation and FP16-quantized HDR/visibility arithmetic.
- git diff --check.
- Exact /tmp/bg2e-p25-before source comparisons: raster, core v1/v2/v3, FSR and old
  diagnostic shaders plus FSR/correspondence implementations unchanged. Accumulator
  record differs only by dropping unused diagnostic motion owner; beginFrame/reset/
  setters/count/freeze intact. Renderer recordLinear/full and policy signatures/
  sceneFingerprint/camera/reset methods unchanged. Reviewed optional kernel failure,
  owner retention/retired-slot cleanup, original diagnostic selection and strict input
  formats. Checked existing pool discards unsubmitted image allocations, and fixed
  the old const-ticket abandonment source API misuse.
- Independent perspective fixture motion differs from incorrectly interpolated endpoint
  motion by ~2.032 pixels, so the oracle discriminates the intended math. Queued
  analytic means remain >.003 away from raw restart and FP16 rounding stays <.003.
Build/native authorization and results:
- No C++ compilation, shader compilation, C++ CPU probe execution, native probe/SDK
  dispatch, visual acceptance or benchmark requested/run. Vulkan/Metal all not_run.
- P2.5 phase file contains the complete scenario/support table and exact future host
  entry points. Source checks are not compiler/static_assert/GPU results. CPU ticket
  rings do not measure real GPU overlap; queued batches may finish naturally.
Remaining acceptance/limits:
- Real raster v3 normal-map/perspective/alpha/transparent scenarios, actual AO/GI/
  reflection/FSR trails, interleaved prepared FSR slots, two/three native queue slots,
  device format/deployment checks, editor/gizmo callbacks and memory/timing are pending.
- Nonuniform mapped normals remain approximate/unverified, mirrors/deformation
  unsupported; transparent/refraction/reflected-content FSR motion approximate.
  Stationary receivers can still see stale changing indirect illumination. No quality
  or speed claim; default replay stays off. Prior phases remain implemented_unverified.
Next: P3.1 in phase_3_disocclusion.md, exact bilinear candidate footprint and oracle;
no P3.2–P3.5/adaptive integration work in this request.

P3.1 started 2026-10-23 (in_progress): specify the versioned validated-footprint contract in phase_3_disocclusion.md and add tests/draw/temporal/footprint_reference.py, a standard-library executable oracle. Intended changes are specification/oracle only, as required by the resume pointer; production sampling integration follows P3.2/P3.3. Clean initial git status. skills.md remains absent; root instructions and current draw architecture fallback read. No compilation, native execution or CMake changes.

P3.1 completed 2026-10-23 — specified_unverified.
- Files: phase_3_disocclusion.md, tests/draw/temporal/footprint_reference.py,
  summary.md and progress.md. Versioned algorithm contract only; no public shader
  interface/ABI registration or production sampling change.
- Checked current paired v3 nearest implementation and accumulator interface;
  specified exact candidate-center metadata mapping, strict UV/clip gate,
  per-candidate validity, raw support and normalized positive weights.
- Executed Python standard-library oracle: passed all 16 survivor masks, corner/
  fractional/center/odd/1x1/mismatched-resolution cases, invalid/nonfinite guides,
  token/coverage/count/position/normal rejects, black and tiny-support cases.
- git diff --check passed. No compilation, shader compilation, C++ execution,
  GPU/native pixels, visual acceptance or benchmarks run. Replay stays default off.
- P3.2 positive-depth predicate, P3.3 signal/count/confidence integration and
  P3.4/P3.5 diagnostics/native/cost work remain pending. No behavior/performance
  improvement is claimed from the reference oracle. Next exact step: P3.2.

P3.2 started 2026-10-23 (in_progress): paired temporal_reprojection.glsl/.metal geometric candidate predicate, footprint_reference.py depth oracle, source checks and plan records. Preserve prior P3.1 modifications. skills.md search still absent; root instructions and draw architecture fallback reviewed. Implement conservative positive previous-view depth baseline without optional footprint tolerance or incomplete signal reduction. No compilation/native/CMake changes.

P3.2 completed 2026-10-23 — implemented_unverified.
- Changed paired temporal_reprojection GLSL/Metal helpers: reusable geometry
  predicate requires finite previous-camera positions and positive depths, same
  existing world tolerance for depth, full world distance and normal cosine.
  Current nearest path invokes it; production/diagnostic scalar/HDR share it.
- No optional footprint tolerance added: zero addition baseline; optional item 4
  deferred with explicit contract requirements in phase_3_disocclusion.md.
- Expanded footprint_reference.py depth/math and paired source parity checks;
  adjusted Phase 2 source assertion for helper parameter extraction. Corrected
  synthetic reprojection/queued source fixture RH cameras using compensating
  projection translation and full inverse VP, preserving world/clip expectations.
- Checks passed: Python footprint oracle, phase2_source_checks.py, git diff --check.
  Blend/count/freeze/output suffix equality checked against unchanged legacy core.
- No C++/shader compilation, C++/native execution, visual/performance or backend
  support pass. All native checks not_run. No CMake, GPU, bake, ownership, renderer
  reset policy, v1/v2/custom ABI, settings/default or resource allocation changes.
- Boundary clarification: per-candidate predicate is integrated with nearest now;
  four-tap reads/token mapping activate with correct P3.3 signal reduction, avoiding
  incomplete reflection/count behavior. Full native depth scenarios in phase file.
- Next: P3.3 only; P3.4/P3.5 diagnostics and acceptance remain pending.

P3.3 started 2026-10-23 (in_progress): paired temporal_reprojection.glsl/.metal footprint sampling/reduction, footprint_reference.py signal/count/freeze oracle and source checks, API/plan documentation. Preserve prior P3.1/P3.2 changes. Root instructions reviewed; skills.md still absent, draw architecture fallback read. No compilation, shader/native execution, CMake edits or P3.4 diagnostics authorized.

P3.3 completed 2026-10-23 — implemented_unverified.
- Paired temporal_reprojection.glsl/.metal: four positive-weight unclamped taps,
  per-tap retained-token-center lookup using actual texture size, P3.2 geometry,
  finite signal/coverage/count rejection before sums, W threshold/renormalization.
- Scalar weighted visibility; GI coverage with valid black; reflections spatially
  premultiplied by clamped confidence with separate confidence recovery. Minimum
  valid count capped by maxFrames increments exactly once through original blend.
  Internal geometricConfidence records valid support independently from radiance
  confidence and remains reserved for Phase 4, with no image or adaptive multiplier.
- Extended footprint_reference.py reduction/freeze/count/math fixtures and full
  normalized paired sampling-block parity. Updated Phase 2 source suffix assertions
  for intentional minimum-count and token-loop changes; temporal blend/output
  suffix unchanged from oldWeight onward. API and phase/summary records updated.
- Checks passed: Python footprint_reference.py, phase2_source_checks.py,
  git diff --check; source comparison of P3.3 against saved pre-step shaders shows
  unchanged geometric helper, initial coverage/raw handling and blend/output math.
- Compilation/shader compilation, C++ native fixtures, GPU/backend support,
  visual acceptance and cost measurements: not_run. No CMake/CPU/GPU API/renderer/
  FSR/bake/vendor/reset/ownership changes. V1/v2 shaders untouched; replay default off.
- Remaining: P3.4 full diagnostic reasons and synthetic/native footprints; P3.5
  cost/handoff. Existing Phase 2 fixture pixel expectations need native review with
  four taps (strict geometric rejection can reduce support); no native pass claimed.
  Next exact step: P3.4 only.

P3.4 started 2026-10-23 (in_progress): paired v3 diagnostic reasons/colors, footprint reference rejection oracle, native footprint_probe.hpp source fixture, API/probe/plan records. Preserve prior P3.1–P3.3 edits. Root instructions and fallback architecture read; skills.md absent. No compilation/native execution/CMake changes.

P3.4 completed 2026-10-23 — implemented_unverified.
- Paired v3 shaders: diagnostic-only ordered reasons and cyan/green partial/full
  support in existing LocalHistory capture. Accepted support overrides neighbor
  rejects; otherwise fixed minimum reason across evaluated failures. Coverage
  precedes token mismatch at token-rejected diagnostic taps. Existing capture/
  display ABI, default-off replay, production output and v1/v2/custom remain.
- New footprint_probe.hpp: optional stopped native scalar source fixture, 16
  masks and distinguishable means at 3x3 receiver/2x2 signal midpoint, all ten
  reason/acceptance colors plus NaN/Inf and missing-capture unsupported result.
- Extended footprint reference: combined failure priority, rotated normals,
  transparent-edge-like uncovered guides, diagnostic color parity. API/header
  comments, probe README, phase/summary/progress updated; no new public API.
- Executed/passed: footprint_reference.py, phase2_source_checks.py (including new
  fixture GPL/includes/delimiters), git diff --check; paired footprint/color source
  checks; stripped non-diagnostic shader token comparison versus saved P3.3.
- Build/native: no compilation, shader compilation or C++/GPU execution. Vulkan/
  Metal formats/pixels, actual raster alpha/transparent coverage, native 1x1/odd/
  rotated/HDR/queued and all visual/performance acceptance remain not_run. CPU
  oracles and uncompiled fixtures are separate evidence, not native acceptance.
- No new images/formats/bindings, ordinary synchronization/readback/counters,
  CMake, GPU/backend, bake/vendor, reset, ownership or renderer-default changes.
  P3.5 timing/image-cost audit remains. Next exact step: P3.5 only.

P3.5 started 2026-10-23 (in_progress): audit paired shaders and accumulator/pool ownership; add opt-in diagnostic CPU/GPU timing in TemporalAccumulator.cpp, reproducible Phase 3 source/cost checker, API/probe/phase acceptance and P4.1 handoff. Preserve all P3.1–P3.4 edits. Root instructions and draw architecture fallback read; skills.md absent. No compilation/native execution/benchmark/CMake changes.

P3.5 completed 2026-10-23 — implemented_unverified; Phase 3 source complete.
- TemporalAccumulator.cpp adds optional stack CPU/GPU diagnosticCapture scopes only
  for admitted acceptance work and diagnosticDisplay scopes after Off/unavailable/
  missing-data gates. Existing effect.temporal.record and renderer timing remain.
  These scopes use the existing profiler/ordered timestamp API, no CPU/GPU waits,
  extra readbacks, counters or new telemetry settings. Layer detail filtering stays.
- New tests/draw/temporal/phase3_source_checks.py runs the independent footprint
  oracle, source invariants and analytical image accounting. API/probe/phase docs
  record timing interpretation, review matrix, unique-image byte formulas, retained
  ownership caveats, controlled nearest/four-tap comparison and native scenarios.
- Reviewed strict UV/clip preconversion and per-tap bounds, actual retained token
  extent, finite signal/geometry, full world/positive-depth/normal predicates,
  GI valid black, confidence-premultiplied reflection/misses, min count, freeze/
  rolling gates, mode/extent/chronology rejection, invocation/metadata retention,
  acquired-slot reuse and pool completed/unique-owner checks. No algorithm edit.
- Executed/passed: phase3_source_checks.py, phase2_source_checks.py, git diff --check.
  Exact comparison to saved pre-step accumulator after removing added scopes/include
  proves no recording/lifecycle/owner/reset/selection changes beyond instrumentation.
- Analytical only: scalar history 26 bytes/signal pixel, HDR 32; one full metadata
  domain 32+D bytes/receiver pixel, History subset already included there; optional
  capture/display 8 bytes per signal/receiver pixel. Unique sets, not resident totals.
  Up to four candidate fetch groups; no assumed 4x runtime or measured cost saving.
- C++/shader compilation, native probe/format/deployment/timestamp support, actual
  peak memory, visual quality, nearest comparison and benchmarks: not_run on both
  Vulkan/Metal. Full future acceptance matrix and controlled comparison are in
  phase_3_disocclusion.md P3.5. No native validation/default-release claim.
- Replay remains default off. No CMake, GPU/backend, bake/vendor, FSR, shader ABI,
  persistent image or renderer reset changes. Optional tolerance widening remains
  deferred. Thin/grazing/reduced-resolution support, mapped normals, deformation,
  reflected/transparent FSR motion and stationary indirect-light staleness remain
  documented limitations. Prior phase native acceptance is also still pending.
- Next exact step: P4.1 in phase_4_adaptive_accumulation.md; define default-off
  settings/serialization/validated weights and preserve the Phase 3 nonadaptive
  baseline/freeze. Do not implement later statistics/clipping/moments yet.

P4.1 started 2026-10-23 (in_progress): TemporalSettings/setAdaptiveSettings/reflection/catalog cross-validation, cached/reference signature fields, optional versioned v4 368-byte weighting kernels and renderer provisioning, settings/native source fixtures and source/math checks. Preserve prior Phase 3 edits. No compilation/shader/native/benchmark/CMake work; no P4.2–P4.6 statistics/clipping/moments. skills.md absent, root and fallback architecture reviewed.

P4.1 completed 2026-10-23 — implemented_unverified.
- TemporalAccumulator.hpp/.cpp: four trailing defaulted settings and strict atomic
  setter (finite ordered weights/range, one invalidation on change, no-op preserved),
  optional trailing v4 config, independent init/failure/cleanup and read-only
  availability/status. Per-key v4 mode=3. Existing v1/v2/v3 remain 208/288/352 bytes;
  v4 appends vec4 at 352 (368 total), new same-binding pipelines/diagnostics.
- Eight paired v4 scalar/HDR production/diagnostic wrappers; shared reprojection
  helper adds only v4 compile-time uniform/weight blocks. Accepted rolling alpha
  follows the plan with reactivity=0; saturated authored freeze skips override.
  Rejection raw restart/count one and confidence-premultiplied temporal math remain.
  Sigma is reserved; trust-count cap, neighborhood, clipping and moments deferred.
- Owned renderer provisions optional v4; replay/adaptation remain off by default.
  Missing v4 retains nonadaptive v3, without per-frame retries or new image formats.
  Legacy/custom layouts receive only their declared bytes. Pool leases distinguish
  layout/size; retained owner/chronology/rolling paths stay intact.
- EffectSettings reflection plus RenderSettingsPreferences complete proposed-tuple
  validation. Reflected weight accessors stage companion bounds so valid batch
  imports work in either order; strict atomic API rejects invalid pairs. Existing
  catalog setters/post-commit resets remain. Missing old payload fields retain
  current values/defaults. Four settings appended in identical order to cached
  equality/payload and full reference serialization after normalThreshold.
- Settings codec source fixture adds real temporal defaults, no-op/atomic generation,
  valid import orders/round-trip/legacy payload and invalid-pair/nonfinite rejection.
  Invalidation fixture adds every field in cached/full modes. Footprint native
  fixture's optional true argument requests v4 and discriminates one-tap alpha .75
  from nonadaptive .5. All C++ fixtures remain uncompiled/unexecuted.
- Executed/passed: phase4_source_checks.py independent range/staging/alpha/freeze
  math, paired weight ABI/wrappers and signature order checks, source GPL/includes/
  delimiters; phase3_source_checks.py; phase2_source_checks.py; git diff --check.
  Exact saved pre-step shared shader comparison after removing v4-only blocks is
  byte-identical. Older checkers explicitly strip v4-only blocks for v3 parity.
- No C++/shader compilation, codec CPU execution, native pixels/deployment/fallback,
  queued adaptive HDR/freeze/visual/performance checks run on Vulkan/Metal. No
  native quality or speed claim. No CMake, GPU/backend/vendor/bake/FSR changes,
  no extra image, rays, ordinary waits/readback or new global transform reset.
- Known boundary: P4.1 weights use support/minimum floor only, not signal-change
  detection. Geometrically valid stationary lighting can still trail. Count trust
  adjustment waits for P4.3. Defaults remain off. P4.1 acceptance/interface matrix
  is in phase_4_adaptive_accumulation.md; API and probe READMEs updated.
- Next exact step: P4.2 only, robust current-neighborhood statistics; do not add
  clipping/reactivity/moments or later-phase defaults in that step.

P4.2 started 2026-10-23 (in_progress): paired temporal_current_statistics helpers and v4 rolling-only call sites, independent neighborhood reference/source checks, API/probe/plan records. Preserve Phase 3/P4.1 dirty paths. Root instructions and draw architecture fallback read; skills.md absent. No compilation/native/benchmark/CMake changes; no P4.3 clipping/reactivity or P4.4 moments.

P4.2 completed 2026-10-23 — implemented_unverified.
- New paired temporal_current_statistics.glsl/.metal helpers plus v4-only include/
  call in temporal_reprojection. Row-major 3x3 raw-current samples at signal extent,
  individual bounds (no duplicate clamp), per-texel-center receiver mapping, same
  nonzero token and finite coverage/signal/current normal/depth agreement.
- Geometry contract: current receiver/metadata depth in [0,1) and D16/D32 agreement,
  finite inverse-VP reconstruction with positive w, world distance from center <=
  existing absolute + relative*current-camera-distance threshold, normal cosine.
  Conservative metric depth/lateral consistency, no new current-view ABI or widened
  tolerances. Tight support may intentionally fall below two neighbors.
- Uniform accepted weights calculate statistic mean/secondMoment/variance, statistic
  min/max, componentwise color bounds and sampleCount/varianceValid. Scalar clamp;
  nonnegative radiance luminance with RGB retained; valid-black GI included;
  reflection only confidence>1e-6/roughness-eligible radiance enters bounds. Current
  alpha/confidence stays unchanged; zero-confidence misses remain valid temporal
  samples in the main kernel. Epsilon 1e-6 and float32 arithmetic in both helpers.
- Zero/one samples cannot enable variance clipping/reactivity. Calls are only in
  accepted adaptive/nonfrozen v4 branch. Local statistics are reserved for P4.3;
  unused work may be optimized out. Reactivity is still zero and existing alpha,
  count, freeze, confidence blend, diagnostics and disabled v3 behavior are unchanged.
- New neighborhood_reference.py plus phase4_source_checks.py integration: exact
  scalar moments/clamp, 1x1/edge/odd/mismatched mappings, rejected bright/foreign/
  depth/normal/nonfinite neighbors, orthographic five/one-neighbor support, rotated
  normal, valid-black/invalid GI, reflection miss/low confidence/roughness, finite
  HDR maximum/outlier, low support and normalized GLSL/Metal helper parity.
- Executed/passed: Phase 4/neighborhood, Phase 3 and Phase 2 source/reference checks;
  git diff --check. Saved pre-step comparison proves unchanged disabled v3 tokens
  and exact previous P4.1 source after removing helper include/call. These do not
  execute shader reconstruction, C++ or native floating-point behavior.
- C++/shader compilation, nested-include native deployment, Vulkan/Metal statistics/
  projection/transparent/normal-map/HDR pixels, freeze/output parity and measured
  runtime/visual/performance acceptance: not_run. Native statistics inspection
  needs a temporary exporter test wrapper or P4.3 consumer; unchanged P4.1 outputs
  alone cannot verify unused statistics. No native wrapper execution/pass claimed.
- No settings/uniform/binding/ABI, persistent image, CPU/ownership, CMake, GPU/backend,
  renderer reset, bake/vendor/FSR changes. Replay/adaptation stay default off. API,
  probe README and Phase 4 geometry/acceptance contract updated in English.
- Next exact step: P4.3 only; use varianceValid before clipping/reactivity and add
  the specified adaptive trust-count cap. Moments remain P4.4.

P4.3 started 2026-10-23 (in_progress): paired variance-gated history response helpers and v4 integration, bounded alpha/trust count, independent clipping/reactivity references and adaptive native source expectations, API/probe/plan records. Preserve prior dirty files. Root/fallback architecture reviewed; skills.md absent. No compilation/native/benchmark/CMake work or P4.4 moments.

P4.3 completed 2026-10-23 — implemented_unverified.
- New paired temporal_history_response helpers, included only by v4, consume
  varianceValid current statistics for intersected scalar/luminance clipping and
  specified unclipped-history reactivity. Insufficient support or invalid/empty
  interval returns original history/r=0. Near-zero HDR uses componentwise bounds;
  other HDR scales RGB by luminance ratio. Original alpha/confidence is preserved.
- Explicit storage safety: scaled RGB bounded to finite FP16 +/-65504 to avoid
  new channel overflow from valid luminance scaling. This can limit target luminance;
  equal-luminance chromatic/hue artifacts remain documented, not solved.
- Accepted adaptive/nonfrozen path applies P4.1 bounded alpha then caps previous
  count to min(N,max(1,1/max(alpha,1e-6)-1)) only above alpha0. Increment occurs once.
  Per-pixel count is now fractional effective trust in existing half storage;
  CPU TemporalResult.frameCount stays nominal chronology. Rejection=1/uncovered=0;
  accepted alpha=1 can store count 2 per the specified minimum-old-trust rule.
- Literal saturated freeze skips gathering/response/count cap; motion window and
  authored settings unchanged. Persistent reaction/support/minimum floor can keep
  trust below saturation, so nominal frame count does not imply freezing. Default
  adaptation/replay remain off and disabled v3 tokens are unchanged.
- Native footprint source updated: test-only protected-guide accessor retains count
  texture; constant-current v4 masks now expect clipped .75, eight stable frames
  then GPU trust 2 vs CPU frame 9, saturated freeze and motion thaw. V3/default
  pixel expectations remain. This fixture is uncompiled/unexecuted.
- New history_response_reference.py + Phase 4 integration cover scalar/noise/step,
  sigma-zero/empty/low support, frozen/disabled/rejected bypass, maxFrames1,
  fractional trust/half range, valid-black GI, near-zero/HDR storage bound,
  same-luminance chromatic limit and reflection hit/miss/alpha preservation.
  Executed/passed: Phase 4 response/neighborhood/source references, Phase 3 and
  Phase 2 source checks, git diff --check. Saved pre-step shader comparison confirms
  unchanged disabled v3 source tokens after removing only adaptive blocks.
- C++/shader compilation, actual statistics/projection/native HDR/half trust,
  queued/freeze/transparent/raster pixels, stable-noise/lag/chromatic quality and
  cost measurements: not_run on Vulkan/Metal. No native quality/speed/support pass.
- No new ABI/settings/bindings/images/moments, C++ production/ownership, CMake,
  GPU/backend, renderer/FSR, bake/vendor or ordinary synchronization/readback edits.
  P4.2 current-neighborhood work is now used and may have runtime cost; unmeasured.
  API/probe README and Phase 4 acceptance/storage/count contracts updated.
- Next exact step: P4.4 optional per-key moments, bounded log-HDR statistics and
  versioned transport; preserve neighborhood-only v4 fallback and freeze/owners.
  P4.5/P4.6 acceptance/tuning remain separate and native work still unauthorized.

P4.4 started 2026-10-23 (in_progress): optional v5 moment bindings/pipelines and RG16F capability/dummy/history ownership, paired bounded moment transport/log-domain confidence and spatial fallback, moment source/math/native fixtures and documentation. Preserve prior dirty files. Root/fallback architecture reviewed; skills.md absent. No compilation, native execution, benchmark, CMake or P4.5/P4.6 work.

P4.4 completed 2026-10-23 — implemented_unverified.
- TemporalAccumulator config appends optional moment interface/prefix for v5,
  independent capability/status, init/failure/cleanup and typed RG16F zero dummy.
  V5 requires v4 and RG16F sampled/storage/transfer support; failure retains
  neighborhood-only v4, with no ordinary retries. Owned renderer provisions v5;
  replay/adaptation stay default off. External empty interface preserves v4.
- Same 368-byte uniform; new sampled 19/Metal17 and storage 20/Metal18 only in v5.
  Eight new scalar/HDR production/diagnostic wrappers and paired bounded-statistic/
  moment-reactivity helpers. Canonical diagnostics fail independently of core.
- Optional History.moments at signal extent, metadataMode=4 and missing-predecessor
  guard; binding leases key the distinct v5 layout. Existing previous/next and slot
  owners retain moments before dispatch through submitted chronology/retirement.
  Per signal/view/layer allocation, maxHistories/pruning and cleanup drain remain.
  No cross-key sharing or new public moment output is introduced.
- Finite visibility statistic [0,1], HDR log2(1+L) clamped [0,16], squares <=1/256.
  Reflection miss statistic zero avoids arbitrary zero-confidence RGB; moments
  describe raw input statistics, not clipped color. Current neighbors transform
  individually before averaging. Pair validation finite/in-range; negative
  half-quantized variance floors to zero, not a mismatched-domain heuristic.
- Joint moment/color candidate validity ensures same taps/support/weights; invalid
  pair rejects locally. Rejected color restarts moments; uncovered clears them.
  Same effective alpha blends both; frozen transport keeps prior moments without
  current blending. Moment-domain reactivity compares prior mean/current spatial
  mean, uses spatial variance below 4 prior effective samples and max(spatial,
  temporal) afterwards. Linear clipping bounds stay independent. Trust/freeze and
  alpha/confidence behavior otherwise unchanged.
- New moments_reference.py covers finite bounds, half precision, mean(log) vs
  log(mean), all masks/joint invalidity/same-alpha/reset/freeze and early/mature
  variance, ABI/capability/dummy/owner selection and full paired moment regions.
  Balanced feature_source.py handles nested version branches; old checkers retain
  exact v3/v4 baseline comparisons. Native footprint source optional fourth true
  argument requests v5 and asserts pairs/retained owners/frozen .25/.0625 state.
- Executed/passed: Phase 4 moment/response/neighborhood/source references, Phase 3
  and Phase 2 source checks, git diff --check. Saved pre-step shader comparisons
  after stripping moment branches show unchanged v4 reprojection/statistics tokens.
  Existing pool pixelFormatBytes accounts RG16F as 4 bytes; no pool code edits.
- Analytical cost: extra 4*S bytes per unique per-key image, 8.2944 MB at 1080p or
  33.1776 MB at 4K before retained/queued/external/pool/driver overhead. Idle pool
  limits are not live allocation caps. Optional typed 1x1 dummy is per lifecycle.
- No C++/shader compilation, native scalar/HDR/invalid moment/format-fallback,
  queued/isolation/resize/freeze/half precision, real lighting/visual/performance
  checks run on Vulkan/Metal. No native support, quality or speed pass claimed.
  No CMake, GPU/backend/vendor/bake/FSR, new authored settings/signature fields,
  ordinary waits/readbacks or global transform resets. API/probe/phase contracts
  updated. Native acceptance matrix remains open.
- Next exact step: P4.5 source regressions and native scenario/visual acceptance
  specification only, without compiling/running natively unless explicitly asked.
  P4.6 cost/tuning/default handoff remains separate.

P4.5 started 2026-10-23 (in_progress): adaptive scalar/HDR native regression source, queued v4/v5 extension with moment/key retention and dropped-history assertions, temporal-sequence reference and final acceptance/visual matrix. Preserve all prior changes. Root/fallback architecture read; skills.md absent. No compilation/native/benchmarks/CMake or P4.6 tuning/defaults work.

P4.5 completed 2026-10-23 — implemented_unverified.
- New adaptive_probe.hpp: stopped synthetic 3x3 scalar/GI/reflection source fixture
  compares matched adaptive/nonadaptive streams, noisy/step input with unchanged
  accumulator generation, one-neighbor fallback, valid-black GI, HDR/outlier finite
  storage, hit/miss/confidence, known red/blue luminance limit and maxFrames=1.
  Optional v5 path and deliberately missing moment family cover explicit unsupported
  selection/v4 recording. Missing-module failure is not a fake RG16F format test.
- Queued fixture adds trailing adaptive/moments flags while preserving v3 default
  baseline. V4/v5 narrow-geometry one-neighbor inputs retain analytic old weights.
  Two/three submitted slots, scalar/log-HDR moment pairs, distinct signal/view/layer
  images/statistics, retained old owners, abandoned/gapped restart are source-covered.
  Inputs uploaded before batches; no actual overlap claim or abandoned output read.
- New adaptive_sequence_reference.py integrates half storage, response and moments
  in temporal streams, covering every required signal/support/freeze/dropped/
  unavailable/disabled/maxFrames1 scenario. Immutable reference states are not GPU
  lifetime proof. Phase 4 source checker now runs the sequence suite and audits
  new/expanded C++ source GPL/includes/delimiters.
- Executed/passed: Phase 4 reference/source suite, Phase 3 and Phase 2 checks,
  git diff --check; SHA256 comparison of entry production .cpp/.hpp/.glsl/.metal
  inventory shows no production source additions/edits in P4.5. No tuning/default,
  shader, API, CMake, synchronization, ray-count, renderer/backend/bake/FSR changes.
- Controlled reference-only experiment: 32 initial .25 frames then .75, maxFrames32,
  freeze off, first frame of a three-sample error<=.01 window =123/1/1 new samples
  for v3/v4/v5. Balanced deterministic checker noise tail RMSE ~.005524 for all.
  These are half-quantized model numbers, not native pixel/quality/timing evidence;
  flat noiseless steps favor clipping and do not establish general performance.
- Phase file contains complete per-case source/native ledger, exact stopped host
  entries and a reproducible AO-outside-footprint/reflected-only/GI/pan/edge/HDR/
  chromatic/freeze visual protocol. Equal settings/work, capability/format status,
  common ROI/reference/tolerance, scene-frame convergence and stable noise/bias/
  ghosting measurements are required. Global generation/frameCount alone is not
  pixel correctness. Current adaptive numeric debug views still belong to P4.6.
- All C++/shader compilation, CPU codec fixtures, native scalar/HDR/moment format/
  invalid/gap/queue/half/freeze checks, actual lighting/visual acceptance and cost
  benchmarks remain not_run on Vulkan/Metal. Actual unavailable-format observation
  is distinct from forced missing-module fallback and remains pending. False or
  unsupported paths never count as a pass. No support/quality/speed claim.
- Next exact step: P4.6 only: cost/source review, opt-in adaptive diagnostic views,
  defaults/limits documentation and P5.1 handoff. Keep release/default choices off
  and all native checks not_run until explicitly authorized.

P4.6 started 2026-10-23 (in_progress): independently optional v4/v5 metric-capture families/display, appended diagnostic views and exact-key renderer integration, numeric native/reference sources, Phase 4 final source/cost/default/limitations handoff to P5.1. Preserve prior dirty files. Root/fallback architecture reviewed; skills.md absent. No compilation/native/benchmarks/CMake/Phase 5 implementation.

P4.6 completed 2026-10-23 — implemented_unverified; Phase 4 source complete.
- TemporalDiagnosticView appends CurrentWeight/Reactivity/Variance/EffectiveCount
  IDs 6–9. New trailing diagnoseAdaptive=false is exclusive with diagnoseAcceptance;
  both true reject before recording. Separate canonical v4_metrics/v5_metrics
  layouts add storage 21/Metal19 with unchanged 368-byte uniform; old/core/custom
  kernels receive their existing bindings only. Independent optional init/failure/
  cleanup/status, no ordinary retries, no core disable on missing metric shader.
- New metrics display uses independent pipeline/layout and existing 16-byte display
  ABI. Accumulator/renderer/reflection expose availability/status; renderer capability
  includes metadata availability. Exact latched signal/layer requests capture;
  current sequence/recording/token-owner matching prevents stale data display.
  Missing capture is amber, zero-token/nonfinite selected value black. No implicit
  replay/adaptive enable or capture-induced mode/signature/reset change.
- Encoded RGBA = actual current alpha, actual reactivity, variance/(1+variance),
  post-response trust/maxFrames. V4 variance is linear spatial; v5 bounded moment
  domain spatial/max temporal estimate. Frozen/rejected/unsupported statistics
  report zero (not evaluated). Uncovered is zero, cold alpha one/count one, frozen
  alpha zero/saturated count. View shader selects one grayscale channel, alpha one.
- Optional signal-sized RGBA16F History.metrics and receiver-sized display retain
  existing frame-slot invocation/metadata owners before dispatch. Off/unselected
  paths allocate no new capture/display, though retired/pool/external images may
  persist. Existing capture/display CPU/GPU profiler scopes include the new work;
  no readback, counter, wait, immediate submission or synchronization added to
  ordinary frames. Resource leases distinguish new layouts and uniform sizes.
- Expanded adaptive native source optional fifth metrics bool: ordinary adaptive
  control checks color/guide/moment parity within FP16 tolerance, four display
  channels/missing capture, finite bounds/count, cold/frozen/thaw, mutually exclusive
  request rejection and capture toggles without chronology reset. C++ unrun.
- New diagnostic_reference.py + existing Phase 4/3/2 suites executed/passed:
  enum values, source bindings/guards, paired metric blocks, finite channel encoding,
  count/variance semantics, analytical costs, GPL/includes/delimiters. Saved P4.5
  source comparison with TEMPORAL_METRICS removed is token-identical. git diff
  --check passed. These do not compile C++ or GLSL/Metal or prove native parity.
- Analytical cost reviewed: v4 scalar/HDR history 26/32*S, v5 moments +4*S per key,
  optional selected capture +8*S and display +8*R, metadata shared per domain
  (32+D)*R. A full-1080p three-signal v5/D32 one-generation example is 286.1568 MB,
  or 319.3344 MB with one capture/display. Not resident/peak measurements; count
  all retained/in-flight/external/pool/driver resources. No extra rays or measured
  speedup. Up to nine current neighbors plus guide reads and v5 moment taps remain.
- Final Phase 4 review covers paired shaders, settings/codec/signature source parity,
  disabled paths, literal freeze/trust/confidence, optional capability/custom/kernel
  fallback and retained ownership. Full audit/cost/default/limitations/native matrix
  and exact host calls are in phase_4_adaptive_accumulation.md P4.6; API and relevant
  probe docs updated. Phase 5 source has not started.
- All C++/shader compilation, actual combined descriptor/format support, numeric
  capture and output parity, native queue/half/freeze, lighting/visual/noise/bias,
  timestamp availability and cost/high-water measurements remain not_run on both
  Vulkan/Metal. No release or quality/performance/support pass is claimed.
- Replay/adaptive remain false by default; weights .03125/.25 and sigma3 unchanged,
  diagnostic Off. Existing limits remain: thin/grazing support, same-luminance hue
  lag, FP16/saturation, trust-dependent freeze, mapped-normal/deformation/reflected
  transforms, transparent/reflection motion and stationary lighting after freeze.
  No CMake, GPU/backend/vendor, bake, FSR, ray count or Phase 5 tuning changes.
- Next exact step: P5.1 only, current producer/consumer/invalidation/ownership audit;
  do not automatically execute P5.2–P5.6 or request/run native work on this handoff.
