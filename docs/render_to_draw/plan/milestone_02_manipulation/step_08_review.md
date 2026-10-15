# M02-08 — Review M02 and adapt RT, baking and application steps

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02 working-product evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_02_manipulation/review.md`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/summary.md`
- `docs/render_to_draw/plan/milestone_06_model_edit/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual mode overloads, SelectionItem identity, tool attachment/overlay order, MaterialModifier patch/callback semantics and widget migration decisions.
2. Compare material/geometry edit boundaries with M03 history/TLAS invalidation and M04 UV2/bake publication; update those steps to real notifications/lifetimes.
3. Compare GPU readback and new utils analyzer with M05 exports and M06/M07 viewpoint/selection integration; revise pending API paths and validation.
4. Audit production regressions and no-RT/tool behavior, then update all affected M03–M10 documents using the review protocol.

## Verification and exit conditions

- The review distinguishes static/authorized runtime evidence and provides an impact table covering every pending milestone.
- Pending app tasks reference actual overloads/widgets, and no implementation change has silently weakened deep-clone or partial-material guarantees.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

Enter M03 with its advanced-RT decision checkpoint still visible. Do not infer full tracing readiness solely from successful query shadows/picking.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M02-09 — Cumulative documentation](step_09_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
