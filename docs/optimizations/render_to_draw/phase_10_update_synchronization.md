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
