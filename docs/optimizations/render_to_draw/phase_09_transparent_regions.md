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

## Implementation record

Source implementation is complete; native compilation, image parity, GPU validation and timing acceptance remain pending.

- Uploaded mesh generations capture bounds per indexed submesh. CPU vertex edits do not change bounds for existing GPU geometry; changed live draw ranges disable bounds until the next upload. Invalid bounds use full extent. The built-in rigid G-buffer shader has no deformation; custom G-buffer, lighting and final layers use the complete path. Debug views also retain the complete path.
- Transparent bounds use the current jittered projection, view and captured model transform. Eight AABB corners provide conservative projected coverage with a two-pixel raster/sampling guard. Near/far/eye-plane uncertainty, nonfinite inputs and non-affine model transforms fall back. Empty projected regions skip only raster layers; tracing snapshots are collected and built before this decision. The original stable object-origin distance ordering is unchanged.
- `RenderingDescription::scissor` restricts draws while preserving the full viewport and full attachment clear area. Vulkan and Metal apply it when binding graphics pipelines. The G-buffer clears all six color attachments and restores opaque depth before each processed layer, preventing stale coverage.
- Regional composition copies the complete current background to the destination, uses attachment Load and restricts only writes. Refraction keeps access to the complete source background; no read-footprint assumption is made.
- Pointwise soft-shadow receivers use a conservative origin-anchored dispatch covering the region, rounded up to workgroups. Complete outputs are cleared to neutral visibility first. This deliberately preserves the paired shader ABI and omits only right/bottom workgroups. Standalone incompatible extents/usages retain full dispatch.
- AO, GI, reflections, temporal accumulation and denoise retain their full signal/history domains, so scaled sampling, filter support, reprojection, moved layers and newly uncovered pixels retain existing validity. Their allocation, clears and full-screen dispatch costs remain visible. Other region savings operate independently.
- `RendererDeferred::setTransparentRegionsEnabled(false)` selects the complete diagnostic path. Area/workgroup counters are independent of prior binding counters; full background-copy pixels are reported separately. Existing CPU effect/pass scopes remain available, but GPU timings and speedup have not been measured.

Static verification: `git diff --check`, source contract checks and 20,000 independent positive-w projective-envelope arithmetic samples. The CPU probe in `tests/draw/transparent_regions/probe.hpp` is supplied as source only. Neither native compilation nor probe execution was requested or performed. See that directory's README for the outstanding two-backend visual and lifecycle matrix. No CMake or shader files were changed by this phase.
