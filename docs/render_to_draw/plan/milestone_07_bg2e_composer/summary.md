# M07 — Experimental bg2e_composer application

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M06 shared editor/resource/IO review and all previous product gates.

M02 confirms that weak editor selection and renderer-generation checks do not
replace document/scene-generation checks for composer edits or asynchronous
imports. MaterialModifier owns neither dirty state nor baker/history invalidation;
the composer coordinates queued changes, redraw/reset, save state and job validity.
See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `apps/draw/bg2e_composer` reproduces scene composition/editing/import/baking through draw and the new DB/settings/tools APIs. The production composer stays operational through M08 as a behavior reference.

## Scope exclusions

- No premature deletion of production scene factories/apps or loss of scene serialization fields.
- No engine dependence on composer classes, no ResourceManager redesign and no default instancing of duplicated mesh nodes.
- No recreation of renderer/UI/history services already completed in prior milestones.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M07-01](step_01_application-copy.md) | Inventory and create apps/draw/bg2e_composer |
| [M07-02](step_02_scene-io-cloning.md) | Complete scene graph IO, factories and independent cloning |
| [M07-03](step_03_scene-editing.md) | Scene editor, reflection, materials and manipulation |
| [M07-04](step_04_async-import-service.md) | Guarded asynchronous import and server integration |
| [M07-05](step_05_scene-baking.md) | Scene-wide bake jobs, previews and complete saves |
| [M07-06](step_06_product-validation.md) | Composer feature parity and complete-scene regression |
| [M07-07](step_07_review.md) | Review M07 scene/import contracts and revise CLI/cleanup |
| [M07-08](step_08_documentation.md) | Update maintained documentation cumulatively through M07 |

## Milestone exit gate

- The final [documentation step](step_08_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- The new composer opens/saves/imports complete scenes with the single-editable-root behavior and required resources.
- Scene-tree/reflected editing/selection/gizmos, graph cloning, materials, camera/environment/light edits and viewpoint helpers work.
- Guarded asynchronous and HTTP/import workflows preserve current request/status behavior without stale scene publication.
- Scene-wide integrated baking, UV preview/output metadata and dynamic graphics configuration work.
- M07 review adapts standalone/batch/retirement/documentation tasks and records current production regressions.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Consume actual M06 full IO plus M01 raw/reference IO, fresh node IDs/deep resource clones, generation-bearing queued settings and later manipulation. Async adoption needs distinct scene/job validity and resource-reload rebinding; renderer generation alone is insufficient. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M07-08](step_08_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03 reviewed dependency

[M03 technical review](../milestone_03_ray_tracing/review.md) is the actual source/ownership/signal/settings baseline. Its pending-plan impact table and the concrete additions in this milestone's step files supersede historical statements that explicit profiles or effect controls are still unimplemented. Keep exact averaging, chart isolation, required-job capabilities and actual validation evidence distinct from interactive fallback/history behavior.


## M04-06 material/application handoff

The [integrated example](../../../../examples/draw/12_lightmaps/README.md) now
uses explicit MaterialAttributes::IndirectTextureMode (ao/bakedAO/bakedGI in
JSON), linear RGB UV2 maps through the AO slot and an HDR G-buffer multiplier.
Original scalar AO and a baked multiplier do not coexist in the same six-slot
material. Actual image export is still required for GPU-only baked sources;
sampler settings/empty paths alone cannot round-trip a lightmap. Preserve
RGB32F HDR and explicit mode/UV2 metadata in editor/export/batch integration.
See [material API](../../../../doc/api/draw/BakedIndirectMaps.md). Backend visual
validation is delegated to the lead; the upcoming M04 review reconciles the full
pending-consumer impact.


## M04 reviewed dependency

The [M04 technical review](../milestone_04_lightmaps/review.md) records actual
integrated bake/sample/result ownership, RGB UV2 material semantics, preview
implementation and native shader deployment. Its impact table and concrete
pending-step requirements supersede assumptions that M04 already implements a
headless scheduler, batch writer, full GPU-only resource export or camera-image
averaging. Continuation acceptance does not establish hardware-specific numeric,
performance or exported-file evidence. Preserve production references until M09.
