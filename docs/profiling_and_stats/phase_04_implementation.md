# Phase 04 implementation and verification

Status: implemented in source. Native compilation, application acceptance,
GPU-tool comparisons and overhead measurements remain pending. No native build
or GPU execution was performed: repository instructions require explicit build
authorization, which was not part of this request.

## Interval contract and bounds

`CommandBuffer::beginTimingInterval/endTimingInterval` and `GpuTimingScope` add
copied, bounded names, recording-local invocation IDs, parent IDs and 64-bit
contexts. The scope closes on exceptions and marks that invocation aborted.
Missing endpoints become aborted at terminal submission publication. Duplicate
ends are harmless. Admission after submission is rejected. Recording APIs belong
to the recording thread; they are not concurrent command-recording primitives.

Each command carries at most 32 intervals and 95 bytes per label. Detailed-only
admission stops at 24 slots, reserving eight slots for later coarse passes. Capacity
omissions increment `intervalsDropped`. Labels can be truncated; IDs/context retain
invocation identity. Unsupported sampling, no natural encoder, missing samples,
query exhaustion and exceptions remain explicit terminal states, never zero-time
substitutes. Parent relationships are metadata: overlapping/nested elapsed values
must not be summed.

GPU submission, GPU pass, and detailed transparent sampling have separate controls.
Modes are captured at request admission, and raw CPU counter events record mode
changes. Total-only mode allocates two Vulkan queries and no Metal counter buffer.
Disabling GPU timing leaves CPU collection/recording running. Closing Performance
still releases only its live subscription; recording remains independent.

## Vulkan

Each measured command owns a fresh query pool until existing fence completion.
Pass mode reserves 66 timestamp slots: two total endpoints and two per interval.
All slots are reset once outside rendering. Pass markers are accepted only outside
active rendering/compute scopes, avoiding multiview query expansion and lazy render
materialization. There are no new barriers or command-buffer splits.

