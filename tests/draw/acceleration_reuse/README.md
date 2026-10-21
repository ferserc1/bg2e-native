# Completed acceleration reuse probe

Status: source delivered; not compiled or executed. No CMake target is added.
Builds and native execution require explicit user authorization.

Include `probe.hpp` in a native test host and call
`bg2e::draw::tests::runAccelerationReuseProbe(engine)` with an initialized, stopped
Engine and no active frame/background submitters. False means acceleration/array
capabilities are unavailable, not a backend pass. This fixture allocates resources,
records/abandons command buffers and synchronously builds TLAS/BLAS generations;
it does not dispatch shaders or compare rendered pixels.

Checks cover recording versus submission/completion, immutable warm CPU/GPU instance
storage, independent material-buffer edits, texture replacement, changed transforms
without BLAS rebuild, cutoff opacity, membership/order and material addressing,
visibility, diagnostic full builds, native BLAS generation changes, geometry reload,
cache clear and source-owner destruction. Abandoned work never certifies readiness.

Run manual integration on both Vulkan and Metal:

1. Compare a static scene and camera-only movement against
   `TracingSceneResources::setAccelerationReuseEnabled(false)`, through standalone
   passes and RendererDeferred. After a matching native completion, expect zero
   recurring TLAS builds and zero instance uploads with identical images. Phase 05
   material and array persistence remains enabled independently.
2. Use multiple in-flight slots; prepare the next snapshot before the first build
   completes. Pending generations cannot be reused. Retire slots normally, then
   verify completed reuse, texture transitions and descriptors in native captures.
3. Animate one transform and change masks, opacity, collection order, visibility,
   membership, material-address sharing and geometry. Verify correct dense IDs,
   new full builds, stable source BLAS for transform-only edits and retained older
   snapshots. Standard collection currently emits a constant mask of 0xff.
4. Abort a recording, reset/reuse a Vulkan command wrapper, reject an invalid send,
   and exercise GPU execution error handling where available. Old recording tickets
   must not become ready due to another send, frame serial, fence reuse or waitIdle.
5. Exceed eight retained state candidates, remove/replace the source scene, clear
   caches and continue using explicitly retained snapshots through GPU completion.
   Export/import remains source-owned and must not depend on cached scene indices.

Record `tracing.accelerationGenerationHits`, `tracing.instanceUploadedBytes`,
`tracing.TLASBuilds` and `tracing.skippedTLASBuilds` with object/instance counts,
backend, frames in flight, scene changes and completion state. Separate initial
pending warmup from completed steady state. No speedup has been measured.
