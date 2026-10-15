# M09-07 — Review M09 final production API and revise documentation plan

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09 draw-only product evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_09_cleanup/review.md`
- `docs/render_to_draw/plan/milestone_10_documentation/summary.md`
- `docs/render_to_draw/plan/architecture-decisions.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [docs/render_to_draw/plan/architecture-decisions.md](../architecture-decisions.md).

## Implementation work

1. Record final public class/header/namespace, settings/upscaler extension, lifecycle/clone/IO/bake contracts and canonical executable/source locations.
2. Compare final implementation and deletion manifest with every M10 page/tutorial/build/app task; update paths/names/examples/status rather than documenting tentative names.
3. Record native backend exceptions/seams and removed previous APIs; verify no promised old application compatibility reappeared.
4. Update root plan status and decisions while preserving completed historical step intent. Carry remaining verification issues into explicit doc limitations/follow-up items.

## Verification and exit conditions

- A final implementation-to-documentation mapping and updated M10 steps exist, with all supported products and public component extension points accounted for.
- The engine is labeled production draw only after actual acceptance; documentation work has concrete source anchors.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 is the final maintained-doc pass, not a substitute for unfinished removal/product verification.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M09-08 — Cumulative documentation](step_08_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
