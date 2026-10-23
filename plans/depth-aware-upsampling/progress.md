# Implementation progress

status: implementation_complete
last_completed_step: 2
current_step: none
verification_status: not_run_user_owned

- [x] Step 1 — Implement the private compute stage and paired shaders
- [x] Step 2 — Integrate resources, lifecycle, and GI composition

## Next action

Read `summary.md` and `step-2-integration.md`, set `current_step: none`, and integrate compatibility checks, optional initialization warnings, per-slot output allocation, acquired-slot retirement, drained lifecycle release, and reconstruction after GI signal resolution in both opaque and shared transparent paths.

## Resume instructions

Update this file in the same directory as the step files. After a complete step, check its box, advance `last_completed_step`, and record files changed, decisions/deviations, and the next concrete action. For an interrupted step, leave its checkbox unchecked and describe the remaining work. Keep compilation, tests, and validation user-owned; do not use them as progress milestones.

## Planning revision

The original two-step outline was replaced after examining GI receiver mapping, denoise dimensions, deferred alpha/IBL behavior, nearest sampling, transparent receiver reuse, acquired-slot resource ownership, and paired shader loading. This revision changes the planned step boundaries; it does not record implementation progress.

## Step 1 implementation

Files changed:

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `lib/src/bg2e/draw/RendererDeferred.cpp`
- `shaders/src/draw/gi_upsampling/joint_bilateral.comp.glsl`
- `shaders/src/draw/gi_upsampling/joint_bilateral.comp.metal`
- `plans/depth-aware-upsampling/progress.md`

Implemented the opaque private `GIUpsamplingState` and its shared owner, plus complete `initializeGIUpsampling`, `retireGIUpsamplingSlot`, `recordGIUpsampling`, and `cleanupGIUpsampling` helpers. Initialization publishes only a complete state and requires RGBA16F Sampled | Storage support. Recording accepts retained image owners, camera matrices, output, acquired slot, serial, and opaque/shared-transparent layer identity. Unavailable state, absent signal/destination, and equal extents return the selected signal unchanged. Lifecycle misuse and invalid image contracts are explicit errors.

The state retains fresh immutable uniform buffers/resource sets and all four referenced image owners for each invocation until acquired-slot retirement. This uses the plan's explicitly allowed retained-object alternative to TemporaryResourcePool; there is no pool to collect. Retirement and teardown require the caller's existing acquired/drained boundary and perform no CPU waits. Step 2 must call these helpers at those boundaries.

Both shaders implement the fixed bindings, 176-byte uniform ABI (with C++ size/offset assertions), integer guide fetches using the producer's exact receiver association, matrix-based view depth, finite/coverage rejection, the 4x4 joint bilateral kernel with documented private defaults, geometrically gated nearest fallback, finite RGBA16F output, and binary alpha. GLSL declares 16x16 workgroups; the renderer dispatches ceil(F/16) for both backends, following the existing compute convention. Metal omits the unused sampler argument because integer reads do not use it; its pipeline layout retains the specified sampler binding. No ABI or kernel deviation is intended.

Rendering remains disconnected: no helper is called from the active graph, no output image is allocated, and no public configuration or CMake file is changed. Step 1 source work is complete. Compilation, tests, diagnostics, benchmarks, and backend/visual validation were not run and remain user-owned (`not_run_user_owned`).

## Step 2 implementation

Files changed in this step:

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `lib/src/bg2e/draw/RendererDeferred.cpp`
- `plans/depth-aware-upsampling/progress.md`

The stage initializes after layer replacement selection and GI availability are resolved. `giUpsamplingCompatible()` admits only exact built-in GBuffer and RTGlobalIllumination dynamic types, preserving custom/derived producers and receivers. Optional initialization failures report through `warnInitialization` and retain the original scaled GI path. The existing RGBA16F capability requirement is unchanged.

`Targets::giUpsampled` is allocated at internal extent with Sampled | Storage usage only when the compatible stage exists and raw GI dimensions differ. `buildTargets()` replaces this resource together with the other slot targets; the existing raw GI quality-size reconfiguration detection is unchanged. The extent-independent compute pipeline survives resize. `update()` retires only the acquired slot's prior invocations, while scene attachment and resize clear all invocation slots after their existing device drains. Cleanup releases the stage immediately after its existing drain and before effects, targets, and device references are released. No extra waits or history-generation changes were introduced.

GI reconstruction now follows the single existing temporal/denoise resolution in `recordEffects(layer)`, only for enabled GI mode. The selected image must retain the raw producer dimensions and have a matching retained owner. The resolver optionally forwards the denoise owner or temporal pass-through owner; raw GI pass-through uses its target owner. Ownerless injected replacement images conservatively preserve the original path, avoiding a fabricated ownership guarantee. AO/reflection signal selection and temporal keys remain unchanged.

Opaque composition consumes the reconstructed destination before the shared transparent invocation overwrites it. Each dispatch retains separate immutable uniforms/resource sets and image owners through slot retirement. Shared transparent reconstruction runs once immediately after receiver generation, before individual draws overwrite normals, and uses preserved opaque depth exactly like the producer. This remains an approximation of transparent geometry; temporal metadata depth is deliberately not substituted. Existing explicit transitions order prior lighting reads, compute writes, and subsequent lighting reads. Every dispatched destination is written; disabled GI/other indirect-lighting modes continue binding their original cleared signal, never stale reconstructed contents.

No shader correction, public setting, lighting contract, CMake change, or modification outside the allowed implementation files was needed. No compilation, tests, diagnostic tooling, benchmarks, or backend/visual verification were run (`not_run_user_owned`).
