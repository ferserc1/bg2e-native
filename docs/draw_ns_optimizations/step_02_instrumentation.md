# Step 02 — Isolate remaining CPU costs

Status: Completed.

## Goal

Explain the approximately 24 ms shadow preparation region and attribute recurring
completion waits and isolated delegate stalls without changing rendering behavior.

## Instrumentation work

Use the existing opt-in CPU profiler and Graphics settings control. Do not emit
per-triangle/per-texture log lines or introduce GPU waits for measurement.

### Shadow tables

- Separate `buildShadowQueryTable` CPU processing from each buffer allocation/upload.
- Correct the instance-upload timing boundary so it excludes shadow processing.
- Split the builder into a small number of measured regions, based on inspection:
  instance metadata, resource deduplication, triangle-data sizing/packing and other
  substantial work. Avoid clock reads inside every triangle iteration.
- Add bounded aggregate counts where useful: collected instances, unique meshes,
  packed triangles, table entries, buffer bytes, capacity and overflow slots.
- Record whether shadow resources are prepared when there is no actual shadow
  consumer. Treat this as evidence, not permission to change behavior yet.

### Delegate and synchronization

- Trace `prepareFrame` through renderer replacement, queued settings changes,
  deferred operations and any GPU drain sites actually present in current code.
- Add named regions at relevant call sites so identical low-level `waitIdle`
  measurements can be attributed to their initiating operation.
- Separate actual CPU work from waiting. Do not describe wall time as CPU utilization.
- Identify whether settings UI activity repeats costly synchronization even when
  no setting changes, without presuming that it does.

Preserve existing diagnostic labels where useful for comparisons. Update the
profiler documentation to explain any changed labels and counts. Keep diagnostic
overhead bounded and report any known timing limitations.

## Owner handoff

The owner may enable profiling from Graphics settings and close that window
immediately; this does not invalidate a capture. An open-but-untouched window
capture is optional only if UI-specific stalls are under investigation. Do not
infer that a long interval was rendering work.

Stop here until the owner supplies the results. Do not compile unless authorized.

## Acceptance

- Shadow preparation time is attributed to meaningful non-overlapping work regions.
- Initiators of device drains are attributed; unresolved low-level completion
  waits are documented as a separate evidence question.
- Profiling changes do not alter resource preparation or shader behavior.

## Required updates on completion

Update this record and `summary.md` with actual findings, label definitions and
capture references. Revise Step 03 hypotheses accordingly.

### Execution record

- Instrumented files and regions: `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`
  now separates instance metadata, alpha-texture deduplication, triangle-data
  validation and triangle UV packing, and emits per-interval aggregate counts for
  instances, alpha-test instances, alpha-texture comparisons/unique textures,
  packed triangles, entries, capacity, overflow slots and buffer bytes.
  `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp` now separates instance
  upload, table build, shadow-instance upload and shadow-triangle upload; the
  former misleading `tracing.allocateAndUploadInstanceBuffer` label is replaced
  with `tracing.instanceBufferUpload`. It counts collected instances, unique
  meshes, buffer byte sizes and whether shadow preparation was skipped for zero
  shadow capacity. `lib/src/bg2e/draw/RendererDeferred.cpp` counts actual shadow,
  AO, GI and reflection consumers. `lib/src/bg2e/draw/DefaultRenderLoopDelegate.cpp`
  labels delegate preparation, queued renderer updates, settings attach/detach,
  replacement/scene replacement, and renderer reconfiguration waits. `lib/src/bg2e/draw/RenderLoop.cpp`
  labels target rebuild and shutdown drains. `lib/src/bg2e/gpu/metal/Device.cpp`
  separates its nested submission-drain body while retaining
  `metal.device.waitIdle`. `lib/src/bg2e/app/MainLoop.cpp` and
  `lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp` label safe-update and
  application execution drains. `lib/src/bg2e/draw/FsrUpscaler.cpp` labels resize
  and cleanup drains. The Graphics settings UI requests a capture with the window
  closed; an open-but-untouched capture is optional for a UI-specific investigation.
  Profiler documentation defines
  the revised labels and per-interval aggregate counters.
