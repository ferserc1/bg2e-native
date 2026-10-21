# Phase 07 Temporary resource pools

## Outcome

Temporal accumulation, denoise, and effect/fullscreen invocation preparation reuse compatible retired resources instead of allocating new images, uniform buffers, and resource sets every frame.

## Implementation

1. Introduce bounded pools keyed by device, extent, format, usage, layout identity, and other compatibility properties. Use normal acquired-slot retirement or explicit completion ownership to return resources to a pool.
2. Pool temporal signal/position/normal images while preserving both current histories and previous histories still read by submitted work. A fixed two-image ping-pong is insufficient unless the scheduling proof covers every in-flight reader.
3. Pool denoise outputs and invocation uniform buffers/sets. Preserve distinct invocation storage within a frame, particularly across transparent layers and effect keys. Reusing a GPU image after ordered consumption requires a separate aliasing proof; start with retired resources.
4. Reuse compatible fullscreen and effect surface bindings by slot and invocation identity, rebinding only changed tuples. Integrate the phase 04 stable arrays without introducing mutable cross-invocation sharing.
5. On resize, configuration changes, aborted recording, or device cleanup, retire incompatible resources safely. Cap retained bytes and idle entries; release excess completed resources. A returned image still held by a caller is not free pool storage.

## Verification and exit

Exercise multiple slots, long GPU latency, many transparent layers, changing effect counts, freeze-at-limit, resize, history reset, standalone effect results held by callers, and failed recording. Check no history is overwritten or output reused before its consumers finish.

Measure allocation counts and retained memory after warmup and after workload contraction. Stable workloads should stop allocating compatible temporary resources repeatedly. Exit with all histories and effect outputs correct; this phase must not depend on relaxing temporal invalidation in phase 08.

## Implementation record

Status: source implementation complete; compilation, native probe execution,
visual comparisons and measured allocation/retention acceptance remain pending.
No native speedup or validation pass is claimed.

- TemporaryResourcePool matches device, extent, format, exact usage, image type,
  effective mip count and native binding generation. Binding leases additionally
  match retained layout, set, uniform size and invocation identity. Both pools
  retain explicit recording tickets, not a frame-number completion assumption.
- Temporal signal/position/normal images and denoise outputs are leased only when
  their writer completes successfully and the pool is their sole shared owner. Current/previous
  histories, in-flight invocation owners and standalone result owners pin images.
  Every integrated binding lease retains its sampled inputs, including later reads
  of another component's output. There is no image aliasing within a recording or
  fixed two-image history assumption.
- Temporal/denoise, AO/GI/reflection surface preparation, shadow invocation set0 and
  fullscreen image sets reuse retired uniform/set storage. Slot plus signal/view/
  layer or invocation ordinal distinguishes live invocations. Full native tuples
  compare image generations and weak-owner identity, updating only changed bindings.
  Phase 04/05 immutable scene/array sets stay separate and are never rewritten.
- Native recording tickets now distinguish retired abandonment from completed GPU
  work. Wrapper destruction or successful reset retires an unsubmitted attempt;
  a submitted ticket still waits for normal completion. Abandoned image entries
  lose cache ownership instead of being recycled, because their recorded Vulkan
  layout transitions may never have executed; uniform/set storage can retire safely. Acceleration reuse continues
  requiring completed(), so aborted builds remain ineligible. Aborted temporal
  history is rejected by later recordings, including same-sequence retry.
- Cache defaults are 512 MiB estimated image/uniform payload, 256 images, 256 sets
  and eight idle retirement visits per component. Temporal/denoise configuration
  exposes poolLimits. Completed excess/idle entries lose cache ownership; pending
  work may exceed limits and is never discarded for a budget. Externally held
  images survive eviction and component cleanup. Existing cleanup drains remain;
  no per-frame wait or new device drain was introduced.
- setPoolingEnabled(false) on affected components preserves the fresh allocation
  diagnostic path. poolStatistics reports current counts and estimated payload;
  profiler counters distinguish image/set/uniform allocations, hits and descriptor
  updates. Temporal invalidation conditions and shader contracts are unchanged.

Static checks cover ticket lifecycle, completion/lease gates, full image compatibility,
per-invocation identity, owner retention, cache bounds, changed-tuple updates,
cleanup ordering and whitespace. New resource-pool native probes and an abandoned
history/same-sequence retry fixture were delivered as source only. No CMake files
were modified; no code/shader compilation or application/test execution was run.

Expected counters after warmup within budget: no repeating temporary.imageAllocations,
temporary.uniformAllocations or temporary.bindingAllocations for a stable workload;
image/binding hits increase and unchanged fullscreen/surface tuples skip updates.
Changing history image tuples still updates descriptors without allocating new sets.
Retained bytes are payload estimates, excluding allocator alignment/descriptor overhead.
Use poolStatistics snapshots for contraction/idle-age measurements; profiler retention
samples aggregate counts across collect visits and components.

Pending exit: run tests/draw/resource_pools plus existing temporal, denoise, shadow,
AO, GI, reflection and deferred probes on Vulkan/Metal. Compare pooled/fresh output
at multiple slots, long GPU latency, many layers, effect-count contraction, freeze
at limit, resize/reset, source destruction, held standalone results and aborted work.
Capture validation, actual allocation counts, retained memory, exclusive CPU timings
and independent GPU timings. Verify reset of a Vulkan command wrapper and failed
native submissions; neither may release submitted readers or certify aborted TLAS.
