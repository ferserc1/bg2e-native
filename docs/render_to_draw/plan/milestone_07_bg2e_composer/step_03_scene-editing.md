# M07-03 — Scene editor, reflection, materials and manipulation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-02 and shared M02/M06 widgets.

## Scope and deliverable

- `apps/bg2e_composer/src/SceneEditor.cpp`
- `apps/bg2e_composer/src/SubmeshWindow.cpp`
- `apps/bg2e_composer/src/ToolBar.cpp`
- `apps/draw/bg2e_composer/src/SceneEditor.cpp`
- `lib/include/bg2e/ui/ReflectionWidget.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/SceneEditor.cpp](../../../../apps/bg2e_composer/src/SceneEditor.cpp), [apps/bg2e_composer/src/SubmeshWindow.cpp](../../../../apps/bg2e_composer/src/SubmeshWindow.cpp), [apps/bg2e_composer/src/ToolBar.cpp](../../../../apps/bg2e_composer/src/ToolBar.cpp), [lib/include/bg2e/ui/ReflectionWidget.hpp](../../../../lib/include/bg2e/ui/ReflectionWidget.hpp).

## Implementation work

1. Port scene tree and component inspector actions, adding/removing/duplicating nodes and cameras/lights/environments/controllers with draw geometry access where needed.
2. Use retained high-level manipulation mode overloads and typed selection mapping; preserve node-only scene-tree selection, overlays and input capture.
3. Route material/configuration edits through actual accessors/MaterialModifier/reflection and safe redraw/history/unsaved-state hooks.
4. Integrate utils viewpoint framing and dynamic renderer/upscaler settings import/export. Keep new widget variants distinct where old concrete types are incompatible.

## Verification and exit conditions

- Tree selection, property editing, transform handles and material changes refer to the same node/submesh and preserve clone independence.
- Renderer/settings rebinding and component/scene replacement do not leave UI or selection pointers targeting removed objects.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07-04 async publication and M07-05 bake jobs must update these exact inspector/selection/history hooks.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-14 renderer settings UI

Reuse [GraphicsSettingsWindow](../milestone_01_deferred_renderer/settings-ui.md) and the connected live facade for graphics settings. Full scene/material/texture-preview editors remain separate adaptation work; ResourcePicker only returns native-free file intent. Preserve safe queued edits, generation refresh and weak completions when composing editor lifecycle callbacks.
