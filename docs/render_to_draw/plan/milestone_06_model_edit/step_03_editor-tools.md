# M06-03 — Stage, selection, materials and widget migration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M06-01/M06-02 and accepted M02 widget decisions.

## Scope and deliverable

- `apps/model_edit/src/SubmeshWindow.cpp`
- `apps/model_edit/src/EnvironmentSettings.cpp`
- `apps/model_edit/src/ToolBar.cpp`
- `apps/draw/model_edit/src/AppDelegate.cpp`
- `apps/draw/model_edit/src/StageScene.cpp`
- `lib/include/bg2e/ui/MaterialEditor.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/src/SubmeshWindow.cpp](../../../../apps/model_edit/src/SubmeshWindow.cpp), [apps/model_edit/src/EnvironmentSettings.cpp](../../../../apps/model_edit/src/EnvironmentSettings.cpp), [apps/model_edit/src/ToolBar.cpp](../../../../apps/model_edit/src/ToolBar.cpp), [lib/include/bg2e/ui/MaterialEditor.hpp](../../../../lib/include/bg2e/ui/MaterialEditor.hpp).

## Implementation work

1. Integrate manipulation mode overloads, typed draw DrawableComponent accessors, optional overlays and MaterialModifier into actual model-selection/edit flows.
2. Port required material/submesh/environment/texture-history widgets by the M02 per-widget rule. Use a new settings widget for dynamic reflected renderer/upscaler options rather than duplicate old hardcoded preferences.
3. Preserve input capture, clone independence, submesh names/groups/visibility and safe waitIdle texture/geometry edits; do not instantiate mesh resources by default.
4. Route edits to unsaved state, redraw/history resets and resource updates consistently. Keep native UI registration in GPU/private UI backend implementation, not application code.

## Verification and exit conditions

- All useful material/texture/submesh/environment workflows operate on new resources, including partial modifier updates and cloning.
- Selection/gizmos/preview handles remain valid across loading, replacement and resize; capability-driven settings agree with object accessors.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M07 reuses these public widgets/overloads and editing notifications. M09 removes only legacy widget variants whose new callers are proven.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
