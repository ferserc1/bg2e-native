# Phase 0: preserve history on object motion and temporarily reopen freeze

Dependency: none. Entry step: P0.1. This is a deliberately approximate, independently deliverable change.

## Exact behavior

A transform-only change must not call RendererDeferred::resetHistories, increment its history generation, clear TemporalAccumulator histories, or reset the upscaler. Keep the existing per-pixel shader rejection and camera policy unchanged. Do not add metadata targets or motion vectors in this phase. Keep all other existing invalidation causes.

The full scene fingerprint must still observe transform edits, including between explicit fingerprint queries and update. Frozen offline jobs must still detect scene mutation.

For every active effect/view/layer history, a detected object transform change opens rolling updates on that rendered scene frame. Also allow exactly `3 * maxFrames` subsequent consecutive scene frames without motion, then restore the configured freeze preference. Motion during the window renews it. UI/presentation-only activity does not consume the window. Do not extend maxFrames, alter its weighting, change the authored freeze preference, or call invalidating setters to thaw.

## P0.1 — audit and record the integration boundary

1. Read the source map in summary.md, the three temporal/upscaling probe READMEs, and DefaultOffscreenApplicationDelegate::sceneFingerprint consumers.
2. Search all call sites of TemporalSignatureState::equals/flattened, fullTemporalSignature, sceneFingerprint, resetHistories, invalidateHistories, beginFrame, and freezeAtLimit.
3. Identify full snapshot equality versus the reset decision versus frame fingerprint consumption. Record actual symbols and callers in progress.md; preserve full snapshot semantics.
4. Inspect RenderSettingsPreferences and DefaultRenderLoopDelegate for generic interaction callbacks that reset histories. Only remove a reset if source evidence proves it is invoked for transform-only motion; do not suppress resets for arbitrary edits or failed scene replacement.
5. Record all intended files and current unrelated dirty paths. If the architecture reference is missing, record the fallback documents read.

Done when the remaining steps have verified source locations and no source edits have yet been made without identifying the exact policy boundary.

## P0.2 — split reset policy from full equality

1. Keep `equals()` and `flattened()` transform-sensitive for exact snapshot/cache equality and full fingerprints.
2. Add explicit named operations to compare global history compatibility and detect transform changes. The compatibility comparison includes every current component except instance world transforms. Preserve ordered membership, object IDs, mesh/material identity, RT inclusion, lights, environment, camera configuration and settings comparisons.
3. Detect transform changes by matching stable node/submesh keys; do not interpret extraction IDs as persistent correspondence. Missing/duplicate identity follows the existing validation rules. Membership changes already force a global reset and need no synthetic motion event.
4. Renderer update must use compatibility for reset decisions and motion detection for thawing, then publish the accepted snapshot. A fingerprint query may update derived caches but must never overwrite the accepted baseline used to detect motion.
5. Preserve cached/reference validation in all modes: add a reference policy serialization with world transforms omitted, separate from `fullTemporalSignature()` which keeps its exact format. Use that reduced payload to validate reset-decision parity. Full byte parity continues to validate full snapshots. Add distinct storage/baselines so toggling reuse/validation does not compare reduced bytes against full bytes.
6. Keep countResetCauses consistent: transform-only edits count as a motion observation, not `temporal.signatureResets` or global `objectMotion` resets. Do not remove transform comparison from cache snapshot identity; that would retain stale matrices.

Done when full fingerprints still change on motion, while cached, reference-only, and validation modes all choose no global reset on the same transform-only edit.

## P0.3 — implement the update-window state machine

