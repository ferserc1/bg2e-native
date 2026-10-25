# GPU profiling transport

**Header:** `<bg2e/base/GpuProfile.hpp>`  
**Namespace:** `bg2e::base`

These plain telemetry types carry asynchronous timing information without
native graphics or UI types. Include this header explicitly when needed.

## Enumerations

| Type | Values |
|------|--------|
| `GpuProfileStatus` | `Pending`, `Available`, `Unsupported`, `Dropped`, `Failed`, `Aborted`, `Unavailable` |
| `GpuProfileBackend` | `Unknown`, `Vulkan`, `Metal` |
| `GpuProfileWorkload` | `Unassociated`, `Scene`, `Ui`, `SceneAndUi`, `Auxiliary` |
| `GpuProfileGranularity` | `None`, `PipelineBoundary`, `EncoderEnvelope` |

## GpuProfileInterval

One bounded labeled interval: `label` is a 96-character array; `context`, `id`,
and `parent` identify it. `durationMs`, `status`, `granularity`, and `ended`
describe the measurement and completion state. Initially pending, with no
measurement granularity and `ended = false`.

## GpuProfileSample

Contains a fixed array of up to `maximumIntervals = 32` intervals,
`intervalCount`, and `intervalsDropped`. `passTiming` defaults to true,
`detailed` to false, and `intervalCapability` to `None`.

Identity fields include token, epoch, recording, thread, presentation/scene
frames, device, queue, command, and submission. `requested`, `submitted`, and
`resolved` are CPU monotonic correlation timestamps: never subtract them from
GPU-clock timestamps. `durationMs` is the resolved GPU duration; inspect
`status` before interpreting it. `backend` and `workload` identify its source.
The sample is bounded to at most 8192 bytes.

## GpuProfileQueueStatus

A per-device/queue status record with `pending`, `dropped`, `failed`, and
`unsupported` counts, `supported` (initially false), and `backend`.

These records do not fetch results or synchronize queues themselves.
[CpuProfiler](CpuProfile.md) accepts their publication through its GPU adapter
entry points.

## See also

- [Base API](index.md)
- [API reference](reference.md)
