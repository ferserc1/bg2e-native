# M06 — Experimental model_edit application

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M01–M05 products/reviews; inventory the current application before copying behavior.

M02 establishes `DrawMaterialEditor`/`DrawTextureWidgets`, typed weak selection,
optional output overlays and a synchronous draw viewpoint analyzer. It does not
make generic reflected scene/component mutation safe: M06 must provide its own
queued weak-target and scene/resource-generation checks, unsaved-state and
history notifications, and teardown ordering. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `apps/draw/model_edit` reproduces the current model editor using draw, reflected graphics options, manipulation/material APIs, utils viewpoint tools and bake UV/lightmap resources. The existing apps/model_edit stays functional as reference until M09.

## Scope exclusions

- No modification of legacy ViewpointAnalyzer behavior, no generalized ResourceManager or default mesh instancing.
- No premature removal of production application paths/overloads.
- No experimental-only undocumented reductions in editor features: omissions become explicit incomplete gates.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M06-01](step_01_application-copy.md) | Inventory and create apps/draw/model_edit target |
| [M06-02](step_02_complete-db-io.md) | Complete engine-aware model/scene resource load and save |
| [M06-03](step_03_editor-tools.md) | Stage, selection, materials and widget migration |
| [M06-04](step_04_viewpoint-camera.md) | Utils viewpoint analysis and camera helpers in editor |
| [M06-05](step_05_editor-baking.md) | Model UV2 preview and integrated lightmap workflow |
| [M06-06](step_06_product-validation.md) | Model editor parity and application packaging checks |
| [M06-07](step_07_review.md) | Review M06 application APIs and revise composer/CLI/retirement |
| [M06-08](step_08_documentation.md) | Update maintained documentation cumulatively through M06 |

## Milestone exit gate

- The final [documentation step](step_08_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- The new application has its own target/bundle/identity and starts without constructing a production engine.
- Model/import/resource IO and complete saves, including generated GPU-only textures, work through the two DB levels.
- Selection/submesh edits/MaterialModifier/texture previews, graphics settings, camera framing and safe mesh operations match current useful behavior.
- UV2, preview, integrated lightmaps/progress/cancel/application/export and unsaved-document behavior work.
- Production model_edit remains usable and M06 review adapts composer, CLI and retirement steps.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Start from draw_deferred_renderer/GraphicsSettingsWindow and actual typed resources/loadResources. Complete glTF/MTL/full edited/GPU-only saves and inventory each editor widget. Scalar graphics settings and HDR ResourcePicker do not establish preview/material-editor parity. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M06-08](step_08_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).
