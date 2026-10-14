# M07-01 — Inventory and create apps/draw/bg2e_composer

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M06 review.

## Scope and deliverable

- `apps/bg2e_composer/src/AppDelegate.hpp`
- `apps/bg2e_composer/src/StageScene.hpp`
- `apps/bg2e_composer/src/SceneEditor.hpp`
- `apps/bg2e_composer/CMakeLists.txt`
- `apps/draw/bg2e_composer/`
- `apps/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/AppDelegate.hpp](../../../../apps/bg2e_composer/src/AppDelegate.hpp), [apps/bg2e_composer/src/StageScene.hpp](../../../../apps/bg2e_composer/src/StageScene.hpp), [apps/bg2e_composer/src/SceneEditor.hpp](../../../../apps/bg2e_composer/src/SceneEditor.hpp), [apps/bg2e_composer/CMakeLists.txt](../../../../apps/bg2e_composer/CMakeLists.txt), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt).

## Implementation work

1. Inventory composer-specific workflows beyond model_edit: editable scene root, components, scene-tree, imports/server, multiple targets, save prompts and lightmap jobs.
2. Create the new application target/bundle/resource identity and wire runtime draw Renderer/delegate and shared reflected settings/widgets.
3. Preserve old application/source and apply reviewed common services from M06 rather than clone new library implementations into the app.
4. Map every production engine/Drawable/material/selection/bake/viewpoint dependency to an actual replacement or concrete missing task.

## Verification and exit conditions

- The new composer starts and displays its editing stage using draw only.
- The feature matrix captures scene/import/job behaviors, not only toolbar/window appearance.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M07-02–M07-06 complete this inventory; M09 promotion keeps surviving target/assets consistent.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
