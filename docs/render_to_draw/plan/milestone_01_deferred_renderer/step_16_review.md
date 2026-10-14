# M01-16 — Review M01 code and revise all remaining milestones

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01 product checks and actual implementation diff.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_01_deferred_renderer/review.md`
- `docs/render_to_draw/plan/architecture-decisions.md`
- `docs/render_to_draw/plan/milestone_02_manipulation/summary.md`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [docs/render_to_draw/plan/architecture-decisions.md](../architecture-decisions.md).

## Implementation work

1. Follow the root review protocol and record actual public class names, runtime delegate/setRenderer behavior, component mode accessors, deep-clone semantics and cache overloads.
2. Compare actual MRT/capability/subresource/query binding tables with M02 readback, M03 tracing design and M04 UV targets; update their step contracts and source paths.
3. Compare settings codec identifiers/catalog generation, FSR bridge placement and shader layout with M05 headless and M06–M08 applications; revise pending steps rather than leave compatibility guesses.
4. Record all reference-scene restrictions and verified/unverified backend scenarios. Update M02–M10 for intentional changes and missing requirements; do not mark unrun product tests passed.

## Verification and exit conditions

- A review report has evidence and a planned-versus-actual/impact table covering every pending milestone.
- Pending steps use real M01 APIs; first-milestone exclusions are still scheduled in the appropriate later products.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

This is the mandatory handoff into M02. Advanced RT choices remain explicitly deferred until M03-01, even if M01 query tables suggest a candidate approach.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
