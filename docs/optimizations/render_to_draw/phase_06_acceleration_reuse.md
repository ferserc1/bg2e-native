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
