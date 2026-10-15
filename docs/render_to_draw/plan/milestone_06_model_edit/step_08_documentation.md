# M06-08 — Update cumulative implementation documentation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M06-07 technical review and the implemented product/corrections it records. This is a separate required final step, not implemented by adding this plan file.

## Scope and deliverable

Update maintained `doc/` API/reference, conceptual, tutorial, application and build/distribution pages affected by M01–M06, including indexes and retained pages with affected links/examples. Produce `documentation.md` in this milestone directory only during execution, mapping changed/retained/retired pages to actual source, examples and validation. Historical handoff/review files do not replace user documentation.

## Milestone coverage: Draw model editor and complete model IO

Actual apps/draw/model_edit executable, resources and build/deployment; model/material/import/save and complete DB IO now delivered; editing/reflection/preview, input/camera/viewpoint and integrated model baking; safe editing/lifecycle/settings. Update shared API/reference/tutorial pages as affected; composer and standalone batch functionality remain future work.

## Implementation work

1. Follow the [cumulative documentation policy](../documentation-policy.md). Read actual source, accepted corrections and this milestone review; update the previous documentation baseline instead of writing a parallel future API reference.
2. Inventory affected maintained pages and reconcile actual signatures, namespaces, lifecycle/ownership, reflected settings/schema, capabilities, extension hooks, examples/assets/shaders and deployment. Update or retire contradictory guidance and navigation.
3. Add usable reference and focused tutorials/application instructions for the functionality delivered so far. Reuse actual source/example call sites; name backend/SDK limits and evidence honestly. No snippets or usage guides for unimplemented later milestones.
4. Check links/anchors, symbol/header/target/resource paths, snippets and diagrams statically. Preserve accurate foundation pages and the current coexistence/production status. Compilation/runtime validation stays with the project lead unless explicitly requested.
5. Record coverage/dispositions, source mappings, checks, limitations and remaining documentation obligations in `documentation.md`; append the documentation handoff to `review.md` without rewriting its earlier history. Reconcile affected pending steps and progress.

## Exit and transition

- Maintained documentation describes the actual cumulative implementation through M06, with no future feature presented as available.
- Relevant API reference, tutorials, app/CLI/build guidance and navigation are updated; all applicable former M10 authoring requirements are covered or explicitly assigned to an unimplemented milestone owner.
- Source/links/snippet checks and actual validation ownership are recorded; no invented compilation/backend coverage.
- Deliver this documentation step for lead acceptance before entering M07. Follow the normal one-step execution protocol. M10 audits the accumulated result and does not defer missing authoring work to itself.
