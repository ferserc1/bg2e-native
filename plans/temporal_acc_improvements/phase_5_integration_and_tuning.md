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
