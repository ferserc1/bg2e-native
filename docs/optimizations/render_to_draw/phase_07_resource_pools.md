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
