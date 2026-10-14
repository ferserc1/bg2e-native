# M06-07 — Review M06 application APIs and revise composer/CLI/retirement

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M06 parity/product evidence.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

Composer migration begins from the reviewed application/resource architecture, not a second independent adaptation of old engine calls.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
