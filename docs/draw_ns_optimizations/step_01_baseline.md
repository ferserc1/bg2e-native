# Step 01 — Reproducible baseline

Status: In progress.

## Goal

Establish comparable workloads and confirm the current snapshot-local binding
reuse behavior before changing any rendering implementation.

## Work

1. Inspect the current renderer, tracing snapshot, shadow-table builder, resource
   lifetimes, CPU profiler and delegate preparation. Check repository instructions
   and existing local changes. Do not assume the earlier implementation is unchanged.
2. Record backend, build configuration, validation settings, scene identity,
   viewport/internal/effect resolutions and active effect settings. Obtain a
   production-path comparison only if the same scene/settings are available.
3. Ask the owner to capture the default and large scenes through the existing
   Graphics settings profiler. Capture at least three steady-state reports per
   case, with no editing or dialogs during the interval:
   - All RT disabled.
   - Shadows only, with an enabled shadow-casting light.
   - GI only, then reflections only.
   - Shadows, GI and reflections together.
4. Record scene-frame and presentation counts separately. Note unavailable cases
   rather than silently changing their settings. Preserve a reference image.
5. Confirm one TLAS build per scene frame and two scene-binding creations per
   scene frame for the existing GI/reflection configuration. Multiple layouts may
   legitimately increase that count; document them before diagnosing a regression.

No rendering changes are planned in this step. Request build authorization if
needed rather than compiling automatically. The owner runs the application.

## Acceptance

- Repeatable scene/settings description and baseline log references exist.
- Stable reports are separated from startup, dialogs and editing stalls.
- Any mismatch with the starting evidence is explained or blocks the next step.

## Required updates on completion

Fill in the record below, update `summary.md`, and adjust the instrumentation step
to the actual code and missing evidence.

### Execution record

- Files inspected: `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp`,
  `lib/include/bg2e/draw/pipeline/TracingSceneResources.hpp`,
  `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`,
  `lib/src/bg2e/draw/RendererDeferred.cpp`,
  `lib/include/bg2e/base/CpuProfile.hpp`,
  `lib/src/bg2e/base/CpuProfile.cpp`,
  `lib/src/bg2e/app/MainLoop.cpp`,
  `lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp`,
  `lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp`,
  `doc/architecture/cpu_profiling.md`. Profiler UX follow-up changed
  `lib/include/bg2e/base/CpuProfile.hpp`, `lib/src/bg2e/base/CpuProfile.cpp`,
  `lib/src/bg2e/ui/GraphicsSettingsWindow.cpp`, and
  `doc/architecture/cpu_profiling.md`.
- Capture conditions and log references: Owner supplied context-tagged profiler
  output in chat on 2026-10-20. All captures used the same scene, with variants
  for all effects, GI+reflections, GI only, reflections only and shadows only.
  Backend/resolutions are included in each report's `context:` line. Build and
  validation settings and a reference image remain unreported. The owner confirmed
  the profile file was cleared for each capture; the repeated-looking blocks in
  the original pasted content were repeated text, not duplicate profiler data.
- Observations and baseline numbers (observed, not hypotheses):
  - The all-effects group has stable reports of 14–18 frame calls per ~2.0 s.
    `tracing.shadowTablesAndBuffers` self time is 24.622, 23.004 and 23.090
    ms/frame (maximum calls 24.557, 24.824 and 24.682 ms); inclusive total is
    24.021–24.557 ms/call. This is consistent with the plan's approximately
    24 ms dominant region.
  - In these reports, `renderer.recordLinear` total is 90.415–96.873 ms/call
    and `frame.total` averages are 133.529–140.312 ms/frame. These do not match
    the prior 42–48 ms frame evidence; scene/settings/build comparability is not
    established.
  - The same all-effects reports show `tracing.createSceneBindings` calls of 30
    over 15 scene preparations (2 per scene frame), matching the expected reuse
    count for the reported configuration. Each report has 15
    `metal.tlas.encodeBuild` calls for 15 tracing preparations, consistent with
    one TLAS encoding per scene frame. The report does not establish whether all
    those frames are steady-state scene frames in the plan's intended scene.
  - Later per-effect profiles show the same scene and configuration metadata.
    Shadow table work is approximately 23–25 ms/frame when shadows are enabled
    (all effects and shadows-only), and negligible when shadows are disabled.
    This isolates the expensive region to shadow preparation rather than GI or
    reflection preparation. In the all-effects reports, `effect.shadows.record`
    is about 4.2–4.4 ms/frame inclusive and `effect.shadows.prepare` is about
    1.7–1.8 ms/frame inclusive (about 0.03 ms/frame exclusive); disabled-effect
    records and their low values are not active GPU
    work. The earlier duplicate-looking input should not be used to assign
    individual effect cases because its text repeated blocks.
  - All-effects reports include `metal.queue.waitUntilCompleted` /
    `metal.device.waitIdle` calls with averages around 10–22 ms and individual
    maxima around 22–29 ms.
    These are infrequent and are wall-clock waits, not CPU work. Several reports
    have extremely low frame-call counts, which means they are not comparable
    steady-state samples.
  - An early report records `frame.prepareDelegate` maximum 16,425.034 ms and
    `frame.total` maximum 16,433.366 ms over an 18.136 s reporting window. Another
    reports a 1,619.696 ms `frame.prepareDelegate` maximum. These are isolated
    stalls; their cause is not established and they must not be included as
    steady-state rendering cost.
  - The RT-disabled interval has `tracing.shadowTablesAndBuffers` around
    0.007–0.008 ms/frame, consistent with shadows disabled.
- Checks performed / not performed: Inspected the profiler and relevant rendering,
  shadow-table and wait/delegate call sites. Confirmed from code that
  `TracingSnapshot::resourceSet` performs snapshot-local lookup by layout, set
  index and effective capacity and only profiles cache misses as
  `tracing.createSceneBindings`. Confirmed shadow table construction and buffer
  allocation are nested under `tracing.shadowTablesAndBuffers`, itself nested
  within `tracing.allocateAndUploadInstanceBuffer`; do not interpret the latter's
  inclusive time as upload-only cost. Confirmed profiler documentation describes
  wall time, including driver calls and waits, and does not add GPU synchronization.
  No build, application run, runtime profiling or image inspection was performed
  by the agent. Profiler-context follow-up was not compiled or runtime-tested.
- Decisions, blockers and next action: Accept the context-tagged captures as
  evidence that shadow preparation is the dominant measured CPU preparation cost,
  about 23–25 ms per scene frame when enabled and negligible when disabled, with
  the same scene/settings except the effect switches. Correct the earlier
  interpretation: the owner supplied distinct captures, but the original paste
  repeated text and lacked context, causing the agent to misattribute variants.
  Build/validation settings and reference image remain unreported. Step 02 should
  instrument the shadow builder and separate CPU regions; no optimization is
  authorized yet. The
  profiler UX follow-up adds session IDs/start timestamps, displays session
  metadata, and automatically logs backend, extents, renderer family, active
  deferred RT effects and presentation/scene-frame counters. The owner confirms
  scene identity is not needed because all variants use one unchanged scene.
  These changes
  do not change rendering behavior. No rendering optimization is authorized.
  Step 02 follows once the baseline metadata has been recorded.
