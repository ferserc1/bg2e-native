# Phase 05 Persistent material resources

## Outcome

Unchanged tracing material buffers, image/sampler tables, and shadow alpha resources survive across frames even when camera or object transforms change.

## Implementation

1. Split tracing ownership into immutable geometry, immutable material/resource generations, and per-snapshot instance state. Reuse the material generation while its source material identities, effective revisions, admission policy, layout contracts, and capacities are unchanged.
2. Keep table addressing stable within a retained generation. Rebuild or version mappings when membership, classification, texture realization, or capacity admission changes. Instance material indices must always reference the generation retained by that snapshot.
3. Update only dirty material data when safe. Prefer a new immutable buffer generation or retired-slot storage over overwriting a buffer referenced by submitted work. Do not require partial GPU upload support to deliver basic unchanged-buffer reuse.
4. Reuse alpha metadata components that depend only on materials and geometry ranges. Keep dense per-instance mappings correct when the collected instance order changes.
5. Separate reusable image/sampler argument arrays from bindings that also contain a changing TLAS or instance buffer. A resource set may be shared only when its complete resource tuple and layout identity match.
6. Bound retained generations and release them when no snapshot, submission, or export operation needs their resources. Object removal releases cache ownership rather than destroying another object's source data.

## Verification and exit

Verify static scenes, camera-only movement, transform-only changes, scalar edits, texture replacement, cutoff classification changes, object addition/removal, overflow admission changes, failed preparation, and scene destruction with work pending. Export/import must require no renderer table or source-scene library.

After warmup, camera-only and transform-only frames must not rebuild or upload unchanged material/texture arrays. A value edit must not re-encode unchanged texture arrays. Exit with a working snapshot wrapper that may still create a new TLAS each frame; TLAS reuse belongs to phase 06.
