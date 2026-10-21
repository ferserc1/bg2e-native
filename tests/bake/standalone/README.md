# Standalone bake context manual checks

`probe.hpp` supplies `runStandaloneBakeProbe` for an initialized windowless draw
Engine, loaded camera-less root and two enabled targets with valid UV2. Supply
same-device explicit EnvironmentResources or null to exercise public authored/
default environment generation, the deployed shader root, and RTAO/RTGI separately.
The caller owns backend/Engine initialization and cleanup. Keep all returned or
externally retained GPU owners, roots and targets inside the Engine lifetime.
Stop other scene/resource producers for the entire probe. No CMake target is added;
the probe has not been compiled or executed by the implementing agent.

The probe checks Created/Initialized/SceneReady/Cleaned transitions, one live target,
external record rejection, incomplete readback, exact completed counts and frozen
settings, over-completion rejection, reset, cancellation during recording, injected
partial-record failure/recovery, stale-job update/reset rejection, a second target,
unannounced scene edit invalidation and independently retained completed images.
The injected accumulator is a concrete failure/cancellation seam, not a new engine
execution abstraction. Its callback is cleared before the captured baker is freed.

Additional lead-owned validation:

- Build the library with the normal authorized configuration and paired bake/draw
  shaders; call the probe on available Vulkan and Metal backends. Record actual
  device/backend/native diagnostics. No window, UI, camera or RendererDeferred
  should be created by the context. A windowed Engine must reject initialize.
- On missing required RT/array/format/kernel capability, verify createBaker throws
  and publishes no successful bake. Check both AO and GI. GI rejects an empty
  traceable scene; there is no silent environment-only policy.
- Repeat valid scene preparation with the same path after changing HDR file data,
  procedural sky RGB, explicit/null authored Environment selection, and explicit
  EnvironmentResources. Failed loading leaves Initialized and old jobs invalid.
  The context delegates image support/color conversion to current public tools.
- Between synchronous updates, edit geometry/UV2, submesh placement, light/material,
  native image bindings or BLAS generation at a stopped boundary. Old work must
  reject update/result; prepareScene and a new baker are required. Settings edits
  take effect only at explicit reset. In-place uploads not observable through
  public revisions require explicit invalidateSources before mutation.
- Compare first sample index zero after reset and exact requested valid counts,
  top-row-v=0 RGB8/RGB32F, chart isolation/dilation and nonnegative unclamped HDR
  against integrated bake primitives; saturation/invalid raw samples must fail.
- Retain an old sample/map while baking another target; verify no alias overwrite.
  Inspect existing tracing counters for immutable table/finished-TLAS reuse across
  samples. No material ownership/default/capacity optimization should be reverted.
- Inject native submit/wait failures where supported; failed work must never advance
  counts. A failed retirement blocks further work until successful explicit cleanup.
  Run native lifetime diagnostics during context/baker destruction and Engine teardown.

Standalone batch assembly/export/CLI is not supplied by this fixture or M08-01.
Timing scopes/counters remain available; no GPU timing waits are added for profiling.
