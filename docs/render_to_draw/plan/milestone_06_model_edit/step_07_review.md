# M06-07 — Review M06 application APIs and revise composer/CLI/retirement

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M06 parity/product evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_06_model_edit/review.md`
- `docs/render_to_draw/plan/milestone_07_bg2e_composer/summary.md`
- `docs/render_to_draw/plan/milestone_08_lightmap_generator/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual complete load/save/factory overloads, GPU-only texture export, widgets/settings identities and model/bake/viewpoint workflows.
2. Compare shared services with composer scene/async/import/graph clone needs; update M07 files to real APIs and tasks already completed.
3. Compare output metadata/preflight/save responsibilities with M08 standalone bake and M09 surviving app/default paths. Update M07–M10 accordingly.
4. Include feature-by-feature parity and production regression evidence; list any required architecture changes separately from routine application adaptations.

## Verification and exit conditions

- The report and actual edited pending steps cover all shared and application-specific differences.
- No core API dependency on apps/draw/model_edit was introduced to implement reusable resources.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

Composer migration begins from the reviewed application/resource architecture, not a second independent adaptation of old engine calls.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M06-08 — Cumulative documentation](step_08_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
