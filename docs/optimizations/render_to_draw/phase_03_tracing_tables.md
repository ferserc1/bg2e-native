# Phase 03 Tracing table construction

## Outcome

Implementation status: implemented and statically checked. Native compilation/execution, visual comparison and performance measurements have not run. Earlier phase acceptance and the shadow v3/indexed-UV entry validation remain pending.

Tracing preparation replaces repeated linear resource searches with indexed lookup and avoids duplicate derived entries for repeated references to the same material entity.

## Implementation

1. Replace repeated mesh, image, sampler, and shadow image/sampler-pair searches with maps keyed by retained resource identity and relevant generations. Keep vectors as the deterministic GPU addressing order; maps supply lookup only.
2. Emit one tracing material entry for each material identity and effective revision encountered in a snapshot. Update each instance's material index accordingly. Different editable materials with equal values remain distinct; no value-based material merging or implicit instancing is introduced.
3. Preserve first-admission order, atomic image/sampler-pair admission, capacities, neutral fallbacks, warning behavior, and deterministic overflow. Do not change raster resources when a tracing table overflows.
4. Keep geometry bases and dense TLAS indices separate from canonical object identities. Use mesh lookup indices to retrieve packed geometry bases without another linear search.
5. Handle empty scenes, omitted transparent objects, reloads, resource replacement, and failed preparation transactionally. Publish caches only after successful construction.

## Verification and exit

Compare packed geometry ranges, per-instance material mappings, fallback decisions, and images with the previous phase. Include repeated references to one material and distinct equal-valued materials. Editing one independent material must not change another entry's source or export state.

Measure lookup counts and table preparation for many unique resources and many repeated references. Confirm fewer material entries only where identity sharing already exists explicitly. Verify nonzero geometry bases, submesh ranges, UVs, and capacity boundaries. Exit with indexed construction used by the renderer and standalone tracing paths, with no dependence on phase 05 persistence.

## Implementation record

- `TracingSceneResources::prepare()` uses snapshot-local lookup maps for immutable DrawMesh identities, image identity/allocation generation, immutable native sampler identity, and material identity/effective revisions. The private `TracingTableLookup.hpp` contains the shared image, alpha-pair and material keys. Owners retain all key resources during construction; no map survives preparation and no value-based material equality is used.
- Mesh collection records each source's unique-mesh index in a parallel vector. Instance packing retrieves geometry bases directly from that index, avoiding a second mesh search. Existing mesh immutability/reload rules remain: a Drawable reload creates a new DrawMesh identity. Geometry vectors retain first-encounter order.
- One derived material entry is emitted for each source MaterialBase identity plus captured value/binding/classification revision tuple. Repeated references reuse its address in `TracingInstanceData::ranges.w`. Independent equal-valued entities remain distinct, and different captured effective revisions of the same entity also remain distinct. Source owners, instance order, canonical object IDs, submesh ranges and dense TLAS IDs remain unchanged.
- Image/sampler lookups decide admission of both resources before inserting either. Vectors remain the deterministic GPU address order; maps are never iterated to construct GPU arrays. Rejected pairs retain both neutral sentinels, with no orphan admission. Per-material fallback totals are accumulated for every referring instance so the existing warning count/rearming semantics remain unchanged.
- Material storage-range validation now applies to emitted unique entries; instance storage-range validation still applies to every instance. Texture capacities, independent shadow admission, and layout capacities are unchanged. Shared GLSL/Metal tracing consumers already resolve materials through each instance's `ranges.w`, so no shader ABI change was needed.
- `buildShadowQueryTable()` uses an indexed image-generation/sampler-pair lookup in both public builder overloads. It preserves the first admitted pair, scalar fallback, instance order and UV metadata, and rejects stale alpha resource snapshots. Both shared renderer preparation and standalone effect preparation use the same indexed builders.
- Geometry cache publication still occurs only after all table buffers, shadow metadata and the scene descriptor have been constructed successfully. Lookup maps and candidate tables are local and discarded on failure. Empty scenes continue to allocate no tracing buffers or TLAS. Transparent exclusion, raster resources, editable materials, serialization/export state, acquired-slot lifetimes and Metal residency behavior are unchanged. No CMake changes or cross-frame material-table persistence were introduced.

## Verification record

Performed: `git diff --check`; source checks for license headers, lexical balance, removal of the targeted linear searches, the material reuse branch before material packing, atomic admission ordering, per-instance fallback accumulation, and geometry publication after buffer construction. Audited shared GLSL/Metal material indexing. A fixed-seed admission model compared 5,000 scenes with varying capacities and shared identities against the previous per-instance algorithm: resource order, per-instance slots and fallback totals matched. This model is algorithm-level evidence, not execution of the native implementation or a performance measurement.

Added `tests/draw/tracing_tables/probe.hpp` and its runner instructions. The native probe covers explicit repeated identity sharing, distinct equal-valued materials sharing native textures, different captured revisions of one entity, transparent omission, dense/canonical identity separation, nonzero mesh/submesh bases, deterministic reconstruction, independent edits, capacity overflow, asymmetric sampler overflow without orphan image admission, shadow pair overflow, failed preparation/cache recovery, texture reload/stale queues, empty scenes and retained snapshots. It has not been compiled or run. The existing shadow-table fixture now sets explicit cutoffs for its overflow casters rather than relying on the pre-phase-01 material default.

Counters: `tracing.meshLookups`, `tracing.materialLookups`, `tracing.materialEntries`, `tracing.reusedMaterialReferences`, `tracing.imageLookups`, `tracing.samplerLookups`, `tracing.uniqueImages`, `tracing.uniqueSamplers`, `tracing.fallbackTextureSlots`, and `tracing.materialBufferBytes`. Existing `shadowTable.alphaTextureLookups` now counts actual map lookup requests (one per alpha-tested instance), replacing its previous linear-search-size estimate; historical values are not directly comparable as element comparisons.

For N admitted instance references and U distinct material/revision keys, expect N mesh lookup requests, N material lookup requests, U material entries, and 6U image plus 6U sampler lookup requests. These count high-level requests, not hash collision probes. Capture many-unique and many-repeated scenes separately, along with table occupancy/capacity, fallback counts, backend and timings as required by the summary.

Outstanding: native probe execution on Metal and Vulkan; reference image comparisons for alpha holes, scalar alpha, UV0/UV1 and scaling, submeshes and capacity boundaries; renderer and standalone shadows/AO/GI/reflections; settings changes, resize, slot reuse, aborted recording, reload, cleanup and independent object import/export; actual counters and exclusive CPU/GPU timings. No native correctness pass, runtime acceptance or measured speedup is claimed.
