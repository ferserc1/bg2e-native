# Phase 01 CPU collection and recording

Status: implemented; native acceptance pending. See [implementation, audit and verification record](phase_01_implementation.md).

## Outcome

The existing CPU instrumentation produces structured live statistics without an output file. File recording is an independent subscriber. The library and current applications remain compilable and retain a working legacy recording entry point.

## Scope and implementation

1. Audit `CpuProfileScope`, `CpuProfiler`, frame boundaries in the draw render loop and non-frame/worker instrumentation. Document existing inclusive/exclusive timing and counter meanings before refactoring. Preserve nesting, `next()` and exception-safe scope destruction.
2. Define public engine-owned sample/snapshot types under `bg2e::base`. Include monotonic timestamps, capture/thread/frame identities, parent or hierarchy identity, durations, calls, maximum and counters. Own copied labels or intern stable label IDs with an explicit cardinality limit. No UI, ImGui or native GPU types belong in these headers.
3. Separate collection enablement from recording state. Give live consumers an explicit lifetime/registration mechanism so closing one window cannot disable another consumer or an active recording. Preserve compatible `start(path)`/`stop()` entry points or migrate all existing callers in this phase. Define `enabled()` unambiguously and provide a separate recording-state query.
4. Replace two-second text reports as the primary data path with frame publication. Add presentation/scene-frame correlation where known, preserving unassociated worker samples. Use per-thread bounded accumulation and short publication operations; no file I/O or formatting in scope destruction. Publish coherent owning snapshots for readers. Handle thread exit, no-frame work and interrupted frames explicitly.
5. Store a bounded circular history and derive table/window aggregates from valid samples. Bound retained time/frame count, labels and publication backlog. Report evictions and drops. Do not use a thread's report-call count as a universal frame denominator.
6. Implement independent recording with a versioned JSON Lines writer and an explicitly bounded queue. Move formatting/write/flush work off the render thread. Specify overflow behavior and counters. A disabled recorder must neither create files nor enqueue file work.
7. Implement the start/end boundaries in the shared summary. Give recording its own session ID, independent of the live collection epoch. Do not mix pre-start aggregates, scopes from an earlier session or old queued writer records. Document the partial-scope policy and flush accepted partial aggregates at stop rather than losing the final report interval.
8. Make file opening transactional: on failure keep live capture and any existing valid recording state coherent. Define rejection of a second start while recording, overwrite handling, write-error recovery and clean writer termination. Stop must close the file after accepted records are processed; do not introduce GPU waits.
9. Retain application context metadata through a safe provider lifetime contract. Avoid invoking application callbacks while holding profiler publication/output locks. Shutdown must detach providers before their owners die and stop the writer safely.

## Compatibility and compilation boundary

- Update `CpuProfile.hpp` and `.cpp` together, plus every caller affected by changed semantics. Existing graphics profiling controls must still start/stop a valid capture until phase 02 replaces them.
- New declarations have definitions and are exported consistently across supported platforms. Avoid new build-system dependencies.
- This phase does not require Performance or GPU profiling types to compile or link.
- No public ImGui includes or types are permitted.

## Verification and exit

Verify scope nesting and sequential stages, known-duration samples, per-thread separation, counter aggregation and worker shutdown. Check presentation versus scene denominators, aborted frames, paused/UI-only frames, empty intervals and window statistics after eviction.

Exercise live capture without any file; recording without a live window; rapid start/stop/restart; boundary-crossing scopes; files that cannot be opened/written; writer overflow; final partial interval; and shutdown. Parse the resulting file and verify session isolation and terminal omission counts.

Measure disabled and enabled collection overhead and memory bounds without adding workload waits. Build the library and affected applications when explicitly authorized. Record unrun native checks as pending. Exit with a working CPU capture/recording service and unchanged rendering behavior.
