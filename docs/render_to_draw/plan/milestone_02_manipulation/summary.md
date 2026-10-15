# M02 — Scene manipulation and material modification

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** M01 accepted working product and review; resolve current Drawable/settings/visitor names from that report.

## Working product

A runnable `examples/draw/11_manipulation`, derived from M01, with submesh selection, highlights/gizmos and safe material patches through the draw overloads of `utils::MaterialModifier`. High-level manipulation stays in its namespace; production editor paths remain intact.

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
| [M02-07](step_07_working-example.md) | Build examples/draw/11_manipulation and modifier validation |
| [M02-08](step_08_review.md) | Review M02 and adapt RT, baking and application steps |
| [M02-09](step_09_documentation.md) | Update maintained documentation cumulatively through M02 |

## Milestone exit gate

- The final [documentation step](step_09_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Picking/submesh mapping, empty/additive selection, input capture and gizmo transforms work with draw components.
- MaterialModifier applies partial scalar/texture/UV patches without overwriting unrelated CPU/live edits; source/clone independence remains intact.
- New utils viewpoint analysis uses draw/GPU while production render::ViewpointAnalyzer remains unchanged.
- Existing production manipulation/material APIs remain callable; the new example is registered and runs independently.
- M02 review updates M03–M10 for actual selection IDs, resource updates and widget decisions.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Reuse the accepted deferred product, typed draw DrawableComponent and live settings; at that handoff selection/manipulation remained unimplemented. M02-01 now delivers the typed mode/reference/schema/gizmo foundation; see [actual contracts](dual-manipulation.md). Picking reuses raw color readback, overlays need an explicit public unjittered surface seam, and preview registration remains separate work. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M02-09](step_09_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).
