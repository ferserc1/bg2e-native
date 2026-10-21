# Render-to-Draw Optimization Report

**Updated:** 2026-10-21  
**Scope:** Experimental `bg2e::draw` renderer, with production `bg2e::render` used
as a behavioral/reference implementation.  
**Status:** The approved indexed-UV shadow change is implemented. Native build,
shader compilation and runtime validation remain outstanding.

## Summary

Two substantial CPU-side optimizations have been applied to the draw tracing path:

1. Reuse tracing scene bindings within a frame snapshot instead of recreating
   equivalent Metal resource sets for each effect/composition.
2. Remove CPU-built per-triangle shadow UV tables. Vulkan and Metal shadow shaders
   now fetch indices and UVs from the shared GPU tracing geometry at candidate
   intersections, following the indexed lookup model already used by production
   ray tracing and other draw tracing shaders.

The owner-supplied Metal profile after the shadow change shows the old shadow-table
packing cost is no longer material: `shadowTable.build` is about 0.067–0.071 ms per
scene preparation, compared with roughly 23.4–25.2 ms previously spent packing
about 380,302 triangles. The dominant observed CPU work is now Metal resource and
argument-array preparation. Large device drains coincide with queued renderer
updates, so they are transition costs in this capture, not a steady-state shadow
cost.

## Optimizations applied

### Snapshot-local tracing binding reuse

- **Change:** Cache fully initialized tracing resource sets in each
  `TracingSnapshot`, keyed by pipeline-layout identity, set index and effective
  resource-array capacity. Repeated consumers in that snapshot reuse the set.
- **Lifetime:** Cache ownership is snapshot-local; the snapshot and resource sets
  remain alive through GPU completion. A new frame/job receives a new cache.
- **Observed earlier Metal comparison:** Scene resource-set creations fell from
  46 to 2 per scene frame; scene-binding preparation fell from about 136 ms to
  about 6.2 ms; `renderer.recordLinear` fell from about 142 ms to about 11.7 ms.
  Stable `frame.total` changed from about 177–192 ms to about 42–48 ms in those
  earlier captures. These are historical owner-supplied comparisons, not numbers
  from the latest all-effects profile.

### Indexed GPU UV lookup for alpha-tested shadows

- **Change:** Remove `ShadowTriangleUV`, CPU triangle packing, the dedicated
  triangle-UV storage buffer and its per-frame upload. `ShadowInstanceData` now
  identifies a shared first-index range and vertex base. Both native shadow
  kernels read the triangle's indices and vertex UV0/UV1 from shared tracing
  buffers, interpolate with hit barycentrics and sample the existing alpha
  texture/sampler.
- **Preserved behavior:** Dense TLAS instance mapping, submesh ranges, UV-set
  selection and scale, alpha multiplier/cutoff, texture admission/fallback,
  transparent policy and ray traversal remain represented by the existing shadow
  metadata and shader logic.
- **Standalone use:** `SoftShadowPass` now prepares retained tracing snapshots too,
  so standalone shadows use the same immutable shared geometry buffers as renderer
  snapshots.
- **Binding contract:** Shadow kernel/layout v3. Shared vertices bind at Vulkan
  set 0 binding 2 / Metal buffer 3; shared indices at Vulkan binding 9 / Metal
  buffer 7. Existing alpha image/sampler arrays remain at Vulkan bindings 16/24
  and Metal argument records 5/6. Rebuild the library and both native shadow
  kernels together; previous kernel binaries are ABI-incompatible.
- **CPU profile evidence:** In the supplied post-change profile, `shadowTable.build`
  is about 0.067–0.071 ms per preparation; `tracing.shadowInstanceBufferUpload`
  is about 0.004–0.005 ms. The old direct packing scope and triangle-buffer upload
  no longer exist. The earlier packing measurement was about 23.4–25.2 ms per
  preparation for about 380,302 triangles. This establishes removal of the CPU
  packing work, not a measured FPS or GPU-time improvement.

### Profiling and diagnostics supporting the work

- Added opt-in per-stage CPU profiles and bounded aggregate counters for shadow
  preparation, Metal resource preparation, binding work and selected wait
  initiators. No measurement GPU waits were introduced.
- Corrected profiling scope lifetimes and a duplicate timing label discovered
  during capture review. Removed profiling regions/counters that only described
  the deleted triangle table; retained metadata, binding, effect, TLAS and wait
  profiling.
- Added profile context for backend, resolution, renderer and active RT effects,
  plus session identifiers and capture guidance.

