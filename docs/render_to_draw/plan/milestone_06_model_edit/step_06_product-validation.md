# M06-06 — Model editor parity and application packaging checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M06-01–M06-05.

## Scope and deliverable

- `apps/draw/model_edit/`
- `apps/model_edit/`
- `scripts/PACKAGING.md`
- `examples/draw/manipulation/src/main.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/](../../../../apps/model_edit), [scripts/PACKAGING.md](../../../../scripts/PACKAGING.md).

## Implementation work

1. Run the inventory-driven feature checklist against the new editor and relevant production reference behavior with equivalent assets.
2. Verify window lifecycle, native file dialogs under correct platform generator, asset/shader deployment, config import/export, loading/error/unsaved prompts and process cleanup.
3. Exercise raster/no-RT and available RT/FSR modes, safe mesh edits/deep GPU clones, UV/bake/export and viewpoint tools.
4. Record useful semantic differences explicitly; a feature missing from the current copied app cannot be silently deferred to cleanup.

## Verification and exit conditions

- The application is runnable from its bundled/installed layout and required editor workflows have evidence.
- Production editor and new engine examples retain behavior; unavailable backend verification is documented as a gap.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M07 scope should be adapted to actual shared editor widgets/services; M09 product-removal gate relies on this acceptance evidence.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
