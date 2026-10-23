# Step 2 — Integrate resources, lifecycle, and GI composition

## Outcome

Connect the complete private stage from Step 1 to `RendererDeferred` without changing GI tracing, temporal history resolution, the denoiser, or deferred lighting contracts. Only `RendererDeferred.hpp` and `.cpp` need additional changes in this step, apart from corrections to the paired shaders if necessary for the specified integration.

## Renderer lifecycle and compatibility

1. Initialize the private state after GI availability is known, using the renderer's device, backend, `_shaderRoot`, and acquired frame-slot count. Require RGBA16F Sampled | Storage support. Follow the existing recoverable initialization-warning pattern: if the optional stage cannot initialize, retain the existing GI path and report the cause through `warnInitialization`. Do not silently substitute a different reconstruction algorithm or disable otherwise available GI.
2. Restrict automatic activation to the exact built-in `pipeline::GBuffer` and `pipeline::RTGlobalIllumination` implementations and their existing camera-profile receiver mapping. Derived/custom producers may use a different mapping despite an identical image format. Preserve their original path without changing their API. Inspect the selected receiver after layer replacement has been resolved, not before defaults/custom layers have been selected.
3. Add `Targets::giUpsampled` as a shared RGBA16F GPU image with Sampled | Storage usage. Allocate only when GI and the private stage are compatible/available and the producer extent differs from `internalExtent`. Use internal extent, never output extent, and a descriptive debug name. ColorAttachment usage is unnecessary because the new pass writes every pixel through compute.
4. `buildTargets()` must replace this image with all other slot images. Existing quality changes are detected by `needsReconfiguration()` comparing the raw GI extent; retain that mechanism. `resize()` already drains the device, resets effects/histories and rebuilds targets, including transitions to/from Ultra or a one-pixel extent where source and destination become equal. Rebuild allocation from current actual extents. Shader pipelines are extent-independent.
5. At `update()` on a newly acquired retired frame slot, release its previous upsampling invocation leases and collect completed pool resources. Do not clear other in-flight slots, clear leases in `recordEffects`, or mutate resources from a previous acquired serial. Existing stopped-producer lifecycle rules apply.
6. On scene attachment/resize/cleanup, clear retained per-slot pass state only at the existing drained boundaries. Release pass resources in cleanup alongside renderer targets and before the engine/device reference is discarded. Make partial initialization cleanup safe. The pass has no temporal history; it does not independently increment `historyGeneration`.

## Record graph integration

In `recordEffects(layer)`, retain the existing GI mode and enablement logic:

1. Trace into `slot.gi` or clear it using the current logic.
2. If GI mode is selected and GI is enabled, run the existing `resolveSignal(DiffuseRadiance, ...)` exactly once. This already selects temporal/denoise return images and handles effects being disabled/unavailable. Keep temporal keys, layer IDs, and raw GI dimensions unchanged.
3. Only after that selection, reconstruct the selected signal when all activation conditions hold, the stage is available, and a destination exists. Require the selected signal size to match the raw GI size; injected effects that change the source mapping/extent must retain the original path. Equal-sized signals bypass the new pass without a copy or allocation.
4. Use `slot.gbuffer[1]`, the current invocation's `surfaceDepth`, `inverse(slot.jitteredProjection * slot.camera.view)`, and `slot.camera.view`. The opaque invocation uses G-buffer depth; the shared transparent invocation uses preserved opaque depth, matching the existing producer contract and its limitations.
5. Bind the reconstructed image under the existing `globalIllumination` semantic; otherwise bind the currently selected original signal. No deferred contract/version/sampler or lighting shader change is needed because `ExtentDomain::Scaled` already accepts internal extent.

Opaque reconstruction and composition must finish recording before reuse of the same slot destination by the transparent reconstruction. The command stream and explicit layout transitions provide ordering; CPU synchronization is unnecessary. Preserve invocation bindings separately even when both dispatches write the same image.

Reconstruct the shared transparent signal during `recordEffects(TransparentTemporalLayer)`, immediately after the transparent receiver batch has populated the G-buffer. Do not defer this operation into the later per-item composition loop: those draws overwrite normals and no longer match the GI receiver. Do not recompute the pass per transparent item or crop it to per-item scissors. The complete destination must be written each invocation.

## Inactive, unavailable, and fallback behavior

- Existing inactive-mode clears retain alpha zero. Skip reconstruction when GI is disabled or another indirect-lighting mode is active; never bind stale upsampled output in those cases.
- If an enabled GI invocation produces invalid/background samples, reconstruction still writes every destination, rejecting invalid samples according to Step 1.
- With no compatible low-resolution support, output alpha zero and allow the existing diffuse IBL fallback. This does not recover GI for unsampled thin geometry.
- If stage initialization is unavailable or compatibility checks reject a custom path, retain its pre-existing scaled GI consumption and initialization warning where applicable. Do not label that path as successfully reconstructed.
- Keep the existing diagnostic views and split `recordLinear` / `recordFinal` behavior; add no new diagnostics, public toggles, application UI, or verification work.

## Handoff

Update `plans/depth-aware-upsampling/progress.md`: complete Step 2, set `last_completed_step: 2`, `current_step: none`, `status: implementation_complete`, and record actual changed files plus resolved lifecycle/compatibility decisions. Compilation and visual/backend verification remain `not_run_user_owned`. Mark the implementation complete only after both source steps are finished, not during this planning revision.
