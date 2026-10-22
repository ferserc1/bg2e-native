# Temporal accumulation implementation progress

## Resume pointer

- Plan: [summary.md](summary.md)
- Current phase: Phase 1 source implementation complete (implemented_unverified); Phase 0 remains implemented_unverified; Phase 2 not_started
- Next step: **P2.1** in [phase_2_object_reprojection.md](phase_2_object_reprojection.md)
- State: `implemented_unverified`
- Last completed plan step: P1.5 (implemented_unverified)
- Next exact action: mark P2.1 in_progress on the next request; lock camera/framebuffer conventions and capture immutable current/previous jittered/unjittered camera recording state with the preceding submitted scene. Do not implement P2.2 motion attachments in that step.

## Authorization and workspace

- Authorized now: P1.5 source implementation requested 2026-10-22. Added optional renderer/accumulator diagnostics, paired diagnostic kernels and native/internal probe sources; updated API/architecture/plan docs. No compilation/shader compilation/runtime authorization; no CMake changes.
- Compilation/shader compilation: not authorized by the plan-writing request; do not run.
- Native runtime validation/benchmarks: not run.
- CMake changes: prohibited unless explicitly requested.
- skills.md: repository search repeated on 2026-10-22; still absent. Only root AGENTS.md was found; no nested instructions were found. Fallback architecture documents read: doc/architecture/MainLoop_render_draw_gpu.md, draw_advanced_lighting.md, and draw_offscreen_rendering.md. Current source is authoritative where those documents describe older behavior.
- Fresh implementation-start `git status --short`: empty (clean checkout). No unrelated dirty paths are present. The earlier planning-time dirty paths are historical only.

## Fixed decisions

- Object-motion trails are accepted in Phase 0; no global reset for transform-only edits, including fallback paths.
- Keep full transform-sensitive scene fingerprints and exact cached/reference serialization parity.
- Freeze preference remains unchanged; temporarily roll during motion and for 3 * maxFrames subsequent rendered scene frames.
- No changes to maxFrames for thawing; no invalidating setters during interaction.
- Stable CPU key is nodeId/submesh; GPU token is a nonzero monotonic uint32.
- Separate metadata raster pass; preserve the six-color G-buffer and depth index. Experimental renderer replay requires temporalSurfaceEnabled=true (default false until native validation/default tuning); availability/status are separate from the request. P1.4 consumers remain pending.
- Phase 0 camera reset policy remains unchanged. Phase 2 enables continuous-camera reprojection on supported built-in paths.
- Global material/light/environment/topology/membership reset policy remains unless explicitly scoped otherwise in a later approved change.
- Deferred: deformation history, virtual reflection motion, adaptive ray counts, external denoiser integration.

## Phase ledger

| Phase | Implementation | Source checks | Vulkan runtime | Metal runtime |
|---|---|---|---|---|
| 0 | implemented_unverified (P0.1–P0.5 source work finished) | P0.5 final review passed | not_run | not_run |
| 1 | implemented_unverified (P1.1–P1.5 source complete) | P1.5 static ABI/source review passed | not_run | not_run |
| 2 | not_started | not_run | not_run | not_run |
| 3 | not_started | not_run | not_run | not_run |
| 4 | not_started | not_run | not_run | not_run |
| 5 | not_started | not_run | not_run | not_run |

## Current step work record

- Step: P1.5 (implemented_unverified)
- Actual files: renderer/accumulator headers and implementations, settings reflection, shared temporal helpers, eight diagnostic accumulation variants, paired display kernel, temporal and temporal_surface probe sources, API/architecture/plan docs.
- Behavior: optional exact GPU acceptance/token diagnostics, frame-latched view/signal/layer controls, retained owners and source-only raster/chronology probes. Replay remains default off and base v1/v2 math unchanged.
- Workspace: all prior Phase 0/P1.1–P1.4 changes preserved. No unrelated paths, CMake edits or build artifacts introduced. Pre-step snapshots in /tmp/bg2e-p15-before support exact source comparisons.
- Source checks: paired shader profile/math parity, CPU/backend bindings, source delimiters/includes/licenses, reset/fingerprint preservation, synchronized lifetime and no ordinary readback/wait/submission reviewed. git diff --check passed.
- Known risks: all new C++/shaders and native probe assertions remain uncompiled/unexecuted; no actual format/pixel/visual/performance acceptance. See the P1.5 record and temporal_surface README for scenarios and manual validation still pending.

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

Application integration review:

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
