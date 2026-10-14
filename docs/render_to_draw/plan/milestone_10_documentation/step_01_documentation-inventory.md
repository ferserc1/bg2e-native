# M10-01 — Inventory every doc page against final source

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M09 review.

## Scope and deliverable

- `doc/`
- `doc/api/`
- `doc/tutorials/`
- `doc/apps/`
- `docs/render_to_draw/plan/milestone_09_cleanup/review.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/](../../../../doc), [doc/api/](../../../../doc/api), [doc/tutorials/](../../../../doc/tutorials), [doc/apps/](../../../../doc/apps).

## Implementation work

1. List every maintained document and classify update/keep/remove/replace, including API indexes/reference tables, tutorials, conceptual pages, application docs and build/distribution instructions.
2. Map final source headers/public classes/targets/config schemas from M09 to their documentation owners. Use actual code and example call sites, not tentative plan names.
3. Identify old render/scene::vk/scene Drawable/native callbacks, forward/MSAA and experimental-opt-in guidance to remove or replace.
4. Keep unchanged accurate foundation pages, but review their dependency/examples/cross-links; do not claim all-doc review after changing only draw pages.

## Verification and exit conditions

- The inventory covers every current doc file and every newly public draw/pipeline/environment/bake/utils component.
- Each obsolete page has a concrete disposition and final navigation destination.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M10-02–M10-05 execute this map; final review must include pages kept unchanged and why they remain accurate.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
