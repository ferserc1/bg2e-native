# Phase 06 Scene acceleration reuse

## Outcome

Frames with unchanged tracing geometry, instance membership, transforms, masks, and opacity reuse completed acceleration data and instance buffers. Camera-only movement does not rebuild the TLAS.

## Implementation

1. Define an explicit instance-state key using mesh/BLAS identity and generation, submesh ranges, ordered instance correspondence, world transforms, masks, opacity classification, and relevant tracing inclusion state. Raster visibility changes that affect collection must invalidate it.
2. Retain immutable acceleration generations independently of the frame wrapper. Reuse built TLAS and instance buffers only when their complete keys match. Separate material scalar edits from changes in instance material addressing or RT opacity.
3. Keep build-recorded and build-submitted readiness distinct. The current snapshot `_built` flag must not become proof that work from an aborted command buffer completed. Either prove submission ordering for reuse or wait for normal completion before admitting a generation to the reusable cache.
4. For changed transforms, deliver the existing full build into new/retired storage as the complete fallback. Investigate refit/update support only after checking actual backend capabilities; do not make native refit a prerequisite for unchanged-scene reuse.
5. Preserve texture transitions even when no acceleration build is needed. Keep immutable generations alive until all readers complete. Bound cached scene generations and clean up without invalidating borrowed owners.

## Verification and exit

Verify static and camera-only frames, animated transforms, reordered collection, visibility/mask changes, material addressing changes, threshold crossings, geometry reload, object removal, empty scenes, and standalone jobs. Include aborted recordings, multiple in-flight slots, and source-scene destruction.

Demonstrate zero recurring TLAS builds and instance uploads for unchanged scene state after warmup. Moving one object must update the scene correctly without rebuilding unchanged BLAS geometry. Exit with a fully functional full-build fallback for every changed-state case, even if backend refit is unavailable.

## Implementation record

Status: source implementation complete; native compilation, probe execution,
validation captures and visual/performance acceptance are pending. No runtime or
speedup result is claimed.

- Immutable acceleration generations retain packed geometry and source buffers,
  BLAS owners, ordered explicit instance keys, instance CPU/GPU storage and TLAS.
  Keys include node/object/submesh correspondence, BLAS identity/build generation,
  packed ranges, material address, world matrix, mask, tracing inclusion and RT
  opacity. Camera data and scalar material revisions do not enter the key.
- Completed matching generations are reused independently of new frame wrappers
  and material generations. Value edits and texture replacement reuse acceleration
  when addressing and opacity remain stable. Changed state always uses new immutable
  storage and the existing full-build path. No BLAS rebuild is requested for transforms.
- GPU recording tickets distinguish a recorded build from submitted/completed work.
  Each native begin creates a ticket; Vulkan/Metal queues attach the existing
  CompletionRecord only after successful native submit. Reuse requires its normal
  completion via nonblocking poll. Abandoned/reset recordings cannot become ready,
  and GPU execution errors propagate. No new frame wait or device drain is added.
- Snapshot build still transitions its own material and shadow images on reused
  TLAS paths. The recording-local built flag remains consumer coordination only.
  Up to eight pending/completed candidates are cached. Publication is staged;
  eviction, clear and empty preparation drop ownership without cleaning retained
  snapshots. Renderer scene attachment clears service cache ownership after its
  existing drain. Callers retain wrappers/sets through every reader's GPU completion.
- Diagnostic setAccelerationReuseEnabled(false) forces fresh instance buffers and
  full builds, independent of the phase 05 material-persistence diagnostic. Current
  Vulkan code uses MODE_BUILD without ALLOW_UPDATE; Metal calls buildAccelerationStructure
  without a refit path or capability contract. No refit is introduced or required.
  Shader ABI and CMake files are unchanged.

Static checks: source consistency of recording tickets across both native begin/
submit paths, explicit key coverage, bounded publication, transition-before-skip
ordering, owner lifetimes and git diff whitespace. No native code or shaders were
compiled, and no application/probe was executed.

`tests/draw/acceleration_reuse/probe.hpp` delivers native fixtures for unrecorded
and abandoned work, completion-based warm reuse, scalar/texture edits, transforms,
threshold crossing, membership/order, material addressing, visibility, diagnostics,
BLAS rebuild generations, geometry reload and source/cache destruction. Existing
probes that prepare snapshots before submitting remain valid: pending generations
are deliberately separate.

Counters: tracing.accelerationGenerationHits, tracing.instanceUploadedBytes,
tracing.TLASBuilds and tracing.skippedTLASBuilds. After a matching generation has
completed, each warm snapshot should report a hit, zero instance-upload bytes,
zero TLAS builds and one skipped build (after texture transitions). Earlier in-flight
warmup frames may allocate/build separate candidates. Report actual hit/miss counts,
scene size and native completion state before interpreting timings.

Pending acceptance: run the new and existing draw probes on Metal and Vulkan;
compare persistence with full-build diagnostics for static/camera scenes, moving
objects and changing materials. Exercise normal acquired-slot retirement with
multiple frames in flight, recording reset, failed submission/device errors,
reordering/removal, mask changes, source destruction and standalone jobs. The
current standard collection emits mask 0xff; its explicit key must still invalidate
if a future/custom collector changes masks. Capture native validation, image output,
exclusive CPU timings, actual build/upload counters and independent GPU timings.
