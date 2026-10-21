# Phase 02 Performance window

Status: implemented; native acceptance pending. See [implementation and verification record](phase_02_implementation.md).

## Outcome

`bg2e::ui::Performance` is a reusable ImGui window with live CPU charts and complete recording controls. The draw composer exposes it, and the shared graphics settings UI no longer owns profiling controls. CPU-only operation is complete and does not depend on later GPU phases.

## Scope and implementation

1. Add `lib/include/bg2e/ui/Performance.hpp` and its implementation, following `ui::Window` lifecycle/visibility conventions. Expose only engine and standard C++ types. Use private implementation storage where required and include the public class in the UI umbrella header.
2. Keep every ImGui include, call and type in `.cpp` implementation files. If plotting or layout capabilities must be shared with applications/widgets, add `bg2e::ui` wrappers with engine-owned public data types. Applications must never call ImGui directly through this feature. Start with existing ImGui plotting/drawing facilities; no new plotting dependency or CMake changes.
3. Read owning profiler snapshots. Acquire live collection while the window is open; release that consumer on close/destruction. Recording retains capture independently. Do not render hidden windows merely to keep the profiler alive.
4. Provide time-series charts for presentation interval/FPS, scene cadence and CPU frame work; selectable CPU stages; and selected counters with their units. Add a stage table with exclusive/inclusive time, maximum and calls. Show the retained time range and sample counts. Limit series/point counts and reuse buffers to keep graph rendering costs bounded.
5. Distinguish waits from CPU work and avoid adding nested/concurrent totals. Show missing/unassociated samples as gaps or explicit unavailable values. Include pause-display and clear-live-history controls only if their semantics remain separate from collection/recording. Clearing charts must not truncate an active recording.
6. Implement Start Recording through `app::FileDialog`. Cancel leaves button, file and session state unchanged. Handle opening errors visibly. Switch to Stop Recording only after successful start; display output path, duration and errors. Stop finalizes/closes the file and restores Start Recording while live charts continue.
7. Treat file selection/modal-dialog time and partially captured frames according to phase 01's boundary policy. Prevent duplicate starts and reentrant stop actions. Reopening the window must show an ongoing recording correctly. Ensure application shutdown finalizes recording even if the window is hidden.
8. Integrate a Performance window instance and menu entry into `apps/draw/bg2e_composer/src/EditorUi.hpp` and `.cpp`, using the existing ownership and window draw paths. Keep the widget independent of composer-specific state so other applications can adopt it.
9. Remove the profiling block, fixed-home-directory filename logic and obsolete includes from the shared `GraphicsSettingsWindow.cpp`. Audit its diagnostics subclass and other current consumers so duplicate recording controls disappear without removing unrelated graphics/debug settings or frame-rate policies.

## Compilation boundary

- Deliver the window declaration, implementation, wrappers, umbrella include, menu entry and lifecycle wiring together.
- Remove old controls only when the replacement recording workflow is working in the same phase.
- Do not call GPU APIs planned for phase 03. GPU charts may be absent or show a clearly described unavailable state.
- Inspect the transitive public include graph, not only `Performance.hpp`, for ImGui leakage.

## Verification and exit

Open/close/reopen and resize/dock the window. Verify graphs advance without a file, remain bounded, distinguish scene/UI-only frames and display valid units. Check that observer overhead is visible and does not introduce unexpected allocations per sample.

Exercise Cancel, valid recording, permission/disk errors, repeated clicks, Stop, hidden-window recording and application exit. Verify file contents start at the accepted recording boundary and contain the final summary. Confirm graphics settings no longer expose profiling controls in the composer or other shared consumers.

Build the library, composer and other affected consumers when authorized. Run the CPU-only workflow on available platforms and audit public headers for ImGui includes/types. Record compilation and UI checks not actually performed as pending. Exit with a complete standalone CPU Performance feature.
