# Phase 03 Tracing table construction

## Outcome

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
