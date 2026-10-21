# Phase 02 implementation and verification record

Status: implemented; native acceptance pending. No compilation, native probe
execution, UI execution, backend validation or overhead benchmark was performed.

This records the phase 02 CPU-only increment. The subsequent
[phase 03 delivery](phase_03_implementation.md) adds optional GPU submission timing.

## Delivery

- Added public `bg2e::ui::Performance`, its private implementation and a CPU-only
  private presentation model. The public declaration includes Window, common and
  standard headers only. `ui/all.hpp` exposes it. All new ImGui operations and
  types are confined to Performance.cpp; no plotting dependency was added.
- Added Window > Performance in the draw composer, with the existing weak-owner
  menu callbacks, normal UI draw path and tool cleanup. The window starts closed.
- Removed the profiling block and application/home-filename dependencies from
  GraphicsSettingsWindow. RendererDebugWidget inherits that removal; renderer
  settings, diagnostics, import/export and frame-rate policies remain available.
- Added coherent `CpuProfiler::recordingStatus()` for active/session/path/error
  and monotonic start/end timestamps, including asynchronous writer errors.
- Added optional `snapshot(since)` filtering by sample start, before deriving
  totals. The default remains the full retained snapshot. The cutoff belongs to
  a reader; shared history and writer admission are never cleared.
- Made Window::open()/close() virtual so Performance's lease lifecycle also works
  through Window references. Other windows retain their existing implementations.
  Applications/libraries must be rebuilt together for this public vtable change.

The existing phase 01 changes remain in the working tree. CMake, production
`bg2e::render`, GPU backends and frame-rate policy code were not modified by
phase 02. The repository's referenced `skills.md` remains absent; the supplied
AGENTS.md and existing CPU profiling architecture documentation were available.

## Collection and display lifecycle

Performance acquires exactly one Collection lease on open and releases it on
programmatic close, title-bar close or destruction. Its closed draw path performs
no ImGui work or snapshot acquisition. Collapsed/docked inactive windows remain
open consumers, but their contents are not rendered. Closing or destroying one
Performance instance never stops a recording or another consumer.

Reopening reads the process-wide recording status. If closing the final consumer
ended collection, a new epoch is preferred on reopen: old retained frame streams
cannot seize the new default selection before the first new marker arrives. An
active recording keeps the epoch/history continuous. MainLoop's phase 01 cleanup
still joins producers and finalizes recording before graphics teardown, whether
Performance is visible or hidden; composer cleanup also closes its live consumer.

Snapshots refresh at most once every 250 ms while contents are visible. Snapshot
failures leave rendering/recording running, show unavailable data and retry after
one second. Pause
Display freezes the owning snapshot and chart timeline; recording controls and
recording duration/errors still update. Clear Charts advances only this window's
cutoff and clears its display, including while paused. It never truncates a file,
resets a recording ID, changes collection enablement or clears another window.
Scopes crossing the clear cutoff are excluded from this reader's next snapshot.

## Charts and interpretation

Eight fixed series have at most 600 time bins each: presentation interval/FPS,
scene interval/FPS, CPU frame wall time, selected stage inclusive/exclusive time
and selected counter values. Positions use monotonic timestamps over the current
retained completion window (at most 30 seconds for charts). Empty bins are NaN
and display gaps/unavailable text; genuine sampled zeros remain valid points.
Tooltips report values, event count and position relative to the chart end.

A frame stream is a single thread and collection epoch. Presentation intervals
require consecutive successful presentation numbers. Scene intervals require
consecutive scene-frame numbers and scene-rendered markers; presentation-only
frames do not add scene samples. Incomplete attempts reset cadence continuity.
First markers and missing sequences cannot manufacture intervals or zero timings.
Cadence is CPU frame-completion cadence, not compositor/scanout timestamps.

CPU frame time uses only the scope whose ID matches a complete frame marker's
unique token. Nested scopes and concurrent threads are never summed. Missing
root scopes or frame markers leave gaps. It is inclusive CPU wall time, with
waits and driver calls; it is not utilization or inferred GPU execution. Wait
rows are identified by known label spelling (`wait`, `Wait`, `nextDrawable`),
which is a display aid rather than an exhaustive blocking-time classifier.

Stage choices include epoch, thread, label and unit. Stage bins average per-call
inclusive/exclusive values. Table rows show retained exclusive/inclusive totals,
maximum inclusive duration per call and calls, separately per thread/epoch. Rows
prioritize the current epoch, then exclusive total. The table retains raw
modal/partial-scope data; its totals are not divided by arbitrary frame counts.

Counter choices preserve units and thread/epoch identity. Counter bins sum event
values, with no implicit frame normalization or fabricated zero buckets. Selected
counter details retain exact integer total, maximum/event and event count; chart
values use floating-point display precision. Unassociated samples are counted
explicitly and may appear in their own stage/counter charts without being assigned
to a presentation frame.

