# M07-02 — Complete scene graph IO, factories and independent cloning

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M07-01 and M06 complete DB APIs.

## Scope and deliverable

- `apps/bg2e_composer/src/StageScene.cpp`
- `lib/include/bg2e/scene/Node.hpp`
- `lib/include/bg2e/scene/ComponentFactoryRegistry.hpp`
- `lib/include/bg2e/db/scene.hpp`
- `apps/draw/bg2e_composer/src/StageScene.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/StageScene.cpp](../../../../apps/bg2e_composer/src/StageScene.cpp), [lib/include/bg2e/scene/Node.hpp](../../../../lib/include/bg2e/scene/Node.hpp), [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp), [lib/include/bg2e/db/scene.hpp](../../../../lib/include/bg2e/db/scene.hpp).

## Implementation work

1. Adapt scene open/save/prefab and mesh/glTF import to draw high-level loaders and pure schema IO; preserve camera validation and error/relative-path/resource behavior.
2. Keep the existing single editable-root save/load unwrapping rule so repeated round-trips do not nest an extra root each time.
3. Update component factory/deserialization mode handling for draw DrawableComponent and independent CPU/GPU clones, including user component subclass behavior.
4. Complete GPU-only texture/resource export and scene metadata preservation using M06/M05 services; report incomplete resource saves rather than silently omit them.

## Verification and exit conditions

- Repeated save/load retains hierarchy/metadata/resource paths, and unresolved schema IO works without a graphics engine.
- Duplicated nodes independently modify CPU/GPU geometry/materials; originals, controller/component state and saved references remain coherent.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08 prefab/context assembly relies on these root/factory/resource semantics. M09 removes legacy factory modes only after all supported files use draw/neutral paths.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
