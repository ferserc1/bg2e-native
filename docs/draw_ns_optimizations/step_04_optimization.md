# Step 04 — Implement the approved CPU optimization

Status: Completed implementation; compilation and native runtime validation pending.

## Goal

Remove per-frame shadow triangle-UV packing and uploads. Fetch attributes from
existing shared GPU tracing geometry at candidate hits, preserving alpha behavior,
frame ownership and per-frame instance/material/TLAS reconstruction.

## Implementation checklist

- Owner approved replacement of the reservation experiment with indexed GPU UV
  lookup, including paired native shaders and removal of obsolete C++ data.
- Removed `ShadowTriangleUV`, CPU triangle vectors/loops, dedicated triangle
  buffers/uploads, standalone duplicate collection/TLAS preparation and obsolete
  triangle packing/byte counters. Retained alpha table ordering, scalar fallback,
  cutoff, sampler, UV selection/scale and sample budget.
- Shadow metadata stores shared first-index and vertex-base offsets in dense TLAS
  order. Both shaders fetch three indices/UVs and interpolate at the hit.
- Shared snapshots retain existing immutable geometry; standalone SoftShadowPass
  now uses its own retained TracingSceneResources and acquired-slot snapshots.
  No new global waits or per-frame geometry cache policy was introduced.
- Layout/kernel contract is v3: shared vertices Vulkan2/Metal buffer3, indices
  Vulkan9/Metal buffer7, capacity reserves (2,1,10). Existing arrays remain at
  Vulkan16/24, Metal records5/6. Library and both native kernels must be rebuilt
  together; old binary shaders are incompatible.
- Retained metadata/upload, effect recording, TLAS and wait profiling. Added
  `shadow.prepareBindings` and `shadow.geometryBufferBindings`; geometry cache
  packing remains distinct from the removed shadow UV reconstruction.
- Updated shadow/GI probes for indexed ranges, both UV sets, barycentrics,
  multiple meshes/submeshes, geometry reuse and empty snapshots. Existing fallback
  and manual native checks remain applicable. Probes were not compiled or run.

Implement the smallest approved change. Preserve:

- One scene collection and TLAS build per active scene frame.
- Fresh per-frame instance/material tables; no dirty-state/version caching or refit.
- Snapshot-local reuse of scene bindings, including layout/capacity distinctions.
- Valid resources through GPU completion and safe reuse of acquired frame slots.
- Dense TLAS instance order, original object/submesh identity, triangle ranges,
  texture/sampler correspondence, deterministic fallback and overflow behavior.
- Existing alpha-test behavior, transparent sorting and effect invocations. Only
  the approved shadow shader binding ABI changes; raster/material ABIs remain.
- Independent public effect usage and both Vulkan and Metal paths.

Keep targeted profiling available to quantify the change. Avoid unrelated cosmetic
refactors and do not modify production rendering or CMake.

## Checks and owner handoff

Add focused probes or assertions for the accepted algorithm. Inspect error paths,
empty scenes, repeated resources, multiple submeshes and several frames in flight.
Document tests added separately from tests actually run. Do not compile without
explicit authorization.

Native visual/lifetime validation remains pending; no further measurement is a
prerequisite for the owner's approved implementation. Do not stack unrelated Metal
binding optimizations into this change.

## Acceptance

- Implementation matches the approved change list with no hidden scope expansion.
- Measured operation counts validate removal of the targeted work.
- Owner data supports the improvement; behavioral checks remain pending until run.

## Required updates on completion

Update this file and `summary.md` with code paths changed, before/after numbers,
actual checks and remaining limitations. Revise Step 05 based on the implementation.

### Execution record

- Approval reference: Owner's chat instruction to implement the seven presented
  steps, including removal of redundant C++ and retention of relevant profiling.
- Files changed: SoftShadowPass and TracingSceneResources headers/implementations,
  both soft_shadows shaders, shadow/GI/capacity probes, API/profiling documentation
  and plan records. No production render, backend or CMake changes.
- Checks: Source-level range, ownership and native binding review; static Python
  assertions for removal of obsolete paths, paired shader bindings and equivalent
  indexed UV arithmetic across meshes/submeshes. `git diff --check` passed. Probes
  updated but not executed; no C++/shader compilation or native runtime execution.
- Before/after measurements: Historical ~23.5 ms packing and ~18.25 MB UV upload
  per preparation are the removed operations, not a measured new FPS result.
- Next action: Owner build with regenerated native kernels and visual/lifetime
  checks from Step 05. Other Metal resource-preparation costs remain out of scope.
