# CPU profiling

**Header:** `<bg2e/base/CpuProfile.hpp>`  
**Namespace:** `bg2e::base`

The CPU profiling service collects bounded timing/counter history, associates
frames, and optionally records to a file. Include this header explicitly;
`base/all.hpp` does not currently include profiling headers.

## CpuProfileCollection

An independent, noncopyable RAII collection lease. Construct one for each live
consumer. Destruction releases that consumer's lease and never stops recording.
`CpuProfiler::Collection` is an alias. Collection is enabled while a lease or an
active recording exists.

## CpuProfileScope

`CpuProfileScope(const char* label, bool reportBoundary = false) noexcept`
starts a scope; destruction finishes it. Scopes are noncopyable and nest on the
current thread. Labels and counter units must have static lifetime.

`next(label)` finishes the current segment and begins another.
`frameCompleted(presentation, scene, sceneRendered)` marks a boundary complete;
call it only after successful presentation. Omitting it marks an aborted boundary.
Static `enabled()` and `count(name, value, unit = "count")` expose collection
state and counters. Scope instrumentation introduces no GPU synchronization.

```cpp
#include <bg2e/base/CpuProfile.hpp>

bg2e::base::CpuProfileCollection collection;
{
    bg2e::base::CpuProfileScope scope("Plugin discovery");
    // Work to measure.
}
auto snapshot = bg2e::base::CpuProfiler::snapshot();
```

## CpuProfiler

| Members | Purpose |
|---------|---------|
| `start(path, overwrite = false)`, `stop()` | Start/stop recording; `start` returns success and rejects existing files unless overwrite is allowed. |
| `enabled()`, `recording()` | Collection and recording state. |
| `session()`, `recordingSession()` | Collection epoch and independent recording identity. |
| `snapshot(since = 0)` | Reader-local time cutoff; does not clear shared history. |
| `recordingStatus()` | Coherent active/completed recording state. |
| `outputPath()`, `sessionInfo()`, `error()` | Recording metadata and diagnostics. |
| `setContextProvider(provider, context)`, `context()` | External contextual label; provider has signature `std::string (*)(void*)`. |
| `publish()` | Flush the current thread's non-frame work. |
| `count(name, value, unit = "count")` | Publish a counter. |
| `gpuTimingEnabled`, `gpuPassTimingEnabled`, `gpuDetailedTimingEnabled` and matching `set...` functions | Control GPU timing collection/detail. |
| `beginGpu`, `completeGpu`, `gpuQueueStatus` | Transport asynchronous GPU samples and queue status through an adapter outside `gpu`. |

Replacing/detaching a context provider waits for an executing callback. A provider
must not replace itself; owners detach it before destroying borrowed context.
Stopping recording accounts for pending GPU request tokens without waiting on
native queues; late completions retain their identities.

## Snapshot and recording data

`CpuProfileSample` distinguishes `Kind::Scope`, `Counter`, and `Frame`. It
contains epoch/recording/thread IDs, parent/scope IDs, frame identities,
`begin`/`end`, call counts, inclusive/exclusive/maximum milliseconds,
counter values, completion/render flags, label, and unit. Timestamps are steady
clock nanoseconds, not Unix timestamps.

`CpuProfileSnapshot` contains `samples`, per-thread/label/unit `totals`,
`gpuSamples`, `gpuQueues`, GPU pending/drop/failure/unsupported/eviction/late
counts, presentation/scene/incomplete frame counts, time range, and CPU
`evicted`, `dropped`, `writerDropped`, and `partial` counts. Totals are not
normalized by frame count.

`CpuProfileRecordingStatus` exposes `active`, `session`, `begin`, `end`, output
`path`, and `error`. See [GPU telemetry](GpuProfile.md) for GPU transport types.

## See also

- [Base API](index.md)
- [API reference](reference.md)
