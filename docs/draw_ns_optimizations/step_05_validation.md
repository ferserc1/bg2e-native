# Step 05 — Validate behavior and compare renderers

Status: Planned.

## Indexed shadow implementation handoff

The owner approved removal of CPU shadow UV packing without additional profiling
as a prerequisite. Step 04 implements it. Build the library and regenerate both
v3 shadow kernels together before native execution. Run the shadow/GI probes and
the standalone SoftShadowPass checks with shared geometry, nonzero vertex/index
bases, UV0/UV1 and barycentric interpolation, texture overflow, empty scenes and
multiple frames in flight. Ensure no shadow triangle table/upload remains; geometry
cache creation on mesh changes is still expected. No agent build or runtime
validation has been performed. New performance measurements are not claimed.

## Goal

Verify the measured gain is real, preserves behavior and can be compared fairly
with production rendering under per-frame reconstruction.

## Validation matrix

The owner runs authorized builds and captures; the agent must not claim runtime
success for unexecuted cases.

- Default and large scenes under the baseline RT combinations.
- Shadows with and without eligible shadow-casting lights; empty/filtered scenes.
- Multiple instances of shared geometry and multiple submeshes with distinct
  materials. Include existing alpha-tested and transparent content as regression
  coverage only, not as a new optimization target.
- Material/texture edits, object transforms and additions/removals on subsequent
  frames: per-frame rebuilding must still expose changes immediately.
- Several frames in flight, resize, scene replacement, effect toggles and cleanup.
- Metal first, matching the diagnosed problem; Vulkan when a supported runtime is
  available. Record unavailable platforms instead of implying they were tested.

Check visual output against reference images and backend validation diagnostics.
Repeat the same steady-state captures with diagnostics enabled, then have the
owner confirm interactive performance with profiling disabled to account for
instrumentation and file-output overhead.

## Production comparison

Where equivalent scenes and settings are supported, record production and draw
backend/configuration, scene complexity, resolutions, effect settings and timing
method. Both paths must retain per-frame reconstruction. If transparency or effect
algorithms differ, disclose the difference; do not attribute every remaining gap
to resource preparation or claim equivalent rendering workloads without evidence.

Report medians/ranges from repeated captures, operation counts, visible stalls and
CPU waits separately. Existing profiler interval averages/maxima are not per-frame
percentiles; do not fabricate percentile statistics from them.

## Completion criteria

- Owner confirms improved steady-state performance and unchanged expected output.
- Scene tables/TLAS remain per-frame; scene-binding creation remains bounded by
  used layouts per snapshot rather than composition count.
- No observed lifetime/validation regression in the cases actually executed.
- Remaining dominant regions and unexplained stalls are documented, with no
  automatic expansion into deferred optimization categories.

## Required updates on completion

Update this file and `summary.md` with final results, tested configurations,
unverified cases and deferred recommendations. Mark the overall plan completed
only when its acceptance criteria are satisfied, otherwise record the blocker.

### Final record

- Tested configurations and log/image references: Pending.
- Baseline versus optimized versus production results: Pending.
- Visual and lifetime validation results: Pending.
- Profiling-disabled confirmation: Pending.
- Checks not performed / remaining limitations: Pending.
- Remaining bottlenecks and separately proposed follow-up work: Pending.
- Owner acceptance: Pending.
