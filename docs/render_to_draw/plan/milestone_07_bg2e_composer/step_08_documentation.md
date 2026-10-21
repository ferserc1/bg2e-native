# M07-08 — Update cumulative implementation documentation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M07-07 technical review and the implemented product/corrections it records. This is a separate required final step, not implemented by adding this plan file.

## Scope and deliverable

Update maintained `doc/` API/reference, conceptual, tutorial, application and build/distribution pages affected by M01–M07, including indexes and retained pages with affected links/examples. Produce `documentation.md` in this milestone directory only during execution, mapping changed/retained/retired pages to actual source, examples and validation. Historical handoff/review files do not replace user documentation.

## Milestone coverage: Scene composer, complete scene IO and asynchronous import

Reconcile [active environment synchronization](environment-synchronization.md):
explicit Scene selection including nullptr, no hidden Composer Environment,
fresh-load choice, ignored additions, default after active removal, HDR clearing,
in-place RGB edits and visible generation errors. Preserve Model Edit's independent
workflow and default automatic Scene behavior for clients that do not opt in.

Include the later [16384 device-clamped tracing tables](tracing-capacity-16384.md):
native v2 shader contracts, Vulkan specialization ID0, Metal argument-record
buffers8/9, sampler-budget caveat and warning/neutral fallback. Preserve per-object
material/import/export ownership and do not restore 32/128/256 effect ceilings.
Include SoftShadowPass v2 too: no eight-pair whole-scene fallback; exact capacity
with reserves (2,1,9), Vulkan bindings16/24 and Metal argument records5/6. State that
texture-count reductions remain deferred until after migration.

Apply the later lead-directed unknown-component policy: unregistered JSON types
warn/skip without aborting scene load; registered parser/resource errors are
distinct. Typed save omits skipped components, raw IO preserves unknown JSON.
Reconcile historical strict-rejection wording in maintained usage guidance.

Include the subsequent [UI parity correction](ui-parity-correction.md): left
drawable/material panel, right Scene tree/node inspector, no Environment Options,
registered Add Component dropdown and shared ComponentInspector intent callbacks.
Earlier inherited Model Edit panel/preset descriptions are no longer current
Composer usage. Do not change draw_model_edit's independent environment workflow.

Use the [actual M07 review](review.md), including the lead-checked selection
correction and restored Chain controls. Update SelectableList recipes to honor
bool beginList, root transfer/subclass/custom clone contracts, independent request/
document/renderer identities, joined CPU/HTTP lifecycle and synchronous realization.
Describe per-target preview/temporary source ownership and partial bake material
adoption separately from complete DB save and future M08 batch transactions.
Reconcile doc/apps/composer and draw application navigation without claiming
native package/HTTP/backend coverage that the lead has not specified.

Actual apps/draw/bg2e_composer workflows and build/deployment; complete scene serialization/cloning/resource ownership; asynchronous import/cancellation and safe scene edits; scene tools, viewpoint/reflected settings and scene-wide bake/save flows. Update model editor/shared docs if contracts changed; standalone batch generator remains undocumented until delivered.

## Implementation work

1. Follow the [cumulative documentation policy](../documentation-policy.md). Read actual source, accepted corrections and this milestone review; update the previous documentation baseline instead of writing a parallel future API reference.
2. Inventory affected maintained pages and reconcile actual signatures, namespaces, lifecycle/ownership, reflected settings/schema, capabilities, extension hooks, examples/assets/shaders and deployment. Update or retire contradictory guidance and navigation.
3. Add usable reference and focused tutorials/application instructions for the functionality delivered so far. Reuse actual source/example call sites; name backend/SDK limits and evidence honestly. No snippets or usage guides for unimplemented later milestones.
4. Check links/anchors, symbol/header/target/resource paths, snippets and diagrams statically. Preserve accurate foundation pages and the current coexistence/production status. Compilation/runtime validation stays with the project lead unless explicitly requested.
5. Record coverage/dispositions, source mappings, checks, limitations and remaining documentation obligations in `documentation.md`; append the documentation handoff to `review.md` without rewriting its earlier history. Reconcile affected pending steps and progress.

## Exit and transition

- Maintained documentation describes the actual cumulative implementation through M07, with no future feature presented as available.
- Relevant API reference, tutorials, app/CLI/build guidance and navigation are updated; all applicable former M10 authoring requirements are covered or explicitly assigned to an unimplemented milestone owner.
- Source/links/snippet checks and actual validation ownership are recorded; no invented compilation/backend coverage.
- Deliver this documentation step for lead acceptance before entering M08. Follow the normal one-step execution protocol. M10 audits the accumulated result and does not defer missing authoring work to itself.

## M07-08 source reconciliation

At HEAD `78943d5820e69363c942d71136f479a0df87128e`, current source supersedes the
historical v2 table handoff: AO/GI/reflection use native v3 with Vulkan scene set1,
persistent material arrays set2 (bindings5/6), specialization ID0 and unchanged
Metal record buffers8/9. Direct query shadows use `draw.query.softShadows.v5`,
Vulkan invocation set0, scene set1 and persistent alpha set2 (bindings16/24),
Metal records5/6 and reserves (2,1,10). AO/GI/reflection reserves are respectively
(4,2,13), (5,3,15), (6,3,16). Exact device-clamped capacity up to 16384, neutral
pair overflow and object-local ownership remain. Consume current maintained
[tracing reference](../../../../doc/api/draw/TracingSceneResources.md) and source,
not historical ABI labels. This is static reconciliation, not a new native test.
