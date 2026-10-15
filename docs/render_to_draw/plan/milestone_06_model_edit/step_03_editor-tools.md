# M06-03 — Stage, selection, materials and widget migration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M06-01/M06-02 and accepted M02 widget decisions.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07 reuses these public widgets/overloads and editing notifications. M09 removes only legacy widget variants whose new callers are proven.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-14 settings versus editor widgets

Reuse [GraphicsSettingsWindow](../milestone_01_deferred_renderer/settings-ui.md) for live draw renderer settings, strict file IO and safe preference restore. This did not migrate legacy TextureWidgets/MaterialEditor/image preview/picking. Evaluate those editor widgets independently with native-free interfaces and completion-safe ownership.

## M01-16 reviewed baseline

GraphicsSettingsWindow/ReflectionWidget::editValue are complete live scalar settings views with queued edits; they do not migrate legacy TextureWidgets/MaterialEditor/UV previews. Audit the production VkDescriptorSet and renderer dependencies in each old widget and use the M02 native-free registration/lifetime contract. Keep the lead-added virtual Window destructor. See [M01 review](../milestone_01_deferred_renderer/review.md).
