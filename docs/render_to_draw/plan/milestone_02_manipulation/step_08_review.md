# M02-08 — Review M02 and adapt RT, baking and application steps

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M02 working-product evidence.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

Enter M03 with its advanced-RT decision checkpoint still visible. Do not infer full tracing readiness solely from successful query shadows/picking.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