A begin marker uses TOP_OF_PIPE and an end marker BOTTOM_OF_PIPE. The result is a
pipeline execution envelope that can include other queued/overlapping work, not
exclusive shader time. This follows the Vulkan [timestamp query rules](https://docs.vulkan.org/spec/latest/chapters/queries.html).
Only paired, ended intervals are read, individually, after ordinary completion.
Reads use 64-bit values and availability, without WAIT/PARTIAL flags. Existing
valid-bit wrap handling and CPU elapsed upper-bound rejection apply independently
to every interval. Failure or unavailable data does not invent a duration.

## Metal

The optional path checks runtime selectors, stage-boundary counter support,
the timestamp counter set and its timestamp counter, then attempts a shared
256-sample counter buffer. Allocation or capability failure leaves total
GPUStartTime/GPUEndTime timing functional. The sample records the actual interval
capability, including `None` for total-only/fallback commands.

Natural render descriptors attach start-of-vertex/end-of-fragment samples; natural
compute descriptors attach start/end-of-encoder samples. A pair is shared by all
enclosing intervals. Up to 128 natural encoders can be sampled per command, with
explicit dropped interval states thereafter. No encoder is split or created solely
for measurement; no per-draw/dispatch support is required. Interior endpoints are
unsupported. An interval spanning multiple encoders reports an encoder envelope,
including gaps between its first and last sampled encoder. Blits in this command
path mark enclosing intervals unsupported, rather than omit transfers from an
apparently complete duration. Total command timing still covers that work.

Counter data resolves only inside normal completion publication. Paired
`sampleTimestamps` calls before recording and after completion establish the
local CPU/GPU clock ratio described in Apple's [timestamp conversion guidance](https://developer.apple.com/documentation/metal/converting-gpu-timestamps-into-cpu-time).
CPU ticks convert with `mach_timebase_info`; only GPU duration differences are
converted, never mixed with `steady_clock` timestamps. Nonmonotonic calibration,
counter error values, samples outside the calibration range, nonfinite durations
and durations beyond the CPU elapsed bound remain unavailable. This is separate
from the chart's CPU request-time correlation.

The completion handler retains a weak ticket; its resolver retains counter
storage until normal completion. It does not invoke application callbacks, retain
the Performance window or synchronize to complete a report.

## Draw, UI and recording

Instrumentation covers G-buffer, deferred lighting, skybox, transparent processing,
query shadows, AO, GI, reflections, temporal accumulation, denoise, reconstruction,
final conversion and UI composition. Effect/layer names match existing CPU labels
where present. Transparent contexts use object ID and submesh; invocation IDs also
distinguish repeated temporal signals/passes. Coarse opaque effects are enabled
with pass timing; detailed transparent invocations require the detail switch.
No production `bg2e::render` code was changed.

Performance adds a bounded 128-label GPU stage selector, a chart averaging only
available selected intervals, and a bounded 256-row pass table. The table shows
command/invocation/parent, label, context, actual granularity, state and duration.
The submission table marks partial pass breakdowns; pending publications and
native/publication drops remain visible. Unsupported/missing results leave chart
gaps. Repeated invocations are averaged per bin, never added with their parents.
Labels outside the selector are counted. Headers expose no ImGui types.

JSONL version 3 retains one `gpu_submission` record per accepted submission, adding
`pass_gpu`, `detailed_gpu`, `interval_capability`, `intervals_dropped` and a bounded
`intervals` array. Every interval carries ID, parent, context, label, state,
granularity (0 none, 1 pipeline boundary, 2 encoder envelope) and a duration that
is null unless available. The validator also reads versions 1 and 2.

Intervals share submission session/frame identity and all existing boundaries:
pre-start results stay live-only, Stop counts unresolved submissions once (not
once per nested interval), accepted file records drain, and late completions
cannot enter a later recording. Native cleanup uses the existing submission drain;
unsubmitted recordings abort. Resize/replacement does not relabel old tickets.
File failure and application shutdown continue through the existing writer/error
and MainLoop shutdown paths. These lifecycle paths were reviewed in source, not
validated in a running application during this phase.

## Memory and overhead evidence

These are source bounds, not measured memory/overhead results. A static assertion
limits plain GPU telemetry payloads to 8 KiB per submission, including intervals.
The existing limits therefore bound GPU payload storage as follows, excluding
container/allocator/native-object overhead and independent CPU telemetry:

| Component | Maximum GPU payload |
| --- | --- |
| Profiler correlation tokens (256) | 2 MiB |
| Completed live history (512) | 4 MiB |
| Writer queue (256 one-GPU-result batches, shared with CPU batches) | 2 MiB |
| One copied UI snapshot (512) | 4 MiB |
| One backend queue's ready plus active tickets (128 + 128) | 2 MiB |

Each pass-enabled Metal command additionally reserves at most 256 native timestamp
samples (2 KiB of resolved timestamp values; driver allocation overhead is unknown).
Vulkan reserves 66 queries per pass-enabled command; native query memory is
implementation dependent. Label cardinality and transparent-layer count cannot
grow these per-command limits. Native storage stays alive until existing completion
proof, even when a logical token expires or recording stops.

| Mode | Implemented control | Runtime overhead result |
| --- | --- | --- |
| Disabled | No live subscriber/recording | Not measured |
| Live CPU | Performance open, GPU off | Not measured |
| CPU recording | Recording active, GPU off | Not measured |
| Total GPU | GPU on, pass switch off | Not measured |
| Coarse passes | Pass on, details off | Not measured |
| Detailed GPU | Pass and details on | Not measured |

Existing `profiling.gpu.request`, `profiling.gpu.collect`,
`ui.performance.snapshot`, `ui.performance.series` and displayed refresh/draw costs
provide collection/publication/UI evidence when run. Native query/counter resolution
and file writer costs still need separate native-tool/CPU measurements; they are
not inferred from GPU durations or claimed measured here.

## Actual checks and pending acceptance

Executed without native compilation:

- `git diff --check`.
- Python schema probe: three valid v1/v2/v3 captures accepted; 13 invalid interval
  captures rejected (identity/hierarchy, label/capacity, granularity, terminal state,
  null/nonfinite/negative duration, and disabled-pass consistency).
- Recursive public header audits: Performance (3), UI umbrella (157), GPU umbrella
  (39) transitive engine headers, with no ImGui includes/types.
- GPL/delimiter/source-declaration audits, GPU dependency direction and diff audit
  for new GPU waits, WAIT query flags, CMake changes and production-render edits.

Added but **not compiled or executed**: `gpu_interval_probe.cpp` covers hierarchy,
contexts, exceptional/missing endpoints, unsupported fallback, duplicate ends,
detailed capacity reservation, post-submission admission and total-only mode.
`performance_data_probe.cpp` additionally checks selected child means without
summing parent durations. Existing CPU/submission probes remain pending execution.

When builds are authorized, build the library, composer and affected examples;
run the interval probe with `lib/src/bg2e/gpu/SubmissionTiming.cpp` and normal public
include paths/C++20, alongside the earlier documented probes. Then run the summary's
functional matrix on Metal and Vulkan, with native validation and GPU-tool comparison.
Record hardware, driver, counter capabilities, scene/resolution, effects, transparent
layers and sampling mode. Compare settled identical workloads after warm-up in all
six modes above. Include sustained capacity pressure, hidden-window recording,
Stop/late completion, file failure, resize, scene/renderer replacement, device loss,
owner destruction and application exit. Measure memory and query/writer/UI costs.
All these native acceptance and overhead items remain explicit gaps.
