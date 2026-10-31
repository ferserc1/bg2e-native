# Update synchronization

`DefaultRenderLoopDelegate` keeps arbitrary `enqueueRendererUpdate` callbacks conservative. A snapshot containing any such callback drains the device once before applying the snapshot in queue order. This includes mixed typed/generic batches and stale generic callbacks. Callback-enqueued work remains pending until the next `prepareFrame`.

`enqueueFinalColorUpdate(generation, property, value, completion, current)` is a bounded alternative for exposure, contrast, brightness and gamma. It validates finite values/ranges before enqueue, then changes only the standard final pass CPU color tuple at the stopped prepare boundary. Vulkan push constants and Metal `setFragmentBytes` copy that tuple into native command storage during recording; earlier submissions do not read the mutable CPU tuple. It returns false without queueing for unsupported configurations, allowing the caller to use the generic fallback.

The proof covers the exact standard deferred renderer, final pass, temporal accumulator and resampling/FSR upscaler reset implementations. Derived/custom implementations retain conservative callbacks. Changed values keep logical history invalidation. Temporal submitted invocations retain previous/next history owners in completed-slot storage; invalidation clears only the history lookup. The built-in upscaler resets only CPU flags/jitter. Bit-identical values report `Unchanged` and avoid another invalidation. No resource replacement or lower-level synchronization is performed by this path.

Renderer generation, weak target ownership, current final-pass identity and supported reset implementations are checked again at application. An optional read-only CPU validity predicate may reject the update. Completion receives `Applied`, `Unchanged` or `Stale`; it may update UI/persistence or enqueue later work, but must not mutate GPU resources or submit commands. These callback contracts are explicit and do not authorize arbitrary renderer operations without synchronization.

Only adjacent assignments to the same target/property/generation with no completion or validity predicate are coalesced. Any observer, generic callback or different property is an order barrier. `setValueUpdateDrainElisionEnabled(false)` provides a diagnostic baseline that drains typed batches while preserving typed operations and ordering.

`RenderSettingsPreferences::queueSet` uses the typed route for the four proven final color properties on its canonical built-in catalog without extensions. Catalog generation and target identity remain guarded. A supplied completion remains conservative unless the caller explicitly selects `SettingsUpdateSynchronization::ValueOnlyNotification`. `GraphicsSettingsWindow` opts in because its completion only updates UI result/pending state. Validation, stale-target reporting and automatic persistence remain active. Imports, custom catalogs/passes, unknown completions, resource edits and effect/resolution changes retain the generic fallback, except for the separate built-in bloom-only route below.

`MaterialBase::updateTextures` skips native work when all realized source identities/revisions already match, consuming the same pending flags as before. Any actual realization, sampler/image replacement or clone keeps its existing drains. Drawable mesh/material replacement, modifiers, load/reload, scene/renderer replacement, resize and cleanup remain synchronized, with producers stopped by the caller. GPU completion never substitutes for CPU-thread exclusion.

Profiles distinguish typed batch/application/no-op/stale/coalescing counts, conservative callbacks, queued-update drains and lifecycle drains. Material/drawable/texture drains have explicit initiator scopes/counters. Slot acquisition and queue/upload completion remain separate existing scopes. Native verification and steady-state timing evidence are pending; see `tests/draw/update_synchronization/README.md`.

## Bloom-only settings updates

The canonical `RenderSettingsPreferences` catalog also recognizes queued batches
containing only `final.bloomEnabled`, `bloomIntensity`, `bloomThreshold`,
`bloomScatter`, `bloomLevels` and `bloomResolution`. With the built-in bounded final
target and no catalog extensions, it schedules these through
`enqueueCpuValueUpdate`, including bloom-only imports. This route changes only
validated CPU settings and does not invalidate upstream engine/SDK histories.
Pyramid allocation/replacement happens later in `BloomPass::record`, exclusively
for an acquired, completed slot. No submitted descriptor or attachment is rewritten.

Renderer/catalog generations remain guarded and successful edits retain normal
completion/persistence behavior. Unlike `enqueueFinalColorUpdate`, this route has
no `ValueUpdateResult` no-op/coalescing contract. Custom/extended catalogs and
mixed setting imports retain conservative callbacks and history invalidation.
A CPU-value batch mixed with conservative queued operations still drains once;
the diagnostic drain-elision baseline also remains effective. See
[BloomPass](BloomPass.md) and [bloom architecture](../../architecture/draw_bloom.md).
