# Experimental renderer CPU diagnostics

Open **Graphics settings** and select **Enable CPU profiling**. Clear the checkbox
to stop recording. Profiling starts disabled and is not persisted as a graphics
preference. No environment variable, command-line option or CMake option is used.
The control is also available in the renderer diagnostics window.

Reports are appended to `PlatformTools::homePath() / (appId + "-cpu-profile.log")`,
using `MainLoop::current()->appId()` for application identity. Path separators in
the ID are replaced with underscores. For the composer, the filename is
`org.bg2e.apps.draw.bg2e-composer-cpu-profile.log`. The window displays the full
path, current session identifier/start time and file errors. Existing reports are
retained, with a unique monotonically increasing session ID plus Unix and local
human-readable start times written immediately after each session marker. Each
reporting interval also has its own `[CPU profile]` header. When sharing results, include the session marker,
the complete interval(s) for one configuration, and a short note naming the scene
and active effects; do not concatenate unrelated configurations without their
markers. The same UI works with Metal and Vulkan, including macOS app bundles.
Each frame-boundary report automatically records context supplied by the active
draw render loop: backend, surface/internal extents, cumulative presentation and
scene-frame counts, renderer family, and effective deferred RT effects (renderer
availability combined with each effect's enabled setting). A stable scene
identifier/name is not exposed by the current scene API, so it is reported as
unavailable rather than guessed. Context appears as a `context:` line in each
report interval. Backend/resolution/effect context is automatic; build
configuration and validation-layer state are not inferable from the active
renderer and are not included. When the same scene is held constant across effect
variants, no scene name is needed to interpret the comparisons.

Every approximately two seconds, at a completed RenderLoop frame call boundary,
the file receives a table sorted by exclusive (`self`) CPU wall-clock time:

- `self ms/frame`: time excluding nested instrumented scopes, normalized by the
  number of RenderLoop frame calls in the reporting window. Context is sampled at
  each report boundary; scene/effect changes during an interval may therefore be
  represented by the boundary configuration rather than every frame in it.
- `total ms/frame`: inclusive time; do not sum nested totals.
- `avg ms/call`, `max ms/call`, `calls`: distinguish one large stall from many
  repeated operations, including effects invoked for multiple layers.
- `frame.total`: whole RenderLoop call, including waits. The header also reports
  elapsed wall time and frame call count (not necessarily rendered scene frames).

These are wall-clock measurements on the calling CPU thread, **not CPU utilization
or GPU execution timestamps**. Driver work, scheduling and blocking GPU/presentation
waits all contribute. No additional GPU waits or synchronization are inserted.
When disabled, newly created scopes do not read clocks or accumulate samples.
Enabled profiling has overhead, particularly for scenes with many resource bindings;
compare against an uninstrumented run after diagnosing the bottleneck.

Important stages:

| Stage | Interpretation |
|---|---|
| `frame.acquireAndWaitSlot` | Surface acquisition and reuse of a completed slot |
| `metal.surface.waitSlotSubmissions` | CPU blocked waiting for previously submitted GPU work |
| `metal.surface.nextDrawable` | Presentation drawable acquisition; may block on compositor/backpressure |
| `frame.retireSlotResources`, `frame.deferredCleanup` | Resource retirement/destruction |
| `frame.uiPreparation`, `frame.uiComposition` | UI work, including editor callbacks |
| `renderer.extractQueueAndCamera` | Queue extraction, sorting, camera setup |
| `renderer.temporalSignature`, `renderer.objectBindings` | Scene comparison and raster object data |
| `tracing.*` | Collection, geometry packing, material/texture tables, buffer allocation, scene creation |
| `tracing.shadowTablesAndBuffers` | Inclusive shadow metadata/texture table construction and instance-buffer upload; no UV triangle packing or upload |
| `tracing.prepareShadowResources` | Sequential preparation-stage interval; distinct from the nested shadow wrapper |
| `tracing.buildShadowQueryTable` | Shadow table builder inclusive time, with non-overlapping `shadowTable.*` child regions |
| `shadowTable.instanceMetadata`, `shadowTable.alphaTextureDeduplication` | Non-overlapping shadow metadata builder CPU regions |
| `shadowTable.*` counters | Aggregate instance/resource counts and metadata bytes; total and max per count event are not normalized per frame |
| `tracing.instanceBufferUpload` | Instance storage buffer upload only; excludes shadow preparation |
| `tracing.shadowInstanceBufferUpload` | Shadow metadata storage-buffer upload; geometry remains in shared immutable tracing buffers |
| `shadow.prepareBindings`, `shadow.geometryBufferBindings` | CPU binding preparation and number of existing geometry buffers bound (two per active shadow recording) |
| `tracing.getSceneBindings` | All scene-binding requests, including inexpensive snapshot-cache hits |
| `tracing.createSceneBindings` | Cache misses only: native scene tables created once per used layout/set/capacity per snapshot |
| `metal.tlas.*`, `vulkan.tlas.*` | CPU allocation and encoding of TLAS construction, not GPU build duration |
| `effect.*.record`, `layer.*` | CPU recording for effects and render layers |
| `metal.resourceSet.resolveImageViews` | Resolution of all sampled image views |
| `metal.argumentArray.*` | Argument encoder creation, allocation and table encoding |
| `metal.bindResources.*` | Native binding and residency declarations |
| `frame.submit`, `metal.queue.waitUntilCompleted` | Submission and any synchronous completion wait |

The indexed shadow implementation removes the legacy `shadowTable.trianglePacking`,
`shadowTable.triangleDataValidation`, `tracing.shadowTriangleBufferUpload` regions
and triangle packing/byte counters. Their absence is intentional: both Vulkan and
Metal interpolate UV0/UV1 from shared GPU vertices/indices at each candidate hit.
`tracing.geometryPacking` still describes the existing immutable geometry cache
creation when mesh identities change, not a per-frame shadow triangle table.

Set the effect variant before enabling profiling, then avoid edits while collecting
reports. The checkbox may be enabled from Graphics settings and that window closed
immediately; closing it does not invalidate the capture. Only capture with the
window open but untouched if specifically investigating whether the UI contributes
to a suspected stall. Keep scene/build/backend fixed. Wait for queued settings
updates to settle before enabling profiling; intervals with nonzero
`delegate.pendingRendererUpdates` describe configuration transitions, not steady
state. Ignore initial loading/resize intervals when comparing performance.

Reports are flushed to disk after each reporting interval. Stop after at least
one complete interval; an incomplete interval at deactivation is discarded. Aggregate
counts are emitted with each interval on the render thread, so deactivation from
the UI thread cannot lose them. The
stop marker also writes bounded aggregate counters (`count <name> | total=... |
max/call=...`) for the session; these counters are not normalized per frame and
are independent of timing totals.
checkbox label is the session boundary: turn profiling off after each requested
configuration, then on again for the next one. Starting a new session never
truncates the log. Changing sessions resets thread-local aggregates and rejects
old in-flight samples. The session ID advances even if the application log is not
deleted, so independent captures remain distinguishable.
Opening/writing failures disable recording and are shown in the settings window.

All aggregation is thread-local. Reports describe the render thread; unrelated
background threads are not merged into its samples. Diagnostic output is batched,
not emitted per object/texture/sample, and output time itself is excluded from the
completed frame sample.
