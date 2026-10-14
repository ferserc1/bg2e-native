# M06 — Experimental model_edit application

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M01–M05 products/reviews; inventory the current application before copying behavior.

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

## Milestone exit gate

- The new application has its own target/bundle/identity and starts without constructing a production engine.
- Model/import/resource IO and complete saves, including generated GPU-only textures, work through the two DB levels.
- Selection/submesh edits/MaterialModifier/texture previews, graphics settings, camera framing and safe mesh operations match current useful behavior.
- UV2, preview, integrated lightmaps/progress/cancel/application/export and unsaved-document behavior work.
- Production model_edit remains usable and M06 review adapts composer, CLI and retirement steps.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
