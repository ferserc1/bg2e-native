# Phase 04 Shadow arrays and Metal resource preparation

## Outcome

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
