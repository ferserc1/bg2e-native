# Draw renderer optimization implementation plan

Status: planned. Each phase delivers a functional application with an independently verifiable optimization. No phase ends with an unfinished shader interface, disabled feature, or infrastructure that requires the next phase to restore functionality.

This plan extends the [existing optimization report](../render_to_draw.md). Snapshot-local tracing binding reuse and indexed GPU shadow UV lookup are already implemented; their native validation remains an entry requirement for this work. Existing profile numbers are CPU measurements from short Metal captures, including settings transitions. They establish resource preparation as a priority, but do not establish GPU speedups or reliable steady-state frame times.

## Requirements

- Objects remain independent runtime import/export entities. Their editable materials, resource ownership, and export dependencies must not become dependent on a scene material library.
- Cache sharing is internal and derived. It must never merge editable materials by value, introduce implicit object/material instancing, or make edits propagate between independent entities. Distinct objects may reuse immutable GPU representations while retaining separate source identities and exportable resource ownership.
- Renderer table indices are transient addresses, never serialized material identities. Import into another scene reconstructs derived resources from the object's own data. Removal from one scene must not invalidate another object's resources or submitted work.
- `isTransparent` selects raster composition layers. Alpha-test opacity independently determines ray-intersection filtering. A transparent material can have opaque RT instances if explicitly admitted to tracing. Preserve the existing default exclusion of transparent objects from tracing.
- Define one named CPU constant, proposed as `AlphaTestCutoffThreshold`, initially `0.02f`, with a comment explaining that slider values close to zero disable alpha testing. Cutoff below the constant disables alpha test; equality enables it. Preserve the authored value, and pack a zero effective cutoff when testing is disabled. Update existing draw shaders to use the effective classification consistently, without duplicating the numeric threshold in shader source.
- New material defaults use cutoff zero. glTF `MASK` imports use exactly `0.5`, including files with a different authored cutoff; other glTF modes use zero. Explicit cutoff values in other supported formats remain explicit values, subject to the effective threshold.
- Preserve layout/capacity distinctions, texture admission and overflow fallbacks, UV0/UV1 behavior, dense TLAS mapping, standalone passes, acquired-slot ownership, and Metal indirect-resource residency.
- No CMake changes. Native builds, shader compilation, and application execution are validation tasks to run only when the user explicitly requests them. A phase is not runtime-validated until those checks actually run.

## Phase sequence

| Phase | Functional optimization delivered | Dependency |
|---|---|---|
| [01 Alpha classification](phase_01_alpha_classification.md) | Near-zero cutoff materials use the opaque RT path and skip alpha filtering | Existing shadow v3 validation |
| [02 Material revisions and raster bindings](phase_02_material_revisions.md) | Unchanged material packing and raster texture bindings are reused | 01 |
| [03 Tracing table construction](phase_03_tracing_tables.md) | Indexed resource lookup and one derived material entry per material identity | 02 |
| [04 Shadow arrays and Metal preparation](phase_04_shadow_bindings.md) | Stable shadow arrays are prepared once per snapshot/layout rather than per composition | 03 |
| [05 Persistent material resources](phase_05_persistent_material_resources.md) | Unchanged material and texture tables survive across frames | 04 |
| [06 Scene acceleration reuse](phase_06_acceleration_reuse.md) | Unchanged TLAS and instance data are reused across scene frames | 05 |
| [07 Temporary resource pools](phase_07_resource_pools.md) | Retired effect images, buffers, and bindings are reused | 06 |
| [08 Temporal invalidation](phase_08_temporal_invalidation.md) | Unchanged scene signatures are reused without weakening history validity | 02 and 07 |
| [09 Transparent processing regions](phase_09_transparent_regions.md) | Transparent layers limit eligible raster, effect, and composition work to conservative screen regions | 07 and 08 |
| [10 Update synchronization](phase_10_update_synchronization.md) | Proven data-only updates avoid unnecessary device-wide drains | 02 through 09 |

Implement in the listed order. Supporting tasks belong to the phase that first uses them; do not introduce an unused replacement architecture. Keep the previous complete path available as a diagnostic fallback where a change affects resource lifetimes or image coverage.

## Validation and acceptance

Before phase 01, validate the existing library/shadow v3 interface and indexed UV lookup on supported Metal and Vulkan configurations. Cover nonzero geometry bases, submeshes, UV selection/scaling, barycentrics, alpha holes, overflow, empty scenes, standalone shadows, slot reuse, and aborted recording. Capture reference images and diagnostics. If a backend is unavailable, record that gap rather than marking it passed.

Use the existing draw examples, especially `examples/draw/10_deferred_renderer`, together with focused existing probes where available. Add focused verification cases where needed rather than introducing a general test framework. The common scene matrix includes static opaque scenes, masked materials, layered transparencies, animated transforms, material edits, shared source textures, and independent object import/export. Exercise shadows, AO, GI, and reflections in their valid configurations: AO and GI are alternative indirect-lighting modes in the current renderer.

Every phase must demonstrate:

1. Functional rendering, settings changes, resize, scene replacement, cleanup, and standalone use where affected.
2. Correct output against the previous phase, except for the explicitly approved phase 01 alpha/default/import changes.
3. No validation errors, stale resources, partial publications, or mutation of data still used by submitted work.
4. Independent object round trips: export object A, remove it, import it into another scene, edit its material, and verify that object B and its materials remain independent. Include resource sharing, source-scene destruction, and clone/edit cases.
5. A phase-specific reduction in work, using counters as well as timings. Do not accept a cache that only moves equivalent reconstruction elsewhere.

Record backend, resolution, object/material/texture counts, occupied versus allocated table capacity, transparent layer count, active effect context, cache misses, allocations/uploads, and wait initiators. Separate cold start, warm steady state, and transition frames. Compare exclusive CPU timings without adding nested totals. Measure GPU execution separately when available; do not add measurement waits to the rendering loop.

## Completion records

For each phase, record implementation status, exact changes, static checks, native checks actually run, visual comparisons, work counters, timing evidence, and outstanding backend gaps. Keep the original report as historical evidence. This plan does not claim that its proposed optimizations have already been implemented or measured.
