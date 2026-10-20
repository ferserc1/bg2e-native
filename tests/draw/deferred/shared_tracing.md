# Shared per-frame tracing resources

Manual regression coverage for `draw::RendererDeferred`, on Vulkan and Metal.
No build or GPU execution is implied by this checklist.

1. Load an unchanged scene with at least eight RT-enabled objects and a
   shadow-casting light. Enable query shadows, GI and reflections together.
2. Capture a frame with the backend GPU debugger/profiler. Expect one TLAS build,
   before the layers, and the same TLAS handle in all effect invocations and both
   deferred composition layers. GI and reflections must bind the same vertex,
   index, instance and material buffers. Shadow instance/triangle buffers belong
   to the same frame snapshot.
3. Capture the following frame. Expect another TLAS build and fresh instance,
   material and shadow table buffers, even though the scene has not changed.
   Immutable packed vertex/index buffers may be reused, as before this change.
   Enable CPU profiling with transparent submeshes present: `tracing.getSceneBindings`
   should count every GI/reflection invocation, but `tracing.createSceneBindings`
   should count only the distinct layouts used by those effects in that snapshot
   (normally two per rendered frame with GI and reflections enabled). It must not
   scale with the number of transparent compositions. Image-view/sampler resolution
   and argument-array encoding for scene sets should follow cache misses, not hits.
4. Switch GI to AO, toggle reflections and shadows independently, and disable all
   RT effects. Expect one build with any active consumer and none with no active
   consumers. Re-enabling an effect must not produce a snapshot profile mismatch.
5. Compare images with the standalone effect probes, including multi-submesh
   objects, filtered non-RT objects, transforms and transparency. Filtered object
   IDs must not be mistaken for dense TLAS indices. No alpha-test behavior changes
   are intended.
6. Exercise several frames in flight, scene replacement, resize and cleanup with
   validation enabled. No in-flight resource is overwritten or released early.
7. Compare CPU preparation and GPU build times against production at identical
   scene, resolution and effect settings. Keep per-frame table/TLAS reconstruction
   enabled in both paths; do not introduce refit or unchanged-scene caching.

`tests/draw/rtgi/probe.hpp` additionally checks fresh per-frame table/TLAS ownership,
shared geometry and matching dense shadow/material/TLAS instance order. It checks
that 23 repeated requests reuse one ResourceSet, distinct layouts have distinct
cached sets, and the next snapshot does not reuse the previous snapshot's set.