1. Add optional trailing `bool sceneMotion = false` input to `beginFrame` (or an equivalently defaulted explicit context). Propagate renderer motion detection once per scene frame, before effect recording. Document overrides/custom accumulators and adjust repository subclasses.
2. Store the motion flag in the prepared slot. Store `uint32_t rollingFramesRemaining` in each History, not a counter decremented per effect invocation globally. Histories already retain scene sequence, submission ownership and previous/next snapshots.
3. In record, first obtain a valid consecutive predecessor using the existing recording submission checks. Initialize next remaining as follows: motion now => `3 * maxFrames`; otherwise, a valid predecessor with remaining > 0 => remaining - 1; otherwise => 0. A fresh/reset history starts at zero unless motion now. Counter values are at most 3072 with the current maxFrames bound.
4. Effective freeze for THIS invocation is `settings.freezeAtLimit && !motionNow && !(validPrevious && previous.remaining > 0)`. Thus the frame reducing remaining from 1 to 0 still rolls, and the next static frame freezes. Do not use the next remaining counter to decide this frame's freeze.
5. Pass only effective freeze to the existing uniform freeze flag in both GLSL and Metal. Preserve the configured flag in settings/reflection/signatures. Counts remain bounded by maxFrames; rolling at saturation uses current weight 1/maxFrames.
6. Invalidated, absent, expired, skipped or abandoned histories cannot retain stale window state. Publish countdown only through the next recorded History; an update without record must not consume the countdown.
7. An optional GPU-independent pure helper may calculate this transition for source/native probes. Keep the arithmetic in one place. No new live setting is needed; the factor 3 is an explicitly named constant, not an undocumented magic number.

Example with maxFrames=2: a motion frame stores 6 and rolls; six subsequent static records store 5,4,3,2,1,0 and roll; the seventh static record can freeze if its per-pixel sample count is saturated. A new motion record restarts at 6. With maxFrames=1, rolling immediately replaces the previous value; the tail still lasts three static frames.

## P0.4 — add focused regression scenarios

Update existing temporal_invalidation and temporal probes, preserving unrelated assertions. Add checks for:

- Translation, rotation, scale and tiny transform changes: unchanged global history generation, changed full fingerprint.
- Motion observed after an out-of-band fingerprint call; repeated cached/reference/validation mode switches.
- Material, texture replacement, membership, mesh, lights, projection, explicit reset: their existing resets still happen.
- Freeze false: bounded rolling behavior unchanged. Freeze true: exact countdown, repeated motion, multiple independent keys, maxFrames=1 and 1024, resumed scene after presentation gaps.
- Abandoned/unsubmitted recording and update-without-record: no valid countdown/history inherited from discarded work.
- Existing freeze probe still freezes an unchanged saturated scene with no motion; retained earlier outputs never mutate.
- Offscreen frozen-scene validation still rejects a transform edit even though interactive history generation stays unchanged.

Document FSR trails and all indirect-light trails as accepted Phase 0 behavior. Update only relevant existing text; do not overwrite pre-existing user edits in READMEs.

## P0.5 — verify and hand off

Static checks: inspect diff, confirm maxFrames setters are not used for motion, full fingerprints retain worldMatrix, no camera rejection changes, no ordinary per-frame synchronization/readback, paired uniform behavior unchanged, and no CMake edits. Review exact countdown off-by-one example above.

If explicitly authorized to build/run: execute existing probes on available Vulkan/Metal hosts and visually move one cube continuously before a converged static wall. The wall must not globally lose convergence. With freeze enabled, move the cube, stop, inspect rolling for 3*maxFrames additional scene frames, then freezing. Inspect AO/GI/reflected content on static receivers; artifacts are allowed and must be reported honestly.

Record `implemented_unverified` if only source checks were possible. Next pointer: P1.1. Do not claim that Phase 0 implements object reprojection, exact local invalidation, or guaranteed removal of trails.

## Cost / hardware

No new GPU images, no new shader passes, no new graphics features. Small CPU comparison overhead and a uint32 countdown per retained history. Existing full/reference diagnostic modes may serialize additional bytes; normal mode should reuse snapshot components. Continuing to update while thawed is intentional even when the image was formerly frozen; do not promise reduced GPU time.