- Owner capture request / received log references: Owner supplied a context-tagged
  Metal log in chat on 2026-10-20. It contains RT-off, shadow-only, GI-only,
  reflection-only and combined variants. These include repeated stable intervals
  with `delegate.pendingRendererUpdates total=0`. The owner enabled profiling from Graphics
  settings and closed that window; this is valid and does not invalidate the
  profile. An open-but-untouched UI capture is not needed for the shadow-table
  question; it is optional only if investigating UI-related stalls.
- Capture interpretation/procedure: Some supplied intervals include queued
  renderer updates and drains (including batches of 34, 24, 17 and 11). These are
  configuration-transition work, not steady-state cost. Stable intervals with
  `delegate.pendingRendererUpdates total=0` are present in the supplied log. The
  user may activate profiling in Graphics settings and close the window
  immediately; this capture is valid.
- Measured counts, bytes and timings (observed in supplied log): For shadow-enabled
  variants, there are 855–912 alpha-test instances per 15–16 scene preparations,
  380,302 packed triangles per preparation, and ~18.25 MB triangle buffers per
  preparation. `shadowTable.trianglePacking` reports ~23.4–25.2 ms/frame
  exclusive and ~0.41–0.42 ms/call; `shadowTable.build` inclusive is ~23.5–23.8
  ms/scene frame.
  Deduplication, metadata, validation and shadow instance-buffer upload are each
  small (<0.03 ms/frame); shadow triangle-buffer upload is ~1.0–1.14 ms/frame.
  Shadow-disabled profiles show `shadowTable.skippedNoShadowCapacity` once per
  scene preparation, and no shadow table build. These values isolate UV triangle
  packing as the dominant measured table-builder region; this is not yet an
  optimization authorization.
  `effect.shadows.prepare` totals around 1.7–1.9 ms/call inclusive but only about
  0.03 ms exclusive; nested values must not be compared as exclusive work.
  Several intervals show queued settings updates and associated drain waits, so
  those intervals are configuration-transition evidence, not steady-state costs.
- Checks performed / not performed: Inspected the shadow builder, snapshot
  preparation, renderer effect selection, delegate replacement/update/reconfigure
  flow, Metal device drain, queue completion wait, and Graphics settings path.
  No build or runtime execution performed; compilation was not requested. Static
  `git diff --check` passed after edits. The project was not compiled, built or
  run as instructed; compilation and runtime behavior remain unverified.
- Facts, hypotheses, blockers and next action: Inspection of the supplied log
  exposed an instrumentation lifetime bug in an earlier capture: a
  `tracing.uniqueMeshCollection` scope remained alive across later preparation
  stages. The supplied profile now shows `tracing.instanceBufferUpload` around
  0.01 ms/frame and `tracing.geometryPacking` around 0.001 ms/frame, confirming the
  fix is present. A subsequent code change separated the sequential parent-stage
  label from the `tracing.shadowTablesAndBuffers` wrapper, which had duplicated
  that wrapper label in the supplied reports; this only affects wrapper aggregation.
  The direct `shadowTable.trianglePacking` and `shadowTable.build` rows are
  meaningful: packing is ~23.4–25.2 ms/frame over 380,302 triangles/preparation;
  build inclusive is ~23.5–23.8 ms/frame. The attached profiles suffice to identify
  triangle UV packing as the dominant builder work; no additional capture is
  needed for that finding.

  Instrumentation adds no per-triangle
  clock reads and no GPU waits. Shadow table build has 3–4 profiled regions
  (metadata, texture deduplication, triangle validation/packing), while upload
  regions are sequential; totals are non-overlapping within the builder but the
  `tracing.shadowTablesAndBuffers` wrapper remains inclusive. Counter collection
  is opt-in, emitted with each interval on the render thread, and cleared with
  that interval. Whether expensive shadow table preparation occurs when no actual
  shadow consumer remains is answered for these configurations: shadow-disabled
  intervals count `skippedNoShadowCapacity` once per tracing preparation. Wait
  initiator labels attribute long `metal.device.waitIdle` intervals to queued
  renderer-setting updates when the pending-update counter is nonzero. Repeated
  `metal.queue.waitUntilCompleted` waits in stable intervals remain unattributed to
  a higher-level initiator. No optimization was implemented or authorized. Step 03
  is next.
