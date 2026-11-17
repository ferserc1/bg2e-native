# Persistent GPU Deep Debug Logging

Status: implementation added. See README.md for behavior, verification coverage and remaining runtime checks. No compilation or GPU execution has been performed.

## Objective and scope

Implement opt-in, bounded, persistent logging for applications using the Metal and Vulkan implementations of `bg2e::gpu`. The logs must help investigate intermittent native graphics failures, including failures that terminate the process on framework-owned asynchronous threads.

The scope is **logging and the native diagnostic metadata needed to make logs useful**. This work must not attempt to fix the MetalFX crash described below, change GPU scheduling, or change rendering results.

The latest user instruction narrows the earlier discussion: experiments and broader debugging features previously proposed in chat are context, not implementation requirements for this plan.

In scope:

- A `--deep-debug` startup flag in Composer.
- Runtime configuration and bounded persistent output in `bg2e::base::Log` (the actual namespace of the existing logger).
- Selected trace sites inside `bg2e::gpu::metal` and `bg2e::gpu::vk`.
- Native names, correlation identifiers, and useful native error details.
- Documentation and focused verification of the logging behavior.

Out of scope:

- Fixing MetalFX, MPSGraph, ANE, resource synchronization, or rendering defects.
- Serializing frames, adding GPU waits or fences, changing frames in flight, or splitting submissions.
- Changing MetalFX initialization mode, exposure, reset behavior, algorithm, or fallback policy.
- Automatically enabling API validation, shader validation, synchronization validation, or GPU-assisted validation. Existing validation output should be captured when those facilities are already enabled; their activation remains a separate diagnostic procedure.
- GPU captures, input readbacks, NaN scans, crash recovery, exception suppression, or signal-handler-based log dumping.
- Changes to `bg2e::render`, CMake, or unrelated engine APIs.

## User requirements and session constraints

- The requested deliverable in this session is this plan and enough context to resume implementation later.
- Generated files and documentation must be in English; chat replies should follow the user's language, Spanish.
- Do not compile unless the user explicitly requests it.
- Do not modify CMake unless explicitly requested.
- Follow the repository AGENTS.md, including full project license headers and source style for new or edited owned C/C++ files.
- Read the architecture reference `skills.md` before modifying engine internals, as required by AGENTS.md. A root-level file with that exact name was not found during the earlier searches; locate it when resuming and report its absence if still unavailable.
- Do not spawn agents unless explicitly authorized by the user or applicable repository instructions.
- Preserve meaningful GPU debug identifiers and native naming in all build configurations. Do not weaken existing diagnostics.
- The user has requested changes to the logger and application startup as well as backend logging. Keep all other API changes out of scope; preserve existing contracts and obtain authorization before any unrelated API change.

## Motivating incident: intermittent MetalFX crash

This incident is an example of what the tools must help investigate, not a bug to solve in this task.

### Supplied crash report

- Application: `bg2e_composer`, native ARM64, launched from a Desktop `.app` bundle.
- Incident: `B73CA87A-BE5D-42E1-B66C-61FF4A66F82C`.
- Launch: 2026-11-16 17:32:38 +0100; crash: 17:42:03 +0100, approximately nine minutes later.
- Reported OS: macOS 27.0.1, build 26A434.
- Hardware: Mac16,5, Apple M4 Max, 48 GB RAM, two 5K displays.
- Faulting thread: thread 8, dispatch queue `MetalFX`.
- Exception: `NSInvalidArgumentException`, `*** -[__NSArrayM insertObject:atIndex:]: object cannot be nil`.
- Termination: `EXC_CRASH (SIGABRT)`, `abort()`.
- Native framework versions in the report: MetalFX 40.8 and MetalPerformanceShadersGraph 7.0.80.
- Executable UUID: `51bf7a87-caa8-32db-877e-41d69a8904a2`.
- Engine dylib UUID: `2409cfe3-7e89-3c13-a08c-a2cb0ffb2250`.

Relevant exception chain, expressed from caller to failure:

```text
_MFXTemporalScalingEffectV4 encodeToCommandBuffer: asynchronous block
  -> MPSGraphMPSGraphExecutableWrapper::run
  -> MPSGraphExecutable runAsyncWithMTLCommandQueue:...
  -> GPU::ANERegionCallOpHandler::encodeOp
  -> GPU::ANERegionCallOpHandler::encodeAsynchronousWithSharedEvents
  -> NSMutableArray insertObject:atIndex: receives nil
  -> uncaught Objective-C exception -> abort
```

