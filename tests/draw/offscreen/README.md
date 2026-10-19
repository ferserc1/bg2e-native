# Draw offscreen lifecycle probe

`main.cpp` is a manual fixture for M05-02. Compile/link it against the current
bg2e shared library using the existing dependencies and platform runtime setup;
no CMake target is added. The agent has not compiled or run it.

Run with `vulkan` or `metal` (Metal requires macOS). With no argument it uses the
EngineConfig platform default. Add a second `deferred` argument to initialize and
record one real RendererDeferred frame using its empty default scene. That optional
case requires the deployed draw shader tree and assets/runtime lookup setup, just
like the existing draw examples. The basic probe uses GPU clears and no shaders.

The fixture checks:

- Actual offscreen Instance mode, null window accessor, generic surface identity,
  one completed-reuse slot and an omitted surface depth target.
- Two red/green GPU clears with tight 7x3 raw RGBA8 readback in completed callbacks;
  readback sends do not add scene frames. The first delta is zero; the next is the
  configured 0.125 seconds. Recorded, completed and surface frame indices agree.
- A false initial continuation condition submits no frame.
- Cleanup exactly once after init, recording, completion and cleanup failures,
  plus destruction after init without run. A recording failure combined with a
  cleanup failure preserves the original recording exception and reports no
  completed frame. Delegate cleanup sees a still-valid Device.
- A second Engine on the same Backend rejects the already-active Instance without
  destroying the first context.
- Optional deferred initialization/recording with no window/UI, default no-effect
  raster behavior, implicit generic slot derivation and no editor overlays.

The empty-scene deferred case is a lifecycle/shader integration check, not evidence
of PBR image correctness. The known-value clears do not verify row orientation
because every texel has the same color. No file/image encoding is performed.

Additional project lead checks:

1. On Linux Vulkan run with no display server/SDL video initialization. On macOS
   run Metal and, if available, Vulkan/MoltenVK. Repeat no-RT raster and SDK/FSR
   unavailable cases with the real deferred scene. GPU/driver failures must throw,
   retain submitted owners through draining and never notify a false completion.
2. Run existing windowed examples 10–12 and production offscreen examples 15–16.
   Verify window/UI acquisition and settings replacement still work. Passing a
   headless Engine to window RenderLoop/UserInterface must fail before UI creation.
3. With a scene-bearing DefaultOffscreenApplicationDelegate subclass, check a
   camera, procedural mesh, light and environment using the same assets and draw
   pipeline as example 10. Verify fixed-step versus frozen transforms and reduced
   internal/output resolution. Setters/renderer updates commit only at stopped
   prepareFrame boundaries. Set config.frames for N samples with the default
   accumulating delegate; do not override continuation to request more.
4. Verify zero extent, negative/NaN timestep, invalid/integer color formats, invalid
   requested depth and unavailable backend rejection. Reinitialization/reentrant
   calls fail; a fresh owner works after each failure. Surface depth can be omitted;
   RendererDeferred still owns its own configured internal depth attachments.
5. Throw from prepareFrame/update/renderer initialization, leave an active command
   scope deliberately, or stop in prepareFrame. Confirm no partial command is
   submitted, acquired surfaces are retired during teardown, resources outlive
   any admitted work, and stopping creates no extra frame.
6. Verify renderer replacement clears overlays and uses the ordinary
   rendererGeneration/detach/attach callbacks. Direct custom delegates own their
   own output policy and should not install editor UI/gizmo passes.

M05-03 adds exact-N/settings/final-once behavior to the default scene delegate.
The optional deferred branch additionally compares two HDR clears (0.25/0.75)
against one 0.5 clear through the actual final pipeline, with 50% internal scale,
exact completed counts, no early result, and one final conversion. Both jobs
must produce identical tight final bytes. See ../frame_accumulation/README.md
for CPU reference and configuration/mutation scenarios. No checks were executed
by the agent. Encoded scene output remains M05-04/05.
