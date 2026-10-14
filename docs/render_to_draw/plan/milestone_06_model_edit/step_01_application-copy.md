# M06-01 — Inventory and create apps/draw/model_edit target

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M05 review.

## Scope and deliverable

- `apps/model_edit/src/AppDelegate.hpp`
- `apps/model_edit/src/main.cpp`
- `apps/model_edit/src/StageScene.hpp`
- `apps/model_edit/CMakeLists.txt`
- `apps/CMakeLists.txt`
- `apps/draw/model_edit/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/src/AppDelegate.hpp](../../../../apps/model_edit/src/AppDelegate.hpp), [apps/model_edit/src/main.cpp](../../../../apps/model_edit/src/main.cpp), [apps/model_edit/src/StageScene.hpp](../../../../apps/model_edit/src/StageScene.hpp), [apps/model_edit/CMakeLists.txt](../../../../apps/model_edit/CMakeLists.txt), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt).

## Implementation work

1. Inventory menus/workflows/widgets/resources/current rendering assumptions, including Document, StageScene, SubmeshWindow, EnvironmentSettings and ModelLightmapWindow.
2. Create the new application directory/target and adapt its delegates to runtime draw Renderer ownership through the existing MainLoop EngineConfig path.
3. Keep production source/target intact. Use unique runtime target/app identity/resource paths during coexistence and package new paired engine shaders.
4. List every old renderer/material/widget call and assign its replacement or a concrete missing-feature task; do not declare parity from a working window.

## Verification and exit conditions

- The new executable opens an editor window and scene using draw without legacy Engine construction.
- Feature mapping accounts for all current UI and safe-loading behavior, with any required API additions assigned to subsequent steps.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06-02–M06-06 complete this feature matrix. M07 follows the actual application startup/target/persistence conventions; M09 promotes surviving targets.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
