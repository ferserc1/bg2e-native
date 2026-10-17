# Advanced integration manual validation

No target/CMake change is added. Include probe.hpp temporarily in an existing draw
test application. Project lead compiles engine and all native shaders and runs:

- `bg2e::draw::tests::runAdvancedSettingsProbe()` without a Device: live requested
  settings strict JSON round trip while unavailable; bounds and read-only caps.
- `bg2e::draw::tests::runSMAAProbe(engine, shaderRoot)` with a stopped initialized
  Engine. False means unavailable native profile, not a passed numerical test.
  Covers disabled borrowed source, constant color, odd 33x17 extent/borders,
  stair-step smoothing, finite opaque output and retained owners across slot reuse.

Agent has not compiled or executed these fixtures. Keep Device and borrowed
sources alive through submission completion. Restore the temporary harness after
validation. Existing clone/material/picking fixtures remain applicable.

Run BOTH `draw_deferred_renderer` and `draw_manipulation` on Vulkan and Metal:

1. Open shared Graphics settings. Confirm actual targets/values/capability status;
   export, change values, strict import and preference restore; replace renderer
   and verify stale pending handles are rejected and current targets rebound.
2. Compare IBL, enabled AO, enabled GI; each quality and sampling control;
   reflections independently on/off, roughness limits and light affectsReflections.
   Select each diagnostic view (11..14 signal meanings are documented).
3. Test shadows on/off, no shadow lights, cutout rear hits, >8 distinct cutout
   image/sampler pairs. Legacy shadows must show sceneSupported=false and neutral
   visibility; advanced effects must keep the complete material table. Shrink to
   <=8 and verify recovery. Deliberate advanced table capacity errors remain explicit.
4. For AO/GI and reflections test temporal off/on, 1/32/1024 frame limits,
   freeze/rolling, spatial denoise off/on/radius0/8 and extreme finite thresholds.
   Pause light/camera animation to inspect convergence; moving lights/objects reset
   histories conservatively. UI-only presentations must not advance scene work.
5. Edit clone/original materials, gizmos, light/environment properties, hide/remove
   nodes, apply multi-material patches (including partial failure), choose best
   viewpoint and replace geometry. Confirm SDK/engine history resets and redraw;
   preserve clone independence, weak selection, resources and picking behavior.
6. Resize/minimize/restore, change internal resolution and effect quality, pause/
   resume, enable FSR on a supported Vulkan SDK build and inspect motion/jitter;
   SDK missing/Metal/unsupported FSR must retain ordinary resampling.
7. Compare SMAA disabled/enabled with resampling and FSR, including no RT/query-only
   devices. UI/gizmos stay sharp and unfiltered. Check 1x1, odd extents, sRGB/UNORM
   output transfer, long edges/borders and threshold/search extremes. Test missing
   native shaders/formats and compatible injected processors/metadata overrides.
8. Test no RT and partial query/pipeline capabilities independently. Requested
   settings must remain editable and round-trip even when effects are unavailable;
   no unsupported AS/dispatch or aggregate full-RT gate. Validate supported standalone
   temporal/denoise/SMAA paths without RT. Compare hash/injected/shared noise modes.
9. Use asynchronous acquired slots, held outputs and native validation diagnostics;
   no premature resource destruction, output overwrite, read/write alias or open
   command scope. Shader/profile failures must retain usable raster output.

Record backend/hardware, SDK and format/profile availability, scenarios, diagnostics
and outcome; do not infer backend coverage from source checks or continuation.

## Visible controls correction regression

On both examples verify Graphics settings opens at startup beside scene controls;
close it and reopen through the scene window button. Expand Renderer, Global
illumination and Reflections, activate GI mode plus GI enabled, then independent
reflections. Confirm available/status and diagnostic signals, queued changes and
explicit save/restore/import/export. Rebind after resize/renderer replacement.
Enable SMAA and confirm exactly one final processing invocation per scene frame,
no duplicate-slot/serial failure, and no processing during renderer update.
These checks require the project lead's build/runtime; none were executed by the agent.

## Manipulation light editor regression

- On startup check renderer indirectLighting=GI and both GI/reflection enabled;
  verify actual available/status, including unsupported devices. Other examples'
  defaults, disabled FSR/SMAA and existing raster fallback remain unchanged.
- Expand Manipulation > Lights > each light. Compare its type/color/intensity,
  shadow/reflection properties with Graphics settings. Change point/spot/
  directional/disabled type; spot-angle availability must track the current type.
- Edit local XYZ position and Euler orientation. Confirm editing pauses animation
  without resetting the Lights parent rotation; position preserves orientation and
  scale, orientation preserves position and scale. Verify directional/spot +Z
  direction changes affect illumination, while omni orientation has no effect.
- Verify shadow casting, reflection participation, source size/sample budgets,
  material/gizmo editing and selection still work, with histories reset/redraw.
  Save/restore and JSON round-trip poses; reject nonfinite values and stale queued
  handles after rebind. Check resize/reconfiguration retains live transform targets.

Only static source checks were performed for this addition; do not treat these
manual cases as executed or infer device coverage.
