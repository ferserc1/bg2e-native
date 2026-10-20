# Plan Status

## Step 1 in progress: Reproducible baseline

Date: 2026-10-20
Changes:
- `lib/include/bg2e/base/CpuProfile.hpp`, `lib/src/bg2e/base/CpuProfile.cpp`:
  Added a monotonically increasing session identifier and start timestamp to
  each profiler session marker, exposed current session metadata, and added a
  context-provider hook for report-boundary metadata.
- `lib/include/bg2e/draw/RenderLoop.hpp`, `lib/src/bg2e/draw/RenderLoop.cpp`:
  Provide automatic backend, surface/internal resolution, frame-count, renderer
  family and deferred RT effect context to CPU profile intervals; report actual
  enabled state in addition to pass availability.
- `lib/include/bg2e/draw/RendererDeferred.hpp`,
  `lib/src/bg2e/draw/RendererDeferred.cpp`: Expose combined pass-availability and
  enabled-state diagnostics for AO, GI and reflections, so report context reflects
  selected settings instead of merely available passes.
- `lib/src/bg2e/ui/GraphicsSettingsWindow.cpp`: Show the current session metadata
  beside the CPU profile path and remind the owner to identify each capture's
  scene/effect configuration when sharing it. Scene/effect metadata itself is
  now captured automatically in report intervals; the owner confirms the scene
  remains fixed across variants, so scene naming is not needed.
- `doc/architecture/cpu_profiling.md`: Documented session boundaries and how to
  share separate per-configuration captures without ambiguity, and described
  automatically collected runtime context and the unavailable scene identity.
- `docs/draw_ns_optimizations/step_01_baseline.md`: Corrected the earlier
  interpretation of pasted blocks and recorded owner clarification and known
  evidence gaps.
- `docs/draw_ns_optimizations/summary.md`: Updated Step 01 status and execution
  record.

The owner clarified that all captures used the same scene with distinct effect
variants. The original pasted text repeated blocks, which caused the agent to
misattribute configurations; the prior duplicate-capture suspicion is retracted.
Context-tagged profiles show `tracing.shadowTablesAndBuffers` at approximately
23–25 ms per scene frame when shadows are enabled, and about 0.007–0.008 ms/frame
when disabled. Build/validation settings and a reference image remain unreported.
Profiler code/documentation changes have not been compiled or runtime-tested. No
rendering optimization was implemented.

## Step 4 implemented: indexed GPU shadow UV lookup

Owner approved replacing the reservation experiment with indexed vertex/index
buffer reads in both shadow shaders. Removed per-frame shadow triangle UV packing,
dedicated buffers/uploads and their profiler labels/counters. Kept the lightweight
alpha metadata/texture table and its fallback semantics. Standalone shadows now
prepare tracing snapshots with the same immutable geometry cache. Updated probes,
API/profiling documentation and Step 04/05 handoff. No compilation or runtime
validation performed; regenerate v3 native shadow kernels with the library build.

## Step 3 evidence review (historical reservation proposal, superseded)

Date: 2026-10-20

Proposal: checked sizing and exact vector reservation in the shadow-table builder,
preserving packed bytes, instance ordering and texture fallback behavior. Allocation
cost is not independently measured; benefit must be verified, not assumed. The
separate, larger Metal resource-binding cost is documented but excluded from this
proposal. See `step_03_evidence_review.md` for evidence, invariants and approval gate.
Step 04 remains blocked; no optimization or build was performed.

## Step 2 completed: Isolate remaining CPU costs

Date: 2026-10-20
Changes:
- `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`: Added non-overlapping shadow
  builder regions and bounded table/count/byte metrics; separated storage create
  and update labels.
- `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp`: Corrected the instance
  upload timing boundary and split shadow table build, shadow instance upload,
  shadow triangle upload and material upload. Added counts for instances/meshes,
  capacities/bytes, and skipped shadow capacity.
- `lib/src/bg2e/draw/RendererDeferred.cpp`: Counted actual shadow/AO/GI/reflection
  consumers per prepared scene.
- `lib/src/bg2e/draw/DefaultRenderLoopDelegate.cpp`,
  `lib/src/bg2e/draw/RenderLoop.cpp`, `lib/src/bg2e/gpu/metal/Device.cpp`,
  `lib/src/bg2e/app/MainLoop.cpp`,
  `lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp`,
  `lib/src/bg2e/draw/FsrUpscaler.cpp`: Added initiator labels
  around existing waits and delegate update/reconfigure/target attachment paths.
- `lib/src/bg2e/ui/GraphicsSettingsWindow.cpp`: Clarified that settings should be
  changed before profiling and the window may be closed immediately after starting.
- `doc/architecture/cpu_profiling.md`: Defined new labels and per-interval
  aggregate counter format.
- `docs/draw_ns_optimizations/step_02_instrumentation.md` and `summary.md`:
  Recorded implementation scope and owner capture request.

Owner supplied same-scene RT-off, shadow-only, GI-only, reflection-only and
combined profiles. `shadowTable.trianglePacking` measures about 23.4–25.2 ms/frame
for ~380,302 triangles per shadow-enabled preparation; builder inclusive is
~23.5–23.8 ms/frame. Other builder work/uploads are small, and shadow-disabled
snapshots skip table preparation. Queued renderer updates explain the long device
drains in transition intervals; low-level completion waits in stable intervals
remain unattributed. A nested scope defect and duplicate wrapper label were
corrected; the attached packing scope is valid evidence, so no additional capture
is needed for this finding. Enabling profiling and closing Graphics settings
immediately is valid. No build/runtime validation or rendering optimization was
performed. Step 03 should form a narrow evidence-based proposal and record the
remaining completion-wait question.
