# M05-06 — Review M05 configuration, accumulation and execution contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M05 product/export evidence.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06 starts with image/resource export primitives matched to code; M08 remains responsible for standalone bake state/scene-generation handling.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
