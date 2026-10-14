# M07 — Experimental bg2e_composer application

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M06 shared editor/resource/IO review and all previous product gates.

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

## Milestone exit gate

- The new composer opens/saves/imports complete scenes with the single-editable-root behavior and required resources.
- Scene-tree/reflected editing/selection/gizmos, graph cloning, materials, camera/environment/light edits and viewpoint helpers work.
- Guarded asynchronous and HTTP/import workflows preserve current request/status behavior without stale scene publication.
- Scene-wide integrated baking, UV preview/output metadata and dynamic graphics configuration work.
- M07 review adapts standalone/batch/retirement/documentation tasks and records current production regressions.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
