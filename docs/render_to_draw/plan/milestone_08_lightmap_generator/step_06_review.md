# M08-06 — Review M08 and prepare exhaustive production removal

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08 and all prior product gates.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_08_lightmap_generator/review.md`
- `docs/render_to_draw/plan/milestone_09_cleanup/summary.md`
- `docs/render_to_draw/plan/milestone_10_documentation/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual standalone states/generations, bake effect configuration, scene assembly, pixel/output/overwrite contracts and CLI identities.
2. Compare every planned legacy removal with current engine/app/example/test/script/shader references; update M09 to an actual removal/replacement manifest.
3. Compare public namespaces/types and executable deployment with M10 API/tutorial/app documentation paths. Revise pending M09/M10 steps and root decisions.
4. Explicitly inventory remaining required parity/verification gaps before declaring the old API removable; preserve reference source until that gate is accepted.

## Verification and exit conditions

- The report shows all requested products and a concrete remaining-dependency/removal impact table.
- Cleanup and documentation steps reference real new APIs/targets, including specialized output resources in bake.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09 is a deliberate compatibility-breaking retirement milestone, not an opportunity to retain permanent hidden render shims.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M08-07 — Cumulative documentation](step_07_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
