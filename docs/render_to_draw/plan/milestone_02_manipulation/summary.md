# M02 — Scene manipulation and material modification

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** M01 accepted working product and review; resolve current Drawable/settings/visitor names from that report.

## Working product

A runnable `examples/draw/manipulation`, derived from M01, with submesh selection, highlights/gizmos and safe material patches through the draw overloads of `utils::MaterialModifier`. High-level manipulation stays in its namespace; production editor paths remain intact.

## Scope exclusions

- No copying model_edit/composer applications or introducing bake/offscreen applications.
- No advanced RT effects, general temporal accumulation or denoise; retain M01 soft shadows and output methods.
- Do not move editor interaction state into GPU or force manipulation into the Renderer base.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M02-01](step_01_dual-manipulation.md) | Dual-mode manipulation objects and scene references |
| [M02-02](step_02_picking-readback.md) | Picking graphics and minimally sufficient GPU readback |
| [M02-03](step_03_gizmos-overlays.md) | Selection highlights, public overlay components and gizmos |
| [M02-04](step_04_material-modifier.md) | Draw MaterialModifier overloads and safe live edits |
| [M02-05](step_05_widget-adaptation.md) | Material/texture/selection widgets by complexity |
| [M02-06](step_06_viewpoint-utils.md) | Experimental optimal-viewpoint tools in utils |
| [M02-07](step_07_working-example.md) | Build examples/draw/manipulation and modifier validation |
| [M02-08](step_08_review.md) | Review M02 and adapt RT, baking and application steps |

## Milestone exit gate

- Picking/submesh mapping, empty/additive selection, input capture and gizmo transforms work with draw components.
- MaterialModifier applies partial scalar/texture/UV patches without overwriting unrelated CPU/live edits; source/clone independence remains intact.
- New utils viewpoint analysis uses draw/GPU while production render::ViewpointAnalyzer remains unchanged.
- Existing production manipulation/material APIs remain callable; the new example is registered and runs independently.
- M02 review updates M03–M10 for actual selection IDs, resource updates and widget decisions.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
