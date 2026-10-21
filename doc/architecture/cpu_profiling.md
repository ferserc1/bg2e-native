# CPU Performance diagnostics

Open **Window > Performance** in the draw composer. Live CPU charts need no output
file. Graphics Settings contains only graphics controls; profiling controls belong
to the separate Performance window.

Performance provides presentation interval/FPS, scene cadence, CPU frame wall time,
selectable stage inclusive/exclusive timings, a stage table and counters with units.
Frame streams and stage/counter choices retain thread and collection-epoch identity.
Scene cadence counts only scene-rendered frames; presentation-only frames do not
inflate scene FPS. Frame work includes driver calls and existing waits, rather
than CPU utilization or GPU execution. No GPU synchronization is introduced.
Missing/incomplete data is unavailable or a gap, never a fabricated zero timing.

**Pause display** freezes this window's snapshot while collection and recording
continue. **Clear charts** applies a reader-local cutoff; it does not clear shared
history, another window or a recording. Closing Performance releases its live
consumer, while any active recording continues independently. Reopening reflects
that process-wide recording. Applications finalize recording at shutdown through
the existing MainLoop cleanup path.

**Start Recording** opens the native save dialog for a versioned JSON Lines file.
Cancel leaves recording state unchanged. Existing files require explicit Replace
confirmation. Only a successfully opened destination activates recording.
**Stop Recording** drains accepted records, writes the terminal summary and closes
the file; live collection continues while Performance remains open. The window
shows the output path, session, duration and errors. Native dialog stalls are
omitted from charts; recording boundaries follow the scope-exclusion policy.

For another application, keep a `bg2e::ui::Performance` member, call `open()` from
its menu and `draw()` from its normal UI path. The widget starts closed and uses
only engine/standard types in its public header. It can also be included through
`bg2e/ui/all.hpp`. Lower-level consumers can independently retain a
`bg2e::base::CpuProfiler::Collection` lease and read owning `snapshot()` values.
`enabled()` indicates any collection consumer or recording; `recording()` indicates
file recording only.

Scopes retain inclusive/exclusive duration, calls, maxima and parent/thread/frame
identity; `next()` preserves sequential stages. Counters retain summed values,
event maxima and units. Do not sum nested inclusive times or concurrent threads
as frame time. Initial draw backend/extent/effect context is recorded on start,
not continuously sampled as configuration changes; compare settled configurations
in separate recordings.

History, publication batches, registered threads, copied labels, writer backlog,
chart bins and selectors have explicit limits. Writer formatting/I/O is asynchronous;
Stop may finish ordinary file I/O but never GPU completion. The window displays
drops, evictions, partial boundaries and its own observer cost.

Optional GPU submission timings now use Vulkan timestamp queries and Metal
completed command-buffer times. They are per-submission execution envelopes, not
frame totals or presentation/queue latency. GPU submission timing can be disabled
independently while CPU collection/recording continue. Results retain original
frame/session identities when arriving late; unsupported/missing timings are gaps
and explicit statuses. Fine-grained pass timing remains a later phase.

Native compilation, UI/hardware acceptance and
measured overhead remain pending.

See the [phase 01 record](../../docs/profiling_and_stats/phase_01_implementation.md)
for the baseline audit and collection/recording contract, and the
[phase 02 record](../../docs/profiling_and_stats/phase_02_implementation.md) for
CPU chart semantics and lifecycle details. The
[phase 03 record](../../docs/profiling_and_stats/phase_03_implementation.md) describes
GPU measurement domains, resource limits, late-session boundaries, schema v2, probes
and pending hardware acceptance checks.

## GPU pass intervals

Performance offers independent GPU submission, pass and detailed transparent
sampling switches. Pass timing uses bounded whole-pass intervals with invocation
IDs, parent IDs and object/submesh contexts. Select a GPU stage to chart its mean
per time bin; inspect individual statuses and granularity in the pass table.
Nested/overlapping intervals are never added. Missing/unsupported intervals leave
gaps, and total command timing remains available when pass sampling is unsupported.

Vulkan uses pipeline-boundary timestamp envelopes outside active rendering/compute
scopes. Metal uses runtime-supported timestamp counter sets at natural encoder
boundaries. Its encoder envelopes include intervening gaps and cannot attribute
individual dispatches; intervals containing unmeasured transfers are unsupported.
The chart uses the original CPU request time, not a calibrated GPU timeline.

New captures use JSONL version 3 with bounded `intervals` arrays inside
`gpu_submission` records. The validator retains version 1/2 support. Stop never
waits for unresolved intervals: they share their submission's session boundary.
See [phase 04 implementation and acceptance gaps](../../docs/profiling_and_stats/phase_04_implementation.md).
