# Phase 03 implementation and verification record

Status: implemented; native acceptance pending. No library/application/probe
compilation, native execution, GPU validation, graphics-tool comparison or
performance measurement was authorized or performed. Phase 04 remains planned.

## Delivered API and ownership

`gpu/SubmissionTiming.hpp` provides optional capabilities, request/result/status
aliases and a polling batch. Plain telemetry lives in `base/GpuProfile.hpp`, so
base never includes GPU, draw, scene or UI headers. The GPU API uses only numeric
caller identities and base/standard types; native timestamp storage remains in
the backend. Queue defaults return unsupported/empty results without throwing.
Both Metal-disabled and native branches define the new command-buffer method.

Call `requestSubmissionTiming(request)` immediately after `CommandBuffer::begin`,
before recording workload commands. An accepted request returns Pending; an
immediate Unsupported/Dropped result is returned to the caller for publication.
Scene/workload association can be refined with `setSubmissionTimingContext`
before submission, without changing the original frame/session/request identity.

Each request identifies capture epoch, recording session, originating CPU thread,
frame token, intended presentation number, optional scene frame, device, native
queue, command recording and submission. Device identities change on native
Device::create, including reuse of the same wrapper. Vulkan queue index 0 aliases
share the same `(device, family+1)` identity. Metal queues receive unique identities.
Submission IDs come from the existing device-wide completion registry. Unsubmitted
or unknown identities remain zero rather than being inferred from a later frame.

A fixed-capacity queue-local timing state owns tickets independently of command
wrappers. Existing CompletionRecord terminal callbacks resolve/fail them. Abandoned
unsubmitted recordings release resources without waiting; submitted tickets stay
owned until normal completion. Vulkan failed timed sends invalidate executable
state before releasing query resources, preventing retry of a command referencing
a destroyed query pool. Reset/new recording abandons only the old unsubmitted
request. Device cleanup closes timing storage after its existing submission drain,
before native device destruction. No measurement-specific device/queue drain exists.

## Vulkan implementation

Queue initialization queries timestampValidBits for the actual family and the
physical device's timestampPeriod. Zero/invalid bits or a nonpositive/nonfinite
period explicitly disable timing. Every accepted recording allocates an exclusive
two-query timestamp pool. A command reset and top-of-pipe timestamp precede the
workload; a bottom-of-pipe timestamp follows the final workload commands, outside
render/compute scopes. No hostQueryReset feature or timestamp extension is needed.

Only after existing fence completion proves execution is terminal does the
resolver call vkGetQueryPoolResults with 64-bit results and per-query availability.
There is no WAIT or PARTIAL query flag. Missing availability/non-success produces
Unavailable, not a zero-duration success. Native query storage is released at
terminal retirement and is never shared with an outstanding recording or another
session. Fresh pool creation bounds lifetime correctly but can incur driver CPU
cost; hardware measurement is required before claiming acceptable overhead.

Timestamp subtraction masks the queue's valid bits and converts ticks with its
period. A CPU request-to-resolution upper bound rejects ambiguous multiple wraps
or implausible conversions. It is a conservative validity bound, not CPU/GPU
clock calibration. TOP-to-BOTTOM measures the command workload's GPU execution
envelope, including dependencies/stalls within it. It is not CPU submission cost,
queue latency, presentation scanout or a sum of individually isolated passes.
See [Khronos timestamp semantics](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html),
[query result/availability rules](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetQueryPoolResults.html)
and the [timestamp conversion sample](https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html).

## Metal implementation

Each request checks runtime availability of GPUStartTime/GPUEndTime selectors.
The existing completion record retains native command ownership across commit
and ordinary retirement. A native completed handler reads the timestamps and
publishes a plain result into queue-local storage. Its weak ticket capture avoids
cycles for command buffers abandoned before commit, and never calls an application,
profiler or UI owner from Metal's callback thread.