The main thread was waiting, not throwing the exception:

```text
Metal command buffer waitUntilCompleted
  -> bg2e::gpu::metal::Queue::submit completion wait callback
  -> bg2e::gpu::detail::CompletionRecord::wait
  -> bg2e::gpu::SurfaceFrame::waitForSubmissions
  -> bg2e::gpu::metal::WindowSurface::beginFrame
  -> bg2e::draw::RenderLoop::frame
```

### Findings and limits of the evidence

- The immediate failure is an uncaught Objective-C exception inside Apple's asynchronous MetalFX/MPSGraph/ANE execution path.
- The report does not identify which internal object was nil. The shared-events function name does not prove that an event was the nil object.
- The wait on the main thread does not establish a deadlock or make that wait the cause of the exception.
- This is not a reported out-of-memory termination or an `EXC_BAD_ACCESS`. The report alone does not rule out every resource or memory issue.
- The user subsequently ran Composer for 30 minutes with MetalFX disabled and observed no crashes. This strengthens the association with MetalFX but is not conclusive for an intermittent fault.
- An Apple framework defect is a hypothesis, not an established root cause. A backend integration or resource-lifetime issue remains possible.
- No reproduction, compilation, or runtime validation was performed by the agent.
- The checkout examined during the analysis was at `3d784890` (`Remove outdated submodule references from project docs and scripts`). It has not been established that this checkout matches the crashing binaries. Recheck current sources when resuming.

### Relevant implementation observations

- `lib/src/bg2e/gpu/metal/MetalFXBridge.mm` calls `[scaler encodeToCommandBuffer:native]`.
- Its local `@try/@catch` cannot catch an exception thrown later on Apple's internal MetalFX queue.
- Its completed handler currently records command-buffer failure as a boolean; that handler cannot recover from a process-aborting exception on another thread.
- The bridge checks native texture allocation and validates input device, format, and dimensions. It checks slot retirement before reuse.
- Private bridge textures use tracked hazards. No evidence was found that simply adding a fence would address this crash.
- `WindowSurface::beginFrame()` waits for the previous submissions using the acquired slot.
- `RendererDeferred::resize()` synchronizes before rebuilding the upscaler.
- Composer enables continuous rendering, so leaving the UI idle can still exercise MetalFX repeatedly.
- Useful missing evidence includes the last submissions, pending work, scaler generation, resource names, lifecycle events, and native errors preceding the abort.

## Proposed implementation

### 1. Runtime activation

Add a boolean CLI flag, default off:

```sh
bg2e_composer --backend metal --deep-debug
bg2e_composer --backend vulkan --deep-debug
```

Configure the logger after argument parsing and before graphics initialization. Support Release builds without redefining the meaning of `Log::isDebug()` or changing existing Debug/Release behavior globally.

Expose the necessary runtime configuration through `bg2e::base::Log`. Do not add trace methods or deep-debug virtual methods to the common `bg2e::gpu` interfaces. Each backend owns its event generation and native details. Keep startup wiring out of `bg2e::draw` unless inspection reveals a strictly necessary, explicitly scoped adjustment.

### 2. Logger behavior and compatibility

Extend `lib/include/bg2e/base/Log.hpp` and `lib/src/bg2e/base/Log.cpp` with:

- Explicit initialization and shutdown of the persistent diagnostic sink.
- A cheap runtime-enabled query and lazy trace emission.
- Thread-safe, complete-record writes with timestamps and sequence numbers.
- Capture of selected existing warnings/errors while deep debugging, without losing console output.
- Bounded records, file size, and internal buffering.
- Clear reporting of sink initialization and write failures, without recursive logging.

Preserve existing logging macros, chaining syntax, level filtering, and ordinary console behavior. Do not turn all existing Debug messages on merely because deep debugging is enabled. Avoid an unnecessary global refactor of stream formatting.

Do not buffer an entire session only in RAM. Flush records at selected critical boundaries and errors so a process abort leaves useful evidence. A stream flush is not a promise of durability across power failure; do not `fsync` every record. Do not rely on destructors or crash handlers to save the only copy of the trace.

Log locks must never remain held across native graphics calls or GPU waits. Do not introduce callback-to-wait deadlocks. Keep capture lifetimes safe when native callbacks outlive application-side objects or logger shutdown.

