# Phase 10 Update synchronization

## Outcome

Renderer updates proven to publish immutable data or write only retired slot state avoid unnecessary device-wide drains. Destructive lifecycle changes retain the synchronization their resource ownership requires.

## Implementation

1. Capture a stable profile after settings updates settle. Attribute remaining waits to their high-level initiator and separate queued update batches, scene/lifecycle changes, slot acquisition, uploads, and queue completion. Do not infer a recurring steady-state bottleneck from transition-only captures.
2. Audit `DefaultRenderLoopDelegate::applyUpdates`, material realization, drawable updates, resize, attachment, and cleanup. Document the resources and producer/consumer threads protected by each wait before changing it.
3. Classify update operations explicitly. Existing arbitrary renderer callbacks remain conservative unless their resource contract is known; do not infer safety from a UI label or setting name.
4. Publish value-only changes through the phase 02 revisions and immutable generations or retired-slot buffers. Coalesce redundant compatible updates while preserving operation order, renderer-generation guards, and stale callbacks.
5. For a proven resource-replacement case, retain old owners until normal completion and publish new owners transactionally. Preserve the stopped-background-producer contract of lifecycle and `prepareFrame` operations. GPU retirement alone does not remove CPU-thread ownership requirements.
6. Keep device drains for unknown callbacks, unsupported lifetime cases, destructive changes, and device shutdown where needed. Do not modify lower-level synchronization without demonstrating the complete ownership and queue-ordering proof.

## Verification and exit

Exercise rapid sliders, repeated unchanged assignments, material and texture edits, effect changes, resize, scene replacement, stale queued generations, import/export during normal application operation, long GPU latency, and cleanup. Validate callback order and absence of races or stale resources.

Compare drains per update class and per stable frame, update latency, and renderer output. Demonstrate at least the proven data-only update path without a device-wide drain. If an existing generic callback API cannot express that guarantee, add an explicit bounded data-update path and retain the generic fallback.

Finish with the full application functional and all earlier phase checks still passing. Record unresolved waits by initiator and ownership requirement; do not hide them by removing required synchronization or by adding measurement stalls.

## Wait ownership audit

All operations below require main-thread execution at a stopped boundary; applications must stop background CPU producers and submissions. Device completion alone does not protect CPU object graphs.

| Initiator | Protected ownership / consumers | Phase 10 decision |
| --- | --- | --- |
| `DefaultRenderLoopDelegate::applyUpdates` arbitrary callbacks | Unknown callback can mutate scene, descriptors, images, geometry or pass state consumed by submitted work | Retain one drain before the snapshot, including mixed batches and stale generic callbacks |
| Typed final color values | Four CPU floats copied by Vulkan push constants / Metal fragment bytes during subsequent recording; no submitted command reads the mutable tuple | Omit queued-update drain for typed-only batches with exact supported implementations; preserve logical invalidation and notification order |
| `MaterialBase::updateTextures`, already realized revisions | Source/value revisions match; native owners and callbacks are unchanged | Return after consuming existing pending flags; count realization no-op |
| Material realization / `setTexture` / clone | Native texture/image/sampler owners and source settings may be replaced or cloned; users outside persistent renderer generations are not universally completion-owned | Retain drain; stage existing texture arrays before publication; no replacement retirement elision claimed |
| Drawable load / reload / updateMaterials / setMesh / modifier / clone | Raster buffers, BLAS borrowing geometry buffers, materials and mutable CPU mesh definitions; arbitrary callers can hold borrowed data | Retain all drains and stopped producer contract |
| Texture load / sampler update / clone | Existing realization or sampler may be referenced by descriptors and standalone passes; mutable wrapper and upload ownership | Retain all drains; attribute separately from material/drawable callers |
| Scene / renderer attachment and replacement | Traversal, shared scene components, settings callbacks, environment, descriptor/pipeline/TLAS owners | Retain delegate and renderer lifecycle drains and generation invalidation |
| Resize / surface / graph reconfiguration | Extent-dependent attachments, histories, bindings and pipeline formats; surface slot users | Retain existing drains; value-only updates do not bypass reconfiguration |
| Cleanup / device shutdown | Outstanding submissions and destruction of native owners / worker submission gate | Retain existing drains and caller producer shutdown requirements |
| Slot acquisition / uploads / queue completion | Retired per-slot state, staging lifetime and native queue completion | Unchanged; these are separate from queued-update device drains |

The immutable generations and submitted invocation owners from phases 03–07 remain in force. No new replacement path releases owners early, no measurement waits are added, and no lower-level synchronization is changed.

## Implementation record

Source implementation is complete; native compilation, stable application profiles, long-latency race/validation checks, image comparisons and timing acceptance remain pending. No stable runtime profile was captured because native execution was not requested. Transition-only source evidence is not treated as a steady-state bottleneck measurement.

- Added bounded `enqueueFinalColorUpdate` and exact type/target/reset proof checks. Unknown virtual reset implementations are rejected. Changed values preserve logical invalidation; bit-identical assignments avoid repeated invalidation. Background producers must still be stopped.
- Typed entries share the original ordered snapshot with arbitrary callbacks, generation guards and next-prepare reentrant queue semantics. Adjacent unobserved compatible entries coalesce; callbacks and different properties remain barriers. A diagnostic switch restores typed-batch drains.
- Integrated canonical final color edits through `RenderSettingsPreferences` and the UI's explicit notification-only completion contract. Unknown completions, extensions, imports, material/texture/effect/resolution changes and custom implementations remain conservative. Catalog changes/facade expiration are checked before value application; validation and persistence reporting remain intact.
- Realized material source revisions now avoid no-op texture waits. Actual material/texture/geometry replacements retain their original waits. Added `material.waitIdle.*`, `drawable.waitIdle.*`, `texture.waitIdle.*` scopes and matching `*.drains.*` counters so nested initiators can be distinguished using exclusive times.
- Added `delegate.valueOnlyUpdateBatches`, `delegate.valueUpdates`, `delegate.valueUpdatesApplied`, `delegate.valueUpdatesUnchanged`, `delegate.valueUpdatesStale`, `delegate.valueUpdatesCoalesced`, `delegate.conservativeUpdates` and `material.realizationNoops`. Existing queued-update/lifecycle/slot/native queue counters remain available.

Static checks cover source contracts, canonical property/range correspondence, conditional drain placement, conservative fallback, generation/target/validity guards, callback/coalescing order, retained replacement waits and `git diff --check`. `tests/draw/update_synchronization/probe.hpp` supplies the focused native probe as source only; it was not compiled or executed. The README records the remaining Metal/Vulkan validation matrix, stable-versus-transition profiling procedure and timing gaps. CMake and shaders were not changed by this phase.