## Current profile: owner capture, 2026-10-20

The supplied capture uses the same scene as the earlier profiles and reports
Metal, deferred rendering, 3008×1568 surface and 1804×940 internal resolution.
It has short intervals of 14–17 frame calls, generally 15 or 16. Although the
owner described it as all RT effects enabled, the logged context is
`rt_shadows=on`, `rt_gi=on`, `rt_reflections=on`, **`rt_ao=off`**. AO consumer
counters are zero. Interpret it as shadows, GI and reflections enabled; AO was not
active according to the profiler.

Across the supplied intervals, the leading normalized CPU rows are approximately:

| Profile stage | Observed self time per frame | Interpretation |
|---|---:|---|
| `metal.argumentArray.encodeEntries` | 34.5–36.9 ms | Metal argument-array encoding |
| `metal.resourceSet.resolveImageViews` | 24.1–25.8 ms | Sampled image-view resolution |
| `metal.resourceSet.resolveSamplers` | 12.5–13.3 ms | Sampler resolution |
| `metal.argumentArray.createEncoder` | 11.2–12.0 ms | Argument encoder creation |
| `shadow.prepareBindings` | about 4.1–4.4 ms | Exclusive CPU binding work; inclusive total about 81–87 ms/frame over 345–391 calls |
| `shadowTable.build` | about 0.067–0.071 ms | Remaining lightweight shadow metadata/texture table |
| `tracing.shadowInstanceBufferUpload` | about 0.004–0.005 ms | Shadow instance metadata upload |

These rows are CPU wall-clock measurements. They do not measure GPU execution.
Several are nested: do not add inclusive `total` values from `effect.shadows.record`,
`shadow.prepareBindings`, `renderer.recordLinear` or other enclosing regions. The
Metal argument-array and binding rows indicate the next investigation area, but
the profile alone does not prove which binding operations can be safely shared or
removed.

### Capture limitations

- Each interval contains only 14–17 frame calls; it is a short diagnostic capture,
  not a robust distribution or final benchmark.
- `delegate.pendingRendererUpdates` is nonzero in these intervals, with 4–7
  queued updates/drains per interval. The resulting `metal.device.waitIdle` / drain
  waits are about 69–99 ms per wait call and are explicitly nested under queued
  renderer updates. Exclude them from steady-state rendering comparisons.
- The enabled effects are not all on: AO is off in the automatic context and its
  consumer count is zero.
- Consequently, `frame.total` values of roughly 131–146 ms/frame in these
  transition-heavy intervals should not be compared directly with earlier stable
  captures.

## Pending work

1. **Build and shader validation:** Rebuild the library and regenerate Vulkan and
   Metal shadow kernels for v3. Compilation and native execution have not yet been
   performed by the agent.
2. **Behavior/lifetime validation:** Run the updated shadow and tracing probes on
   Metal and Vulkan where supported. Check multiple meshes/submeshes, nonzero
   packed vertex/index bases, UV0/UV1, barycentric interpolation, alpha holes,
   overflow fallback, empty scenes, standalone pass use and acquired-slot lifetime.
   Confirm output against reference images and backend validation diagnostics.
3. **Stable owner profile:** If performance attribution is desired, capture a
   stable interval after queued settings updates settle. Record the actual effect
   context; enable AO only if the intended comparison is all four RT effects.
   No further measurement is required to justify removal of the CPU UV packing;
   this capture is for validating the resulting build and understanding remaining
   costs.
4. **Metal resource-preparation follow-up:** Investigate repeated argument-array
   encoding, image-view/sampler resolution, and `shadow.prepareBindings` as a
   separate optimization. Preserve resource-set identity, layout/capacity
   distinctions, per-invocation resource lifetimes and Metal residency rules.
   This is not yet approved as an implementation change.
5. **Wait attribution:** The supplied long device drains correspond to queued
   renderer updates. Stable recurring queue-completion waits were unresolved in
   earlier profiles; determine their higher-level initiator only if they remain
   relevant after a stable capture. Do not narrow synchronization without proving
   the GPU-lifetime requirements.

## Constraints and verification status

- Production `bg2e::render` was not modified. `bg2e::gpu` was not refactored.
- No CMake files were changed. No build, C++ compilation, shader compilation or
  native runtime validation was performed by the agent.
- Static source checks and `git diff --check` passed for the implementation work.
- The indexed shadow implementation is complete as a code change, but behavioral,
  visual and backend validation remains pending.