Selectors/table are capped at 256 stage rows and 256 counter rows. Frame stream
choices are capped at 64 recent streams, reporting excluded frame records. Other
selection omissions, history evictions, collection drops, writer drops and partial
boundaries are visible. Scratch indices/frame descriptors reuse capacity bounded
by phase 01's 65,536 retained CPU events. Binning uses fixed arrays, not per-event
plot allocations. Owning snapshot copies/aggregation still allocate at refresh
cadence, and UI labels/ImGui geometry have their ordinary bounded display costs.

The display reports its last snapshot/model refresh duration and previous draw
duration. `ui.performance`, `ui.performance.snapshot` and `ui.performance.series`
scopes expose observer cost in normal collection/recording. The draw measurement
excludes native save-dialog time; no overhead result is claimed without execution.

## Recording workflow

Start Recording calls the existing native FileDialog with a JSON Lines filter.
No file, recording ID or recording button state changes on Cancel. A missing
extension becomes `.jsonl`. Existing destinations require an explicit Replace
confirmation; cancellation never opens/truncates them. New destinations use phase
01's exclusive creation, so a destination appearing after the existence check is
rejected safely. Repeated/reentrant actions are guarded; an existing process-wide
recording is also protected by the service's second-start rejection.

Only successful start activates recording. Path, session, duration and file/dialog
errors are visible. Stop drains accepted CPU/writer data and closes the file via
the service, then the Start action becomes available; live charts continue.
Ordinary file-writer shutdown I/O may complete synchronously during Stop. There
are no GPU queries, waits or phase 03 API calls.

Native save-dialog intervals are retained in a fixed ring of 16 intervals and
excluded from chart scopes/cadence edges, including Cancel/error intervals. Raw
table data and recording keep phase 01's boundary policy: scopes crossing the
successful start/stop boundary are excluded from that recording; eligible child
stages may remain. Recording starts after native selection/overwrite acceptance,
so modal selection time is never exported as pre-start scope data. An ImGui
Replace prompt keeps normal UI frames running and does not masquerade as a
blocked native dialog. If the interval ring wraps, the ambiguous older prefix
is suppressed conservatively instead of forgetting a possible modal stall.
This conservative suppression contributes to the displayed chart omission count.

## Verification actually performed

- `git diff --check` and source-level licensing/delimiter checks for new C++ files.
- Recursive engine-header include/type audit: Performance.hpp reaches 3 headers;
  ui/all.hpp reaches 154. No ImGui includes or code types were found in either
  transitive engine-header graph (comments are excluded from the type check).
- Source searches confirmed removal of profiling controls/fixed-home filenames
  from shared graphics settings, preservation of unrelated controls and composer
  menu/draw/cleanup wiring.
- Public declaration/definition and local vendored ImGui API review. No compiler
  or native executable was invoked. These checks do not prove compilation or
  runtime UI behavior.

`performance_data_probe.cpp` is a focused CPU-only model probe, outside automatic
CMake discovery. It covers presentation-only frames, missing/incomplete markers,
separate threads, valid zero counters versus gaps, counter units/bin sums, native
modal suppression, epoch defaults on reopen, row/stream limits and modal-ring
pressure. Once compilation is authorized, macOS/Linux can run it independently:

```sh
c++ -std=c++20 -Ilib/include -Ilib/src \
  docs/profiling_and_stats/performance_data_probe.cpp \
  -o /tmp/bg2e-performance-data-probe
/tmp/bg2e-performance-data-probe
```

The probe has not been compiled or executed. On Windows, use the configured engine
build/export definitions when adding an equivalent local probe invocation.

## Pending native acceptance

1. Build the library, composer and other shared-window consumers on available
   supported configurations; macOS native dialogs require the Xcode generator.
2. Open/close/title-close/reopen, dock, collapse and resize Performance. Repeat
   through a Window reference and with two consumers. Verify no file is needed
   for advancing graphs and that hidden windows do not render to retain capture.
3. Exercise paused scenes, presentation-only frames, aborted/missing acquisitions,
   multiple workers, empty/stale windows, clear while paused and selector pressure.
4. Exercise native Cancel, Replace Cancel/accept, successful Start/Stop, rapid
   clicks, a second Performance instance, existing/new/inaccessible destinations,
   write/close errors, hidden-window recording and normal/exception application
   shutdown. Reopen to inspect the same recording session and completed duration.
5. Parse actual files with validate_cpu_capture.py, confirming session boundaries,
   final counter batches and terminal summaries. Do not use `--probe` for arbitrary
   composer captures; that flag asserts the phase 01 probe's fixture labels.
6. Compare the same settled scene with Performance closed, live/open and recording.
   Measure CPU observer refresh/draw cost, enabled admission contention, graph
   geometry and memory pressure separately. No measured overhead or backend runtime
   coverage is claimed by this delivery.
