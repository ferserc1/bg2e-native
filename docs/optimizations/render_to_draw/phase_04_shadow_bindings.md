# Phase 04 Shadow arrays and Metal resource preparation

## Outcome

Implementation status: implemented and statically checked. The native library and
shader interfaces have not been rebuilt or executed. Visual validation, backend
diagnostics and work/timing captures remain pending, as do earlier phase acceptance
checks. Native acceptance requires an explicitly requested build and execution.

Repeated shadow invocations for one snapshot reuse stable alpha resource arrays. Per-invocation inputs and outputs remain correct, while Metal avoids repeated compatible encoder construction and unnecessary native view/sampler resolution.

## Implementation

1. Separate stable shadow scene/alpha resources from changing invocation resources in `SoftShadowPass`. Prefer a dedicated stable resource set and an invocation set; update both native shaders and layouts together if the split changes their interface.
2. Cache stable arrays per snapshot, layout identity, binding set, capacity, and resource generation. Reuse them across opaque and transparent compositions. Retain each invocation's distinct mutable bindings and owners through GPU completion.
3. Preserve device-sized array initialization and fallback contents. Do not shrink arrays without a separately valid layout and shader contract. Occupancy must be measured independently of allocated capacity.
4. In the Metal backend, cache compatible immutable encoder descriptions or safely managed encoders using all layout-relevant properties. Encoders with mutable argument-buffer state must not be shared concurrently without ownership control.
5. Avoid repeated view/sampler resolution for unchanged bindings and repeated fallback entries. Respect image subresource ranges, native generations, and resource validity. Keep indirect texture residency lists complete for every reused argument buffer.
6. Keep Vulkan behavior equivalent and preserve standalone `SoftShadowPass` use. An aborted recording must not publish an incomplete reusable binding set.

## Verification and exit

Use multiple transparent layers, empty/all-opaque alpha tables, capacity overflow, changed layouts, resized targets, multiple frame slots, and scene replacement. Compare shadow images and validation diagnostics on both backends. Verify that reusing array storage never redirects an earlier invocation's inputs or outputs.

Count alpha array preparations, native encoder creations, resolved entries, and encoded entries. Stable array preparation should scale with distinct snapshot/layout/capacity combinations rather than shadow invocation count. Exit with native interfaces rebuilt and validated when authorized, and the complete renderer functional with this optimization alone.

## Implementation record

- The query contract is now `draw.query.softShadows.v4`. Set0 carries invocation
  uniforms, normal/depth inputs, nearest sampler and visibility outputs. Stable
  set1 carries metadata, indexed geometry, TLAS and device-sized alpha arrays.
  GLSL and the C++ layout change together. The Metal signature retains its existing
  native indices; its source documents the logical split. Both shaders must be
  rebuilt together with the library before native execution.
- Tracing snapshots capture native shadow image/sampler owners and allocation
  generations, including fallback resources, once during successful preparation.
  Build transitions those captured resources rather than resolving mutable wrappers.
  `shadowResourceSet()` checks validity/generations before looking up a fully
  initialized stable set by retained layout identity, set and capacity. A changed
  native generation rejects the snapshot; it never rewrites previously prepared sets.
- SoftShadowPass retains a distinct invocation set and managed input/output owners
  for each composition, together with its stable set. Completed-slot retirement
  releases only that slot's invocations. Standalone preparation uses the same path.
  Empty scenes remain inactive; active all-opaque scenes still initialize full
  fallback arrays. Occupancy and allocated capacity have separate counters.
- Stable sets enter the snapshot cache only after every setter and `update()`
  succeeds. Invocation sets enter the slot only when complete. After an aborted
  recording the existing snapshot-discard contract remains mandatory. Diagnostic
  `setBindingReuseEnabled(false)` constructs fresh stable sets per invocation with
  the same v4 shader contract, capacities and ownership, providing an output baseline.
- Metal PipelineLayout caches compatible native encoders per immutable layout/group.
  Layout identity implicitly includes the device and complete binding description:
  set, stage visibility, IDs, counts, types and record/indexability flags. Each state
  has an exclusive mutex held across selecting writable argument storage and encoding
  all entries. Sets retain distinct argument buffers; encoder caching does not share
  writable buffers across concurrent jobs. Layout cleanup drops cached encoders.
- Metal array setters resolve distinct image-generation/subresource-range keys and
  immutable sampler identities once, including repeated fallback entries. Managed
  source ownership prevents pointer reuse from producing false hits. Unchanged arrays
  retain encoded buffers. Updates construct new buffers, complete residency entries
  and owners before replacing the previous successful state. The last encoded owners
  remain retained even if a later candidate setter/update fails. Residency declaration
  on every bind is unchanged. Ordinary fixed arrays and stride-indexed records both
  select their own buffer target before writes.
- No CMake edits, shader capacity shrinkage, changes to material admission/overflow,
  measured waits in the render loop, or cross-frame shadow table persistence were introduced.

## Verification record

Performed: `git diff --check`; source assertions for exact GLSL/layout set-binding
parity, generation validation before cache hits, complete stable-set publication,
per-invocation set separation, exclusive encoder use, retained residency generation,
lexical balance and GPL headers. Audited native compute binding of both sets and
the independent direct Metal texture/buffer namespaces. These are static checks.

Delivered `tests/draw/shadow_bindings/probe.hpp` and a backend/integration checklist.
The probe covers stable cache identity, layouts/capacities, failed creation recovery,
full fallback initialization, independent native argument buffers, retained source
owners, stale-resource rejection, and Metal unchanged-array/residency behavior.
It has not been compiled or run. Multi-composition image comparison, subresource
recreation, concurrent encoding, aborted recording, resize, slot reuse and scene
replacement remain integration checks rather than claimed probe results.

Counters: `shadow.alphaArrayPreparations`, `shadow.stableBindingHits`,
`shadow.alphaArrayOccupiedSlots`, `shadow.alphaArrayCapacitySlots`,
`shadow.invocationBindings`, `shadow.geometryBufferBindings`,
`metal.argumentEncoder.creations`, `metal.argumentEncoder.cacheHits`,
`metal.argumentArray.resolvedImageEntries`, `metal.argumentArray.resolvedSamplerEntries`,
and `metal.argumentArray.encodedEntries`. The existing encoder timing region becomes
`metal.argumentArray.getEncoder`; creation counts now distinguish misses from hits.
Resolved counts measure unique native resolutions; encoded counts still include
all initialized slots, including fallback slots.

Expected work: I invocation sets for I compositions, but stable arrays prepared
only for K distinct snapshot/layout/set/capacity combinations. Encoder creation
scales with layout argument groups; native resolution scales with unique retained
views/samplers, while all capacity entries are encoded once per prepared stable set.
Actual counts, exclusive CPU timings and separate GPU execution measurements have
not been captured. Metal and Vulkan native acceptance are both pending.
