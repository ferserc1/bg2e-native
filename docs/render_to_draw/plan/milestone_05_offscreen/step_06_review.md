# M05-06 — Review M05 configuration, accumulation and execution contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M05 product/export evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_05_offscreen/review.md`
- `docs/render_to_draw/plan/milestone_08_lightmap_generator/summary.md`
- `docs/render_to_draw/plan/milestone_09_cleanup/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual init/delegate overloads, surface/context ownership, native interop locations and which execution classes are public/internal with rationale.
2. Record precise frame-average versus effect-history/FSR behavior, time/config precedence and readback conversion; update M06/M07 GPU-only texture save steps.
3. Compare the new lifecycle with M08 standalone context/batch needs and M09 canonical default path. Revise M06–M10 for real APIs and verified failure/format/backend behavior.
4. Keep any unverified headless backend/sample mode visible instead of treating a file existing as complete image validation.

## Verification and exit conditions

- The report includes output artifact metadata and actual accumulated-frame evidence, plus changes to every affected pending step.
- No pending application assumes window-only surface access or conflates camera accumulation with UV-space bake accumulation.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06 starts with image/resource export primitives matched to code; M08 remains responsible for standalone bake state/scene-generation handling.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M05-07 — Cumulative documentation](step_07_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
