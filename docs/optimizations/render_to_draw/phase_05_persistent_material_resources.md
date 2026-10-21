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

## Implementation record

Status: source implementation complete; native compilation, execution and visual
acceptance remain pending. No runtime or performance result is claimed.

- Tracing snapshots retain immutable geometry, material and shadow generations.
  Unchanged ordered material identities/revisions reuse CPU vectors and GPU buffers.
  Alpha metadata independently keys geometry, dense mapping, scalar alpha/UV data,
  classification and realized resources, excluding camera and model transforms.
- Edits allocate immutable buffers. Native image/sampler tuples are shared separately
  when admission capacity, ordered owners/generations and padding fallbacks match;
  unchanged arrays survive scalar edits. Captured native owners are never resolved
  through mutable source wrappers during later binding.
- Invocation set0 and snapshot scene set1 are separate from persistent array set2.
  AO/GI/reflection contracts advance to v3; shadows advance to v5. GLSL declarations,
  layouts and all native consumer bind paths change together. Metal native indices
  and indirect residency behavior remain unchanged.
- Service caches own only the latest generations. Array caches retain at most eight
  layout/capacity/route entries per tuple. Returned alias handles retain native sets,
  tuple owners and layouts independently of eviction, without a reference cycle.
  Empty scenes and clear drop cache ownership; retained snapshots remain usable.
  Cache publication follows successful preparation/set update. A failed preparation
  leaves the previous service generations intact.
- Diagnostic setPersistenceEnabled(false) rebuilds material/shadow generations;
  SoftShadowPass binding diagnostics also create fresh arrays. Instance buffers
  and TLAS still rebuild each snapshot; phase 06 remains separate.

Static review/checks cover source consistency, the three-set ABI, bounded cache
ownership, shader declarations and whitespace. Existing tracing-table, RTGI and
shadow-binding probes were extended/migrated but were not compiled or executed.
The tracing-table probe now builds TLAS snapshots to compare native set identity.

Counters: tracing.materialGenerationHits, tracing.materialUploadedBytes,
shadow.metadataGenerationHits, tracing.persistentArrayBindingHits and
tracing.persistentArrayPreparations distinguish reuse from allocation/upload.
Existing shadowTable.instanceBufferBytes records actual metadata uploads;
shadow.alphaArrayPreparations counts actual array preparations. Warm camera/model
movement should report material/shadow hits with zero corresponding upload/array
preparation work after each active layout is warm. Scalar edits should upload a
material buffer while retaining array sets. Timings and these expectations require
native measurement; no speedup has been measured.

Pending acceptance: run the updated probes and full plan scene matrix on Vulkan
and Metal, compare persistence diagnostics with normal output, verify validation
and captures across slots/resize/aborted recording, and exercise independent object
export/remove/import after source-scene destruction. Include cutoff/texture edits,
overflow membership changes, failed preparation, cache eviction and cleanup with
submitted work retained. Native shader rebuild is required for the new ABI.
