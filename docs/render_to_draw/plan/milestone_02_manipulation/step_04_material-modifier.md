# M02-04 — Draw MaterialModifier overloads and safe live edits

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01 Drawable/material contract and M02-01.

## Scope and deliverable

- `lib/include/bg2e/utils/MaterialModifier.hpp`
- `lib/src/bg2e/utils/MaterialModifier.cpp`
- `lib/include/bg2e/draw/MaterialBase.hpp`
- `lib/include/bg2e/draw/Drawable.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/utils/MaterialModifier.hpp](../../../../lib/include/bg2e/utils/MaterialModifier.hpp), [lib/src/bg2e/utils/MaterialModifier.cpp](../../../../lib/src/bg2e/utils/MaterialModifier.cpp).

## Implementation work

1. Add draw material/shared-material/Drawable overloads for indexed/named submeshes and applyAll group-name/regex matching, preserving existing return/count and invalid/no-op semantics.
2. Patch CPU attributes and live draw material independently; note that Drawable::material() currently returns live MaterialBase attributes when loaded, while the stored SubmeshAttributes definitions are private. Add the smallest explicit CPU-definition access/patch seam needed to satisfy both-target semantics; do not mistake two calls to material() for independent CPU/live edits or synchronize a whole material over unrelated runtime changes. Preserve missing/null/wrong-type property policy.
3. Use the target existing engine/cache policy for changed textures. Coordinate producers and waitIdle where resource replacement requires it; update query alpha/material tables and request scene redraw.
4. Retain callback and nontransactional failure behavior unless explicitly revised and recorded. Do not retain target ownership or introduce ResourceManager/catalog infrastructure.

## Verification and exit conditions

- Patch only roughness/alpha/UV or one texture and verify unrelated CPU/live fields remain unchanged; exercise named/group/regex selection and callbacks.
- Cloned Drawable GPU/material objects remain independent after scalar/texture edits, and changed cutoff affects query shadows consistently.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M02-07 demonstrates this API; M03 history invalidation and M06/M07 inspectors/bake apply require its actual edit notification boundaries.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-06 resource handoff

Use the typed draw component path and live `draw::Drawable::material/drawMaterial`; updateTextures/updateMaterials realize pending slot changes synchronously at a stopped-producer boundary. Direct base::Texture settings changes need the material texture setter to invalidate that slot, or explicit Texture::updateSampler/reload. Drawable clones own independent CPU descriptors, GPU images/samplers and built geometry; do not reintroduce mutable cached sharing into clone. See [drawable-resources.md](../milestone_01_deferred_renderer/drawable-resources.md).

## M01-16 reviewed baseline

Use [M01 review](../milestone_01_deferred_renderer/review.md): loaded material() aliases the live attributes; reload preserves existing materials. Texture slot setters mark pending realization, while ordinary scalar setters need no upload. Define the independent CPU/live patch seam here before M06/M07 complete-save workflows rely on it.