### 3. Location and session handling

Use `bg2e::base::PlatformTools::applicationPath()` as the location source. Current platform behavior differs:

- Linux and Windows: executable directory.
- macOS: the `.app` bundle path, not `Contents/MacOS`.

Resolve the actual executable directory inside the macOS bundle, using native bundle metadata rather than assuming an executable filename. Put `bg2e-deep-debug.log` next to that executable. Preserve the public contract of `applicationPath()`.

When the requested directory is not writable, report a clear startup diagnostic; do not silently redirect to another directory or claim that persistent logging is active. Define startup failure handling explicitly during implementation.

Prevent concurrent processes from corrupting the same file. Prefer a fixed session file with exclusive ownership and an explicit error if occupied. Do not accumulate unbounded session archives. A new diagnostic session must have clear initialization semantics; document that the previous log should be copied before restarting if it is needed for investigation.

### 4. Volume estimate and retention

Initial estimate for a small-to-medium Composer scene with about 40 nodes and 30 drawables:

| Selected event group | Estimated records per frame |
| --- | ---: |
| Acquisition, presentation, frame boundaries | 6-10 |
| Recording and relevant pass/encoder boundaries | 30-50 |
| Submissions, waits, completion | 10-20 |
| MetalFX/FSR or other special native operations | 10-20 |
| Approximate total | 60-100 |

These numbers are planning estimates, not measured instrumentation counts. The renderer's passes and submissions matter more than node count because individual draws will not be traced.

Budget approximately 100 records at 240 bytes each: 24 KB per frame, or 12 MB for 500 frames. Use an initial hard limit of **12 MiB**, including the session header. Retention is approximately 500 frames, not a promise of exactly 500 frames under validation storms or resource-loading bursts.

Keep a chronological text file. On reaching capacity, remove the oldest complete records in blocks, initially around 512 KiB, preserving recent records and session metadata. Use bounded temporary storage and safe replacement where appropriate; avoid rewriting the full file per appended record. Document transient disk usage during compaction and do not rotate into unbounded backup files.

Bound unusually large native messages, mark truncation, and coalesce repeated validation messages with occurrence counts and a bounded cache. Do not suppress distinct errors. After implementation, measure actual records and bytes per frame and revise the estimate if necessary.

### 5. Event selection and correlation

Emit events from backend implementation files, not higher-level scene or draw code. Use a common readable envelope with native payloads:

```text
sequence timestamp backend thread event surface/stream frame slot submission resource result
```

Only include fields applicable to the event. Assign stable diagnostic identities for resources and submissions; raw pointers and reused native handles alone are insufficient. Associate callbacks with the original submission, not whichever frame is current when they execute. Distinguish surfaces and offscreen streams instead of assuming one global frame counter.

Trace the following selected boundaries:

- Instance, device, queue initialization and teardown.
- Surface creation, acquisition, presentation, resize, and release.
- Command recording and relevant pass/encoder boundaries.
- Submission acceptance, waits, and terminal results.
- Relevant native resource creation/destruction and allocation failures.
- Special native integration boundaries such as MetalFX and FidelityFX.

Summarize draws, dispatches, bindings, copies, and transitions with counters per command buffer when useful. Do not log every draw, vertex, matrix, descriptor update, or transition. Do not poll completion solely to generate repeated logs; report meaningful state changes and actual waits.

Record both entry and return for critical operations when a missing return would help locate a failure. A returned encode call does not mean asynchronous native execution has completed, and the last logged operation must not be presented as proven causality.

### 6. Metal-specific logging

Likely implementation locations: `metal/Instance.cpp`, `Device.cpp`, `Queue.cpp`, `CommandBuffer.cpp`, `WindowSurface.cpp`, `OffscreenSurface.cpp`, relevant resource files, and `MetalFXBridge.mm`.

- Publish meaningful native names for queues, command buffers, relevant encoders, and internal MetalFX textures, including role, generation, and slot where useful. Preserve naming in Release and outside deep-debug mode as required by repository policy.
- In deep-debug mode, request `MTLCommandBufferErrorOptionEncoderExecutionStatus` when creating command buffers, retaining existing resource-reference semantics. This is additional error metadata, not a scheduling experiment.
- Log native error domain, code, description, and bounded relevant encoder information when provided.
- Record MetalFX instance/generation, initialization configuration, slot, command identity, encode entry/return, completion failure, and cleanup.
- Record information needed to diagnose unexpected lifetime or ownership states without changing synchronization, adding stronger public preconditions, or silently changing fallback behavior.

