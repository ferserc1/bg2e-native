# Phase 09 Transparent processing regions

## Outcome

Transparent layers avoid eligible full-screen processing outside their conservative screen coverage, while preserving composition order, refraction, background color, and per-layer temporal state.

## Implementation

1. Compute conservative submesh bounds from immutable geometry and project them per transparent item. Use full-screen bounds when clipping, near-plane intersection, deformation, or invalid bounds makes tighter bounds uncertain. Fully offscreen raster layers may be skipped; never remove offscreen geometry from tracing solely because it is outside the camera view.
2. Preserve the existing far-to-near order based on camera-to-object-origin distance. Changing the transparency sorting algorithm is outside this phase.
3. Apply scissor or dispatch regions to transparent G-buffer rasterization, eligible RT receiver work, and composition. Expand regions for filter kernels and other sampling support; fall back to full extent when the required sampling footprint cannot be bounded safely.
4. Explicitly preserve valid background pixels in ping-pong targets outside the composition region. Keep G-buffer coverage clearing and depth restoration correct between layers; stale coverage must not become a receiver for a later pass.
5. Define valid signal/history regions for temporal accumulation and denoise. Reprojected samples, moved layers, newly uncovered areas, and old histories outside the current bounds require explicit validity. Use full-screen temporal processing until region-aware validity is proven; the raster/effect/composition savings must remain independently usable.
6. Handle refraction reads separately from write coverage. Background sampling may extend beyond the object's write rectangle; preserve the complete required source image.

## Verification and exit

Compare against the full-screen path for overlapping transparent objects, masked transparent materials, screen edges, near-plane crossings, camera/object motion, disappearing layers, refraction, and each indirect-lighting mode. Check pixels outside the region and multi-frame histories for stale data.

Measure processed pixels and GPU timings separately from CPU binding reductions. Small transparent objects should reduce eligible processed area; full-screen fallback cases must remain correct. Exit with region processing active for proven cases, complete fallback elsewhere, and unchanged layer order and optical behavior.
