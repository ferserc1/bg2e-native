# Persistent GPU deep debug logging

Composer accepts the optional flag in both Debug and Release:

```sh
bg2e_composer --backend metal --deep-debug
bg2e_composer --backend vulkan --deep-debug
```

The flag initializes `bg2e::base::Log` before graphics initialization. Without it,
no persistent file is created and trace payload lambdas are not evaluated.
`Log::isDebug()` and ordinary console filtering keep their existing meanings.
Existing warning/error chains retain console output and are additionally captured
as complete bounded records when their temporary logger is destroyed.

## File location and ownership

The output is `bg2e-deep-debug.log` beside the executable. The location starts at
`PlatformTools::applicationPath()`. For a macOS `.app`, Core Foundation resolves
the executable from bundle metadata, so the file is inside `Contents/MacOS`
beside the actual executable. `applicationPath()` itself is unchanged.

Initialization takes exclusive OS ownership through the fixed sibling
`bg2e-deep-debug.log.lock`. The empty lock file remains on disk; the kernel lock
is automatically released when the owning process exits, including an abort.
A competing instance fails before truncating the current log. An unwritable
location or other initialization failure terminates Composer startup with a
clear error and a nonzero exit code. There is no alternate output location.

Each successful initialization replaces the previous session. **Copy the log
before restarting Composer** if the previous run is needed for investigation.
Session metadata contains UTC start time, process ID, build debug policy and
limits. Clean exit explicitly closes the sink after graphics teardown. Native
callbacks after shutdown safely skip trace emission.

## Retention and failure behavior

The chronological text file has a hard limit of **12 MiB**, including its retained
session header. Each payload is limited to 8192 bytes; oversized messages are
marked `[truncated]`. Newlines in payloads become spaces so records remain intact.
Sequence numbers order records across threads; UTC timestamps and thread IDs
accompany backend/event fields. Resource and submission IDs are process-unique
identities, rather than reused native handles.

At capacity, compaction discards at least 512 KiB of oldest complete records.
It copies the retained tail in 64 KiB blocks to a fixed `.tmp` sibling, then
replaces the log under the separate ownership lock. Peak transient disk usage is
less than 24 MiB plus the empty lock file. There are no rotating archives. An
interrupted copy leaves the original log and at most one bounded temporary file;
the next compaction overwrites that temporary file.

Every emitted record is flushed before returning. This preserves useful evidence
through process abort without relying on final destructors or signal handlers.
It does not guarantee survival of power failure; there is no per-record `fsync`.
A write/compaction failure prints a direct stderr diagnostic, disables further
persistent tracing, and lets the application continue with existing graphics
error handling. Explicit shutdown still releases ownership after such a failure.

Already-active Vulkan validation warnings and errors are captured with message
ID, severity/type, and up to 16 named objects. A bounded cache of 128 complete
message payloads coalesces exact repeats, emitting the first occurrence and
cumulative counts at powers of two. Counts are checkpoints, not exact final
session totals. Cache eviction permits a message to start a new count. Truncated
messages are always emitted, avoiding suppression of distinct oversized errors.
Normal-mode validation console thresholds and validation activation are unchanged.

## Events and interpretation

Both backends trace device/queue/surface lifecycles, acquisition and presentation,
recording/pass boundaries, resource allocation, submissions, actual waits and
terminal results. Command summaries count draws, dispatches and copies; individual
draws, bindings and transitions are not logged. Submission events connect command,
surface, frame counter and slot identities. Offscreen surfaces have separate
identities/counters. Completion callbacks capture the original command/submission
IDs by value and do not retain application objects for logging.

Metal deep debugging requests command-buffer encoder execution error metadata
while preserving retained resource references. Completion errors include native
domain, code, description and up to 16 encoder labels/execution states with up to eight
signposts each, when Metal supplies them. Native queue/command/image/view/encoder names and internal
MetalFX texture names are published in Release as well. MetalFX logs generation,
configuration, slot ownership observations, encode entry/return and native failure.
FidelityFX logs initialization and dispatch entry/return with native SDK results.
Vulkan records native results and reports whether native naming is available.

An encode return means encoding returned; it does **not** establish completion of
asynchronous framework work. A missing return or the last event can guide an
investigation but does not prove causality. These tools cannot catch an exception
that aborts the process on Apple's private MetalFX queue. No waits, fences,
submission ordering, scaler policies, rendering results or validation activation
are changed by this feature. Additional metadata, synchronous logging and naming
have diagnostic overhead; measure it on the target workload.

The planning estimate remains 60–100 records/frame, approximately 24 KB/frame at
100 records averaging 240 bytes, or roughly 500 recent frames. Actual coverage,
resource loading and validation storms change retention substantially. **No
runtime volume or overhead measurement has been performed.**

## Focused verification

`tests/deep_debug/log_probe.cpp` is a standalone probe to link against the engine,
without adding a CMake target. Build it in both Debug and Release only when
compilation is authorized. Then run:

```sh
python3 tests/deep_debug/verify_log.py /absolute/path/to/log_probe
```

The driver uses isolated temporary directories and verifies disabled-mode lazy
payloads, concurrent complete records, repeated compaction, the size limit,
truncation, warning/error console capture, repetition counts, exclusive ownership,
startup failures, injected compaction write failure, flushed records surviving abort, ownership release after abort,
and late trace calls after shutdown. The probe intentionally aborts a child
process; the OS may create a normal crash report.

Additional authorized runtime checks:

- Exercise both Composer backends, offscreen rendering, resize and quit with work
  in flight; correlate completion events to original command/submission IDs.
- Exercise MetalFX and FidelityFX where available; inspect names and native
  errors using existing diagnostic procedures. Keep validation activation manual.
- Use a read-only executable directory to confirm explicit startup failure.
- Test exhausted disk space or injected stream failure to confirm stderr reporting,
  tracing deactivation, clean shutdown and continued ordinary console output.
- Measure records/bytes per frame and frame-time overhead with/without tracing on
  representative scenes. Record OS, device, backend, build and active validation.

Implementation verification in this session is limited to source review,
`git diff --check`, Python syntax/CLI checks and inspection of vendored Metal SDK
interfaces. Neither the engine nor the probe was compiled or executed. The
architecture reference `skills.md` was not present in this repository; the
architecture rules in `AGENTS.md` were used.
