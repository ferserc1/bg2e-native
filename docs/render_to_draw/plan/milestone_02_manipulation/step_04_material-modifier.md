# M02-04 — Draw MaterialModifier overloads and safe live edits

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01 Drawable/material contract and M02-01.

## Scope and deliverable

- `lib/include/bg2e/utils/MaterialModifier.hpp`
- `lib/src/bg2e/utils/MaterialModifier.cpp`
- `lib/include/bg2e/draw/MaterialBase.hpp`
- `lib/include/bg2e/draw/Drawable.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/utils/MaterialModifier.hpp](../../../../lib/include/bg2e/utils/MaterialModifier.hpp), [lib/src/bg2e/utils/MaterialModifier.cpp](../../../../lib/src/bg2e/utils/MaterialModifier.cpp).

## Implementation work

1. Add draw material/shared-material/Drawable overloads for indexed/named submeshes and applyAll group-name/regex matching, preserving existing return/count and invalid/no-op semantics.
2. Patch CPU attributes and live draw material independently; do not call whole-material synchronization that overwrites unrelated runtime edits. Preserve missing/null/wrong-type property policy.
3. Use the target existing engine/cache policy for changed textures. Coordinate producers and waitIdle where resource replacement requires it; update query alpha/material tables and request scene redraw.
4. Retain callback and nontransactional failure behavior unless explicitly revised and recorded. Do not retain target ownership or introduce ResourceManager/catalog infrastructure.

## Verification and exit conditions

- Patch only roughness/alpha/UV or one texture and verify unrelated CPU/live fields remain unchanged; exercise named/group/regex selection and callbacks.
- Cloned Drawable GPU/material objects remain independent after scalar/texture edits, and changed cutoff affects query shadows consistently.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02-07 demonstrates this API; M03 history invalidation and M06/M07 inspectors/bake apply require its actual edit notification boundaries.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
