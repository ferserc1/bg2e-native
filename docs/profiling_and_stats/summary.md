# Profiling and performance statistics implementation plan

Status: phases 01–04 implemented in source; native acceptance and overhead measurements pending. No native compilation, GPU validation or performance measurements are claimed. See [phase 01 implementation and verification](phase_01_implementation.md), [phase 02 implementation and verification](phase_02_implementation.md), [phase 03 implementation and verification](phase_03_implementation.md), and [phase 04 implementation and verification](phase_04_implementation.md).

## Objective

Replace profiling controls in graphics settings with a reusable `bg2e::ui::Performance` window. Display live CPU statistics and, when supported, GPU timings without requiring a profile file. Let the user record a new capture to a chosen file through Start Recording and Stop Recording.

Preserve the existing rendering optimizations. Measurement must not add device-wide drains, synchronous GPU readback or per-frame file flushing on the rendering thread.

## Planning baseline

This inventory predates phase 01; current behavior is documented in its implementation record.

- `lib/include/bg2e/base/CpuProfile.hpp` and `lib/src/bg2e/base/CpuProfile.cpp` provide process-wide recording control, thread-local CPU scopes/counters and text output. Capture currently depends on an open output file. Reports aggregate approximately two seconds of frame calls.
- `lib/src/bg2e/ui/GraphicsSettingsWindow.cpp` owns the current profiling controls. Removing them there affects all consumers of the shared graphics settings window, including the draw composer.
- `apps/draw/bg2e_composer/src/EditorUi.hpp` and `.cpp` integrate editor windows and menus. There is no existing `ui::Performance` class.
- `bg2e::app::FileDialog` already provides a save-file dialog.
- `bg2e::gpu` has Vulkan/Metal command submission and completion ownership, but no public GPU timestamp profiling facility was found during the initial audit.

Recheck these locations during implementation; this inventory describes the planning baseline.

## Non-negotiable requirements

1. **Every phase must leave the library and affected applications compilable and usable.** Implement declarations, definitions, backend variants, consumers and cleanup together. Do not leave stubs needed to link the current feature or require a later phase to restore working functionality.
2. **No public header may include an ImGui header or expose an ImGui type.** Keep all ImGui calls/types in implementation files. Add engine-owned wrappers under `bg2e::ui` where applications or reusable widgets need new functionality. The Performance public API uses engine and standard C++ types only.
3. Keep CPU collection, live history and file recording independent. Live charts must work without creating a file. Existing scope instrumentation and counter semantics remain usable.
4. Keep `bg2e::base` independent of UI/application/rendering layers. Backend profiling APIs belong to `bg2e::gpu`, with no dependencies on `draw`, `render`, `scene` or `ui`. `draw` supplies frame/pass context; `ui` consumes snapshots.
5. Do not modify or migrate the production `bg2e::render` API. This work integrates with the existing draw path and experimental GPU abstraction.
6. All histories, label registries, publication queues, writer queues and GPU query storage must have explicit limits. Report dropped, incomplete and unsupported samples; never represent them as valid zero timings.
7. Profiling failures must not interrupt rendering. Keep disabled overhead small and measure capture overhead separately from the workload being inspected.
8. Do not modify CMake or add external plotting dependencies without a separate explicit request. Use existing source discovery and internal ImGui drawing facilities.

## Phases

| Phase | Working delivery at phase end | Depends on |
| --- | --- | --- |
| [01 CPU collection and recording](phase_01_cpu_collection_and_recording.md) | Structured CPU samples, bounded in-memory snapshots and independent file recording; existing callers still work | Existing CPU profiler |
| [02 Performance window](phase_02_performance_window.md) | Live CPU charts, recording workflow and composer integration; old profiling UI removed | 01 |
| [03 GPU submission timings](phase_03_gpu_submission_timings.md) | Optional asynchronous GPU timings in both backends, charts and recordings | 01 and 02 |
| [04 GPU pass timings and acceptance](phase_04_gpu_pass_timings_and_acceptance.md) | Supported pass breakdowns, explicit capability fallback and end-to-end validation | 03 |

Implement in this order. Each phase must provide a complete usable increment, including unsupported-backend behavior, documentation and focused verification cases. CPU-only Performance is a complete feature at phase 02; phase 03 must not depend on phase 04 to function.

## Shared data and interpretation contract

- Use a monotonic CPU clock. Identify capture epoch, recording session, thread, presentation frame, scene frame, device, queue and submission where applicable. Unknown/unassociated work stays explicitly unassociated.
- Distinguish presentation cadence, scene cadence and CPU work duration. UI-only frames, paused scenes and scene averaging must not inflate scene FPS or change per-scene-frame denominators.
- CPU scopes publish inclusive duration, exclusive duration, call count and maximum; preserve hierarchy or stable parent identity where needed. Do not add nested inclusive totals or concurrent thread totals to obtain frame time.
- Counters retain units and aggregation semantics. Calls, pixels, bytes, allocations and drains are not interchangeable with durations. Window statistics use the actual retained interval and valid sample count.
- GPU results carry their original frame/session identity even when resolved later. Display execution duration separately from CPU submission, queue latency and presentation interval. Overlapping submissions/queues must not be summed into a fictitious frame GPU total.
- Snapshots own their data. Readers never inspect mutable thread-local accumulators or native backend resources. Stopping/restarting a recording does not invalidate live chart history.

## Recording contract

Start Recording opens the native save-file dialog. Cancel changes nothing. Recording becomes active only after a destination is accepted and successfully opened; the button then becomes Stop Recording. Capture starts at that successful start boundary and never exports older live history. Scopes/frames crossing the boundary are explicitly clipped, excluded or marked partial according to one documented policy.

Stop Recording establishes an end boundary, drains already accepted CPU/file records, writes a summary and closes the file. GPU samples still unresolved at that boundary are counted as unresolved and are not waited for or written into a later session. File writer shutdown may finish ordinary file I/O, but must never wait for GPU completion. Live graphs continue after recording stops. Closing Performance does not silently stop an active recording; reopening reflects its current state. Application shutdown finalizes recording before profiler services are destroyed.

Use a versioned JSON Lines format with session metadata, frame/scope/counter records, optional GPU records and a terminal summary. The summary reports omissions, partial boundaries, queue overflow and unresolved GPU samples. File I/O errors return the UI to a non-recording state with a visible error. Define overwrite confirmation and avoid silently appending unrelated sessions.

## Validation and completion records

For every phase, record exact changes, source checks, native checks actually run, backend coverage, capture overhead and remaining gaps. Supply focused probes using the repository's existing verification style; do not introduce a general test framework solely for this work.

The exit gate is a successful library build and affected application build on available supported configurations, plus phase-specific functional checks. If compilation/execution has not been explicitly authorized, do not run it: record the gate as pending instead of claiming success. An implementation phase is not accepted as build-verified until those checks have actually passed. Unavailable platforms/backends remain documented gaps.

Exercise disabled/live/recording modes; multiple threads; aborted frames and submissions; rapid start/stop; Cancel and file errors; bounded-memory pressure; UI-only frames; resize; scene/renderer replacement; long GPU latency; and shutdown. Compare the same settled workload with profiling disabled and enabled. Separate cold start, transitions and steady state, and never add measurement waits to obtain results.
