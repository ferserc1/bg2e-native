# M07-01 — Inventory and create apps/draw/bg2e_composer

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M06 review.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07-02–M07-06 complete this inventory; M09 promotion keeps surviving target/assets consistent.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-13 renderer settings integration

Read [upscaling.md](../milestone_01_deferred_renderer/upscaling.md). Import/export the live resolution/upscaler catalog using stable implementation identifiers and existing SettingsCodec. Reuse queued pre-acquire resize/rebind/reset behavior; preserve external implementation injection and SDK/backend/hardware status.


## M06-01 startup conventions to reuse

The model editor baseline uses a separate `draw_model_edit` target, typed
MainLoop EngineConfig execution, a unique app ID and a distinct draw preference
scope. Its AppDelegate retains the scene stage, EditorUi holds a weak delegate,
and the live settings facade connects before first renderer attachment. Reuse
that ownership/identity/resource-helper pattern with composer-specific names;
consult the [M06 review](../milestone_06_model_edit/review.md) for actual IO,
draw-clone, widgets, Workspace building blocks, input, viewpoint and integrated
bake services. The startup fixture
and application-inventory are not reusable complete scene import/bake services.

## M07-01 delivery

Created `apps/draw/bg2e_composer` and registered `draw_bg2e_composer` using the
shared SDL bundle helper. App-owned M06 orchestration uses namespace
`bg2e_composer`, independent app/bundle/preferences/environment identities and an
initial editable Scene group. Engine services are reused unchanged. See the
[actual workflow/dependency inventory and lead checks](application-inventory.md).
The baseline does not claim Composer import/server/tree/multi-job parity. No
compilation or runtime checks were executed; lead validation remains pending.
