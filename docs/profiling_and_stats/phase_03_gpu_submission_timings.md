# Phase 03 GPU submission timings

Status: implemented; native acceptance pending. See [implementation and verification record](phase_03_implementation.md).

## Outcome

Performance displays optional GPU execution durations for completed submissions, and recordings contain correlated GPU samples. Vulkan and Metal have working implementations or explicit capability-based unavailable results. No GPU wait is added for measurement.

## Scope and implementation

1. Define optional timing capabilities and engine-owned result/request types in `bg2e::gpu`. Identify device, queue, command recording, submission, capture epoch, recording session and presentation/scene frame. The GPU module must not depend on draw, UI or scene types; callers supply opaque numeric identities.
2. Integrate timing requests with native command recording/submission and existing completion records. Allocate bounded timing storage independently per outstanding recording/submission. Reuse it only after normal completion/retirement proves safety. Aborted or failed submissions must release bookkeeping without waiting on work that never executed.
3. Implement Vulkan timestamp queries around the supported command workload. Check queue timestamp support, valid bits, timestamp period, wraparound and query availability. Reset/reuse queries safely with the device's supported facilities. Read only available/completed results without a WAIT query flag or a new fence/device wait.
4. Implement Metal total command-buffer timing using completed native command buffer start/end times. Retain native ownership until the result is resolved or safely retired. Handle errors and invalid/missing native timestamps explicitly; CPU time around `commit` is not GPU time.
5. Retrieve results during normal nonblocking completion processing or safe slot reuse. Let samples arrive several frames late without relabeling them as the current frame. Bound pending requests and expose dropped/unsupported/failed result counts. Capture stop, device replacement and shutdown must not reuse a pending query for another session.
6. Associate requests with frames in the draw render loop without introducing a GPU-to-draw dependency. Account for multiple submissions and distinguish scene, UI and auxiliary work where known. Keep uploads and queue latency separate from execution duration.
7. Publish plain completed samples to the profiler/snapshot layer through a consumer outside the GPU module. Add GPU charts and recording records in the same phase. Keep CPU-only functionality unchanged when timing is unsupported or disabled.
8. Report per-submission/per-queue values first. If an aggregate frame metric is provided, define its interval/overlap policy precisely; never sum overlapping queues or silently claim presentation time is GPU execution time. Keep GPU clocks separate from CPU wall-clock timestamps unless explicit calibration is implemented.
9. Respect recording boundaries: tag at request time; do not attach pre-start or post-stop work to the file. At Stop, record unresolved counts and close normally without waiting for outstanding GPU work. Later completions may update eligible live history but never reopen the old file or enter a new session.

## Compilation boundary

- Add public declarations, default unsupported behavior and complete Vulkan/Metal definitions together. Include platform-disabled/backend-unavailable configurations in the audit.
- UI and recording consume only published engine-owned samples, never native query or command-buffer objects.
- No public header may include ImGui or expose its types; any required UI functionality must use `bg2e::ui` wrappers implemented in source files.
- The submission-timing feature must work without phase 04 markers or pass-level APIs. Unsupported timings are unavailable, not zero-valued success or throwing default methods on normal render paths.
- Preserve all existing submission/completion synchronization and cleanup contracts.

## Verification and exit

Check known GPU workloads, presentation/scene association, multiple queues/submissions, delayed results, out-of-order publication, aborted recordings, failed submissions, query exhaustion and unavailable capabilities. Exercise long GPU latency, slot reuse, resize, renderer/device replacement, recording restarts and shutdown.

Compare measured intervals with native graphics tooling on available hardware and document the measurement domain. Confirm no additional device drains, blocking query reads or queue completion waits. Validate that unsupported devices retain functional CPU charts and recording.

Build and run available Vulkan and Metal configurations when explicitly authorized; mark unavailable backends and unrun validation as gaps. Exit with GPU submission charts and recordings working independently of pass-level profiling.
