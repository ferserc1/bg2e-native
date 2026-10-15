# M03-10 — Review M03 backend contracts and prepare bake/headless reuse

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03 product and checkpoint evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_03_ray_tracing/review.md`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/rt-design.md`
- `docs/render_to_draw/plan/milestone_04_lightmaps/summary.md`
- `docs/render_to_draw/plan/milestone_05_offscreen/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Compare final code with the accepted RT design: shader stages, table layouts, non-opaque candidates, limits/capabilities and backend differences.
2. Verify public effect/surface/configuration/shader reuse and map actual types to bake UV passes/context in M04/M08; revise those steps wherever assumptions were wrong.
3. Compare temporal sample/reset/motion behavior and settings codec with M05 accumulation/export and M06/M07 live editing. Update all pending M04–M10 contracts.
4. Record image/behavior evidence and gaps per backend, and list improvements intentionally not implemented rather than silently call them parity.

## Verification and exit conditions

- The report links implemented backend code and explains actual history/denoise/cutout behavior with product evidence.
- Pending bake/headless/app steps consume real interfaces and account for all missing/approved changes.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

No baking work starts with a vague promise that camera passes are reusable; the review must name the compatible input profiles or schedule concrete adaptations.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M03-11 — Cumulative documentation](step_11_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
