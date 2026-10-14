# M04-07 — Review M04 reusable maps and revise exports/apps/standalone

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M04 working product and bake evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_04_lightmaps/review.md`
- `docs/render_to_draw/plan/milestone_05_offscreen/summary.md`
- `docs/render_to_draw/plan/milestone_08_lightmap_generator/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual bake public namespace/type/configuration mapping and effect/UV-profile reuse, including limitations per backend.
2. Compare safe UV2 reload, bake target/generation/cancel, pixel formats/dilation and material metadata with M06/M07 editor steps and M08 standalone assembly/writer tasks.
3. Compare GPU readback and result completion with M05 output-file plans; revise any assumed conversion/orientation semantics.
4. Update all pending M05–M10 documents and root decisions for accepted refinements; flag unsupported/unverified bake capability separately from valid no-RT scene display.

## Verification and exit conditions

- The report includes actual accumulated-map/export evidence and impact edits for every pending consumer.
- No bake resource has been misplaced into draw/render/DB solely to simplify an application integration.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M05 starts only with its output/accumulation interfaces matched to current code; standalone lifecycle remains explicit M08 work.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
