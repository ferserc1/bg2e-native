# M04-08 — Update cumulative implementation documentation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M04-07 technical review and the implemented product/corrections it records. This is a separate required final step, not implemented by adding this plan file.

## Scope and deliverable

Update maintained `doc/` API/reference, conceptual, tutorial, application and build/distribution pages affected by M01–M04, including indexes and retained pages with affected links/examples. Produce `documentation.md` in this milestone directory only during execution, mapping changed/retained/retired pages to actual source, examples and validation. Historical handoff/review files do not replace user documentation.

## Milestone coverage: Integrated baking, UV maps and lightmap example

Bake contracts and public draw pipeline reuse; UV2/texCoord1 mapping and surface sampling; accumulation/progress/reset, baked outputs and live-GI exclusion; material application and UV preview/UI; delivered lightmaps example. No future standalone/batch CLI or offscreen product instructions.

## Implementation work

1. Follow the [cumulative documentation policy](../documentation-policy.md). Read actual source, accepted corrections and this milestone review; update the previous documentation baseline instead of writing a parallel future API reference.
2. Inventory affected maintained pages and reconcile actual signatures, namespaces, lifecycle/ownership, reflected settings/schema, capabilities, extension hooks, examples/assets/shaders and deployment. Update or retire contradictory guidance and navigation.
3. Add usable reference and focused tutorials/application instructions for the functionality delivered so far. Reuse actual source/example call sites; name backend/SDK limits and evidence honestly. No snippets or usage guides for unimplemented later milestones.
4. Check links/anchors, symbol/header/target/resource paths, snippets and diagrams statically. Preserve accurate foundation pages and the current coexistence/production status. Compilation/runtime validation stays with the project lead unless explicitly requested.
5. Record coverage/dispositions, source mappings, checks, limitations and remaining documentation obligations in `documentation.md`; append the documentation handoff to `review.md` without rewriting its earlier history. Reconcile affected pending steps and progress.

## Exit and transition

- Maintained documentation describes the actual cumulative implementation through M04, with no future feature presented as available.
- Relevant API reference, tutorials, app/CLI/build guidance and navigation are updated; all applicable former M10 authoring requirements are covered or explicitly assigned to an unimplemented milestone owner.
- Source/links/snippet checks and actual validation ownership are recorded; no invented compilation/backend coverage.
- Deliver this documentation step for lead acceptance before entering M05. Follow the normal one-step execution protocol. M10 audits the accumulated result and does not defer missing authoring work to itself.


## M04-07 reviewed documentation baseline

Use [review.md](review.md), including the accepted missing-metallib correction.
Inventory the existing doc/api/bake pages, draw/BakedIndirectMaps.md, draw/bake/UI
navigation and examples/draw/12_lightmaps guide before adding parallel pages.
Reconcile all current build/distribution pages with both paired native trees and
incremental bundle/install coverage. Explicitly explain CPU UV diagnostic versus
Device-backed bake raster, live/stored material mode metadata, the single AO/map
slot, actual result/CPU/display precision, serial sample completion and context
ownership. Supply an integrated-bake tutorial grounded in the example, including
cancellation/restart/rebind/teardown and unsupported required-job behavior.

Do not teach standalone/batch/image-export or offscreen camera averaging as
implemented. Record the per-page source mapping, retained/updated/displaced page
dispositions, snippet/link/style checks and lead-only runtime evidence in
documentation.md. M05 begins only after this documentation delivery is accepted.


## Delivery

Implemented on 2026-10-18; see [coverage and checks](documentation.md) and the
appended [review handoff](review.md#m04-08-documentation-handoff-2026-10-18).
Existing canonical tutorial retained and checked against the actual example;
missing raw-executor/conceptual coverage added. Awaiting project lead acceptance;
next is M05-01 under the usual continuation convention.