Completed status, finite/nonzero start and a finite nondecreasing end are required;
errors/missing timestamps are Failed/Unavailable. The duration is the native
end-minus-start interval converted from seconds to milliseconds. CPU time around
commit is not substituted. The normal poll/wait path still handles execution
errors; a timing failure does not add a wait or change that error contract.
See Apple's [GPUStartTime](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpustarttime)
and [GPUEndTime](https://developer.apple.com/documentation/metal/mtlcommandbuffer/gpuendtime)
completion-boundary and duration documentation. Fine-grained encoder counters
are intentionally deferred to phase 04.

## Completion polling and draw integration

CompletionRecord::pollNonblocking uses try_lock. It cannot block behind another
thread holding the record mutex in a normal GPU wait. SubmissionState's optional
poll also uses try_lock and skips an active drain. Failed records remain available
to normal admission/drain error reporting. The existing synchronization/wait
paths remain; no native GPU wait or blocking query read was added for measurement.

The draw loop requests the graphics command's timing before UI preparation, then
marks it Scene, UI or SceneAndUi when the refresh decision is known. Scene IDs are
zero for UI-only work; a GPU result does not imply successful CPU presentation.
An exception after actual submission can leave a valid GPU sample associated with
an incomplete CPU frame token, which must not be counted as scene/presentation FPS.

A consumer in draw polls graphics, present and transfer queues during normal
frames and after the already-required cleanup wait. It publishes plain results
to CpuProfiler outside GPU locks. Aliased native queues merge diagnostic counts
only, never durations. Only the primary draw command is requested automatically;
uploads/other auxiliary submissions require explicit requests from their owning
caller. The same consumer collects those results if they use the exposed queues.
It does not pretend that unmeasured auxiliary work is zero or part of a complete
frame GPU total. A queue's result stream has one publication owner; independent
consumers must not race to drain the same queue into different sinks.

Disabled process-wide profiling skips request/query allocation and draw polling,
except while earlier accepted GPU requests still require retirement. Performance's
GPU submission timing checkbox independently disables new GPU requests while
CPU live collection/recording continue. Existing GPU results and outstanding work
remain intact. The initial setting is included in each session header; changing
it during a recording affects subsequent requests only. Changes also emit raw
`profiling.gpu.enabledChanges` counter events (0 off, 1 on); their additive
total counts enable transitions, not the current mode. `profiling.gpu.request`
and `profiling.gpu.collect` CPU scopes expose adapter/query-allocation/poll cost.

## Bounds, losses and recording boundaries

- Each logical queue wrapper has at most 128 outstanding tickets/query pools
  (256 Vulkan query slots) and 128 completed-result slots. These limits are
  independent of the existing general submission registry. Vulkan aliases may
  have separate bounded storage while reporting one native queue identity.
- Completed-result overflow drops the newest result and increments backend
  dropped counts. The poll API reports pending/dropped/failed/unsupported counts.
  Immediate request rejections return status directly; the external consumer
  publishes them into the profiler's counters/history.
- The profiler admits at most 256 outstanding GPU correlation tokens, keeps at
  most 512 completed GPU records and at most 64 last-observed queue diagnostics.
  Completed history expires by CPU resolution timestamp after 30 seconds. Queue
  metadata overflow is reported as a telemetry drop; it does not block rendering.
- On a subsequent GPU request admission, logical tokens older than 30 seconds
  are dropped explicitly. A backend result lost to overflow can therefore never
  exhaust correlation storage indefinitely while new requests keep arriving.
  Logical expiry never frees/reuses native in-flight query storage. Results for
  expired/unknown tokens increment a late-result count instead of being relabeled.
- GPU records share the existing 256-batch bounded writer queue. Each GPU batch
  contains one result. Overflow increments writer_dropped independently of native
  and correlation losses. All formatting/write/close work stays on the writer.

Start reserves a recording ID only for newly requested samples after its accepted
boundary. Pre-start tokens/history remain live-only. Stop serializes publication,
counts current-session tokens not yet published (including unsubmitted/ready-but-
unconsumed requests), then closes CPU/GPU admission to that file. Accepted writer
records drain normally before the terminal summary. Stop does not poll GPU queues
or wait for their completion. Late results retain original epoch/session/frame and
may enter eligible live history; they cannot enter a new recording or reopen a file.

Base publication restores reserved epoch/session/thread/frame/request identity,
checks successful result metadata and rejects Pending/invalid durations as
Unavailable. Native resource lifetime is never transferred into base snapshots.
MainLoop still finalizes recording before graphics teardown; later cleanup
retirement is consequently live-only and never changes the frozen summary.

## Performance and file schema

Performance adds one 600-bin GPU duration series and up to 64 queue/epoch choices,
workload filtering and 128 recent result rows. Bins average individual durations
from one selected device/queue/epoch; overlapping submissions/queues are never
summed into frame time. The X axis uses original CPU request time, with explicit
uncalibrated-clock wording. Delayed/out-of-order completion cannot move a point to
the current frame. Missing/unsupported/failed samples leave gaps and show a status,
not a zero duration. Zero remains valid only for an actually available measurement.
Queue metadata is labeled last observed; selection never silently combines devices.
Pause/Clear retain their reader-local semantics for GPU data too.

New files use schema version 2, retaining CPU records and adding gpu_submission
records. They include request/submission/resolution CPU nanoseconds, IDs, backend,
workload (0 unassociated, 1 scene, 2 UI, 3 scene+UI, 4 auxiliary), status and GPU
duration in milliseconds. Non-available duration is JSON null. No raw GPU clock is
mixed with a CPU timestamp. Summary fields add gpu_available, gpu_failed (including
aborted/unavailable), gpu_unsupported, gpu_dropped and the actual unresolved_gpu.
Counters describe publication outcomes and can exceed written records on writer
overflow; they must not be summed as disjoint loss populations.

validate_cpu_capture.py accepts both versions and validates GPU boundaries,
identities, statuses, null-vs-valid durations, token uniqueness and summary counts.

## Verification actually performed

- git diff --check; source licensing/delimiter checks; native and platform-disabled
  declaration/definition review; local vendored Metal API signature/protection review.
- Public transitive engine-header audit: Performance.hpp 3 headers, ui/all.hpp 157,
  gpu/all.hpp 39; no ImGui includes/types in those graphs. GPU direct dependencies
  remain in gpu/base/app/geo/math; base does not include the GPU module.
- Diff audit confirmed no added vkDeviceWaitIdle, vkQueueWaitIdle, vkWaitForFences,
  waitUntilCompleted calls or WAIT query flags; CMake and production render unchanged.
- Python validator syntax and synthetic fixtures: v1 CPU and v2 GPU accepted;
  10 invalid cases rejected, including cross-session records, invalid/null/nonfinite
  available durations, non-null unavailable durations, timestamp bounds, missing
  native identities, duplicate tokens and inconsistent terminal counts.

No native C++ probe has been compiled or executed. The new CPU-only
`gpu_submission_probe.cpp` checks wrap math, completion polling, simulated host
lock contention, ticket abandonment/capacity/overflow, disabled GPU mode, file
boundaries, unresolved Stop and late-session isolation. It uses fake timing values
and cannot establish native GPU correctness. The existing presentation-model
probe now also checks late GPU placement, per-queue isolation, bin means and gaps.
After compilation is authorized, macOS/Linux can run the lifecycle probe with:

```sh
c++ -std=c++20 -pthread -Ilib/include \
  docs/profiling_and_stats/gpu_submission_probe.cpp \
  lib/src/bg2e/base/CpuProfile.cpp lib/src/bg2e/gpu/SubmissionTiming.cpp \
  -o /tmp/bg2e-gpu-submission-probe
/tmp/bg2e-gpu-submission-probe /tmp/bg2e-gpu-submission-probe-output
python3 docs/profiling_and_stats/validate_cpu_capture.py --gpu-probe \
  /tmp/bg2e-gpu-submission-probe-output/gpu_first.jsonl \
  /tmp/bg2e-gpu-submission-probe-output/gpu_second.jsonl
```

Use a fresh output directory. The previous CPU and presentation probes remain
applicable; the validator's --probe mode still targets only its CPU fixture files.

## Pending acceptance and measured gaps

Build the library/composer and affected consumers on available supported platforms;
exercise Metal-disabled/default-unsupported branches and Windows exported symbols.
Then run the CPU lifecycle/model probes and native Vulkan/Metal validation. Test
known GPU workloads versus native tooling, timestamp capability absence, long
latency/wrap ambiguity, out-of-order/multiple-queue results, aborted/failed sends,
query/result/writer/correlation exhaustion, slot reuse, minimized/paused/UI-only
frames, resize, scene/renderer/device recreation, rapid recording restarts, write
errors, hidden-window recording and shutdown/callback ownership.

Compare the same settled workload with profiling disabled, CPU-only, GPU-enabled
live and recording. Record query-pool creation/readback, completion callbacks,
consumer polling, snapshot/UI and writer costs separately. No Metal/Vulkan/Windows
runtime coverage or measured overhead is claimed. These pending gates prevent
calling the phase build-verified or hardware-accepted.