Enhanced command-buffer diagnostics may not capture an Objective-C exception on Apple's private queue. Do not promise that this feature alone diagnoses or prevents the motivating crash.

### 7. Vulkan-specific logging

Likely implementation locations: `vk/Instance.cpp`, `Device.cpp`, `Queue.cpp`, `CommandBuffer.cpp`, `WindowSurface.cpp`, `OffscreenSurface.cpp`, relevant resource files, and `FidelityFXBridge.cpp`.

- Route already-enabled validation warnings and errors into the persistent trace, including message identifier, severity/type, and bounded object names/details.
- The current instance callback prints only severities strictly greater than WARNING. Include warnings in the deep-debug trace without silently changing normal-mode output policy.
- Log native results for acquire, submit, present, fence/status checks, and waits. Distinguish expected statuses such as not-ready, timeout, suboptimal, and out-of-date from actual failures.
- Preserve existing error handling, device-loss behavior, and synchronization semantics.
- Report which validation/debug facilities are actually active or unavailable. Deep logging must remain useful without installed validation layers.
- Keep existing `VK_EXT_debug_utils` naming behavior and report naming limitations where applicable.

Do not change Vulkan validation activation merely to route messages into the new sink. Earlier chat discussed activation in Release, but automatic activation is excluded by the final logging-only scope.

### 8. Disabled-mode cost

- Test the runtime flag before constructing strings, retrieving timestamps, acquiring logger locks, or calculating diagnostic-only payloads.
- Use lazy emission or guarded macros so disabled call arguments are not evaluated.
- Cache immutable session settings where appropriate.
- No persistent files, diagnostic buffers, detailed counters, native error-info instrumentation, or logger worker threads when disabled.
- Do not weaken existing required native naming to achieve this cost target.

## Implementation sequence

1. Recheck AGENTS.md, architecture documentation, current git state, logger usages, and backend lifecycles.
2. Implement and verify the bounded sink in `base::Log`, preserving existing console behavior.
3. Wire Composer's CLI and platform-specific executable-directory resolution.
4. Add selected Metal and Vulkan lifecycle/submission logging with safe identifiers.
5. Add native error metadata and MetalFX/FidelityFX boundary logging.
6. Document launch commands, file location, retention estimate, failure handling, and interpretation limits.
7. Perform source review and focused checks permitted by the session. Compile or run compiled tests only after explicit user authorization.

## Verification and acceptance criteria

- Disabled mode creates no diagnostic file and avoids diagnostic payload evaluation.
- Release mode accepts the flag and produces useful traces without changing `Log::isDebug()` semantics.
- Existing console logging syntax and severity behavior remain compatible.
- The file is bounded, chronological, readable, and retains complete recent records through repeated compaction.
- Concurrent writes do not interleave records; concurrent application instances cannot corrupt a shared file.
- A forced process abort in a logging test leaves already-flushed records readable; do not require final shutdown to persist them.
- Invalid/unwritable paths and sink write failures are visible and do not recursively log or deadlock.
- Async completion events retain correct submission/resource identities across slot reuse, resize, and shutdown.
- Vulkan warnings/errors are captured when validation is active, and native results remain useful when it is absent.
- Metal native error details and internal MetalFX resource names are present when available.
- No new waits, fences, serialization, MetalFX behavior changes, validation activation, or rendering fixes are introduced.
- Measure runtime overhead and observed log volume on representative Composer workloads after compilation/execution is authorized. Report estimates separately from measurements.

## Reference material from the investigation

- Metal command buffer error metadata: https://developer.apple.com/documentation/metal/mtlcommandbufferdescriptor/erroroptions
- Metal API validation, for separate manual investigations: https://developer.apple.com/documentation/xcode/validating-your-apps-metal-api-usage
- MetalFX initialization behavior, an excluded experiment: https://developer.apple.com/documentation/metalfx/mtlfxtemporalscalerdescriptor/requiressynchronousinitialization
- MetalFX optional fence for untracked resources: https://developer.apple.com/documentation/metalfx/mtlfxtemporalscalerbase/fence

Verify current SDK support before implementation. These references explain diagnostic context and do not authorize expanding this task into crash remediation.
