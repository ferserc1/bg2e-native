# Phase 01 implementation and verification record

Status: implemented; native acceptance pending. No native build, execution,
backend validation or overhead measurement was authorized or performed.

This records the phase 01 increment. The later [phase 02 delivery](phase_02_implementation.md)
replaces its temporary graphics-settings recording UI with Performance.

## Baseline audit (step 1)

The audited implementation was `CpuProfile.hpp`/`.cpp`, the draw render loop,
GraphicsSettingsWindow and their instrumentation consumers under draw, gpu and
app. The repository references `skills.md`, but that file is absent from this
checkout; the supplied/root AGENTS.md and existing architecture documentation
were available.

Before this change:

- `enabled()` meant an open recording file. Scope construction was inert when
  disabled. `steady_clock` measured CPU wall time, including driver calls,
  scheduling and existing waits, rather than CPU utilization or GPU execution.
- A thread-local current-scope pointer established nesting. Inclusive time was
  the entire stage; exclusive time subtracted completed direct-child stages.
  Aggregation grouped by label, losing distinct parent relationships. `next()`
  ended a stage, charged its duration to its parent and began another stage with
  the same parent. Destructors caught aggregation failures during unwinding.
- Counters summed submitted values and retained the maximum individual value.
  They were not gauges or timing samples. Labels denoted operations, pixels,
  bytes, resource counts, allocations and drains, with no explicit unit field.
- `frame.total` began before validation/acquisition in `draw::RenderLoop::frame`.
  Early returns and exception unwinding still incremented the report-call count.
  Minimized windows, missing drawables, paused scenes and presentation-only
  frames therefore were not equivalent to scene frames. Context reported
  cumulative presentations and scene frames, but denominators used report calls.
- Only a scope created with the boundary argument caused a report. The sole
  such caller was `frame.total`. Render-thread initialization/non-frame work
  could join the next interval. Worker scopes and counters had their own TLS
  aggregates but no automatic publication or thread-exit flush.
- After about two seconds the render thread sorted/formatted/wrote/flushed text.
  Stopping invalidated the session and discarded its final partial interval.
  There was no session-wide counter store or terminal aggregate-counter dump,
  despite claims in the former architecture document. Labels were unbounded.
- The context callback borrowed the active render loop. Detachment did not wait
  for a callback whose function/data had already been copied by a reader.

## Delivered behavior

`CpuProfileCollection` (also `CpuProfiler::Collection`) is a noncopyable RAII live
lease. Multiple leases coexist with recording. `enabled()` means collection is
needed by at least one lease or recording; `recording()` is separate. Collection
epochs advance on a disabled-to-enabled transition. Recording IDs advance only
on successful recording starts and do not reset live history.

Owning snapshots contain scope/counter/frame records and totals grouped by
collection epoch, thread, kind, label and unit. Each scope event has calls=1,
inclusive/exclusive duration and maximum equal to its own duration; totals sum
calls/durations/values and preserve event maxima. No duration is divided by a
thread's frame count. Averages use totals/calls; presentation/scene rates must
use the retained timestamp interval and respective completed frame markers.
Never sum nested inclusive times or thread totals as elapsed frame time.

Scope IDs and parent IDs preserve invocation hierarchy, including sequential
`next()` stages. Frame correlation uses a unique frame token, with a terminal
frame record carrying the render loop's presentation number, scene-frame count,
scene-rendered flag and completion flag. A zero token means unassociated work.
Missing parents/markers are possible at recording boundaries or after eviction
or drops; consumers must not infer valid zero durations from them. Frame scopes
that leave before successful presentation emit incomplete markers. Workers stay
on their own threads; counter events inside a scope inherit its parent/frame.
Known byte, pixel, allocation and drain counters in draw now supply explicit
units; other existing counters retain the default occurrence unit `count`.

Existing graphics controls still record independently of any live consumer. Each
activation chooses a new `<app-id>-cpu-profile-<monotonic-ns>.jsonl` in the home
directory. The native save dialog and Performance window belong to phase 02.
Closing Graphics settings does not stop recording.

## Limits and publication

- At most 64 simultaneously registered instrumented threads. Excess threads drop
  events explicitly; expired registrations are reclaimed, not accumulated.
- At most 128 events per TLS batch. Publish at a frame/root-scope boundary,
  explicit `publish()`, thread exit, or on an event after 100 ms since the last
  publication. Counter-only producers that become idle should call `publish()`;
  no timer/background callback inspects mutable TLS data.
- Live history retains at most 512 immutable batches (65,536 events), evicting
  batches older than 30 seconds by completion timestamp during publication.
  Readers filter expired completions even when producers are idle. A long scope
  can begin earlier than the completion window; its duration is never clipped.
- Copied labels/units are limited to 127 bytes each; there is no permanent label
  registry. Distinct retained labels are bounded by event capacity. Session
  context is limited to 4,096 bytes. JSON escaping is byte-preserving (non-ASCII
  bytes are escaped individually); instrumentation labels are currently ASCII.
- The writer queue retains at most 256 batches (32,768 events), plus one batch
  being formatted. Overflow drops the newest recording batch and increments
  `writer_dropped`; live history continues. Memory allocation failures increment
  collection or writer drop counts rather than escaping into rendering.

Snapshot acquisition copies immutable batch handles under the publication lock;
copying records and deriving totals run after releasing it. TLS access and
admission/draining use a consistent publication-then-thread lock order. The
current implementation serializes enabled event admission and scope lifecycle
accounting with a process-wide mutex; contention and overhead must be measured,
particularly for many instrumented worker threads. Disabled scopes read no clock
and allocate no sample. No GPU synchronization was added.

## Recording and lifecycle policy

`start(path)` exclusively creates a new file; existing files are rejected.
`start(path, true)` explicitly truncates after caller-side overwrite acceptance.
A second start during recording is rejected and preserves the current recording.
Open failures preserve live collection. Opening is synchronous/transactional;
JSON formatting, buffered writes and final close run on the writer thread. File
errors asynchronously disable recording, preserve live leases and expose an
error string. Buffered failures may first become visible at writer close.

The successful start establishes its own monotonic boundary. Completed events
from earlier history/TLS are never exported. A scope crossing start or stop is
excluded from that recording, while completed child stages wholly inside the
session remain eligible. The summary's `partial` counts open scope segments at
start and stop (a segment crossing both is counted at each boundary). Such
scopes remain usable for live history. Frame markers crossing a boundary are
excluded; associated child records can therefore have no terminal frame record.

Stop serializes admission, establishes an end boundary, drains completed TLS
records, freezes omission counts and closes producer admission to this session.
Already queued batches are processed before a version-1 terminal summary and
close. Stop joins ordinary file I/O only; it never waits for GPU completion.
`dropped` counts collection admission/allocation failures during the session;
`writer_dropped` counts recording-specific overflow/allocation omissions. These
are distinct destinations and must not be added as unique lost events.
`unresolved_gpu` is zero because phase 01 has no GPU sampling.

The writer never invokes application callbacks. Initial context is copied on the
start caller's thread, outside publication/output locks, and is session metadata,
not a claim that settings remained constant for every frame. Callback replacement
and detachment synchronize with in-progress invocation through a separate mutex.
Providers must run on their owner's thread, must not reenter profiler lifecycle
operations or replace themselves, and must detach before the borrowed owner dies.
The render loop detaches on cleanup. MainLoop finalizes recording after joining
load workers and before graphics teardown, including its exception cleanup path.
A final service destructor also drains and joins the writer for standalone use;
standalone owners must join producers before process teardown.

## Focused verification

Source checks actually run: `git diff --check`, instrumentation/caller searches,
public header dependency review and Python validator syntax/synthetic-fixture
checks. CMake and production `bg2e::render` were not modified. No native probe,
engine build, application build or performance comparison has been run.

`cpu_profile_probe.cpp` is a standalone probe outside CMake source discovery.
After compilation is authorized, on macOS/Linux it can be built independently:

```sh
c++ -std=c++20 -pthread -Ilib/include \
  docs/profiling_and_stats/cpu_profile_probe.cpp \
  lib/src/bg2e/base/CpuProfile.cpp -o /tmp/bg2e-cpu-profile-probe
/tmp/bg2e-cpu-profile-probe /tmp/bg2e-cpu-profile-probe-output
python3 docs/profiling_and_stats/validate_cpu_capture.py --probe \
  /tmp/bg2e-cpu-profile-probe-output/first.jsonl \
  /tmp/bg2e-cpu-profile-probe-output/second.jsonl \
  /tmp/bg2e-cpu-profile-probe-output/recording_only.jsonl
```

Use a new output directory per run. The probe checks disabled/live/recording-only
modes, independent consumers, nested and sequential scopes, counters, worker
exit, incomplete/presentation-only/scene frames, failed open, second-start
rejection, session isolation, crossing scopes, final counters and bounded history.
It prints disabled/live workload durations as a smoke measurement, not a settled
renderer overhead benchmark. The parser checks JSON, identities, boundaries,
terminal counts and expected probe records.

Pending acceptance: run the probe and both available native builds; exercise
Windows exclusive file opening/exported symbols, Metal and Vulkan applications,
known workload durations, provider detach under concurrency, simultaneous
start/stop with workers, 64-thread admission pressure, timed history expiry,
slow-destination writer overflow, disk-write/close errors, abnormal shutdown,
resize/scene replacement and settled disabled/live/recording overhead. Cancel
and save-dialog acceptance are phase 02 checks. No backend runtime coverage or
measured capture overhead is claimed.
