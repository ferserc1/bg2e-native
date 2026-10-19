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

## M02-03 implemented overlay integration

Use [the actual overlay contract](../milestone_02_manipulation/overlays.md): register a shared
GizmoAndSelectionRenderer in RendererConfiguration.overlays before initialization.
RendererDeferred drives prepare/update/post-final record and cleanup. MeshOverlayPass
provides public shader/graphics replacement and explicit geometric depth/line/blend
policies at output resolution without jitter. Prepare mesh topology before acquisition,
request a scene frame after selection edits, and retain Engine through tool cleanup.
Do not add private renderer target access or draw tools into ordinary scene extraction.

## M02-04 actual material patch handoff

Use [draw MaterialModifier contracts](../milestone_02_manipulation/material-modifier.md): materialDefinition()
is the stored CPU target, while material() aliases live state after loading.
Drawable patches affect both independently; direct MaterialBase patches are live
only. Queue at a stopped boundary, validate weak targets, request redraw and reset
histories even on partial failure. Existing updateTextures flushes pending slots;
callbacks/errors are nontransactional. alphaCutoff is supported by the shared decoder.

## M02-05 actual widget integration

Use [the widget decision table](../milestone_02_manipulation/widgets.md). DrawMaterialEditor binds Engine plus
a shared DefaultRenderLoopDelegate and a typed draw selection (or explicit weak
material), then queues partial edits. DrawTextureWidgets/TexturePreview use the
draw UserInterface session, retain actual GPU snapshots and retire registrations
after completion. No image upload or material setter belongs in UI draw. Use the
safe onChanged(Renderer&) callback for future history invalidation. ComponentInspector
and raw scene/property mutations require the later queued app adaptation; they
are not certified as draw-safe by M02. UV/bake previews remain with M04.

## M02-08 actual review boundary

Use [the M02 technical review](../milestone_02_manipulation/review.md) as the
source of truth: SelectionItem holds weak node/component/high-level draw resource
identity and selection IDs are distinct from render queue/TLAS IDs. DrawMaterialEditor
checks renderer generation, binding revision and selected-resource validity, but
does not make a general ComponentInspector or reflected node setter safe. M06
must queue scene/property edits, validate current-scene membership and document
generation, request redraw/reset after partial failure, and detach widget/tool
resources before UI/Engine teardown. Reloaded/replaced geometry requires fresh
queue/binding snapshots; keep unsaved-document dirty state in the app.

## M03-10 reviewed implementation requirement

Explicitly own RenderSettingsPreferences, connect before first renderer bind, bind/draw/reopen GraphicsSettingsWindow and rebind on document/resource replacement. The M03 omission demonstrates metadata registration alone is insufficient. Map live light properties and local transforms to document IDs; queue edits, invalidate histories including partial failure and mark unsaved state. Example-only lightPose IDs/defaults are not a generic document schema.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Expose the AO-slot interpretation explicitly when editing baked materials. Selecting an ordinary AO map must reset the baked mode deliberately; a missing texture or UV2 selection alone must not enable a lightmap. The current six-slot material supports scalar AO or a baked RGB map in that slot, not two independent sources. Bind bake::UvMapPreviewRenderer output with ui::BakeLightmapPreview::setImage; do not instantiate production UvMapPreview for a draw UI session.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M06-01 UI and stage baseline

The new AppDelegate owns draw RendererDeferred and StageScene; EditorUi currently
shows a startup-model status panel, GraphicsSettingsWindow and UISettingsWindow.
The live facade connects before first attachment, uses preference scope
`draw.model_edit.graphics`, and does not restore implicitly. Camera reset uses
weak stage ownership plus rendererGeneration at a stopped update. Extend these
actual owners with the [inventoried](application-inventory.md) Workspace/ToolBar,
selection/material/submesh/environment workflows; do not copy production Engine
or raw-native callbacks. Startup has no document editing, drop import, pick manager,
overlay or complete widget parity. Keep stage/document/resource generations
distinct from renderer replacement and preserve dirty/error state on partial edits.


## M06-02 concrete IO/Document baseline

Use the existing Document in apps/draw/model_edit/src and the
[complete IO contract](resource-io.md). StageScene owns document.root separately
from floor/camera/light fixtures. Open/save/drop already execute at stopped
boundaries; successful loads advance Document::generation and reattach renderer
catalogs before retired resources are released. Every tool mutation must mark the
Document modified and check weak ownership plus captured document generation;
renderer generation alone does not identify the open document. Save exports live
GPU textures, so edits must keep MaterialBase attributes and stored definitions
coherent. Keep BG2's single-node limitations and use scene saves for graph edits.


## M06-03 delivered implementation

[Editor tool contracts and review handoff](editor-tools.md) records actual stage,
selection/gizmo/widget/action owners, weak document guards, native-free controls,
independent clone/material/geometry workflows and environment presets. See the
[maintained editor guide](../../../../doc/apps/draw_model_edit.md) and
[manual probe/checks](../../../../tests/draw/editor_tools/README.md). Shared input
capture and widget/export fixes are part of this integration. No build/runtime
execution is claimed. Next continuation selects M06-04.


## Project-lead UI parity correction (2026-10-19)

The project lead reported that separate floating dashboard/properties/environment
windows do not match `apps/model_edit` and prevent M06 acceptance. The correction
uses `bg2e::ui::Workspace` with the same main composition: full-width toolbar,
docked Model Properties left, Environment Options right and bottom StatusBar.
File/View/Window menus reproduce open/import/save/quit, camera/highlight actions,
checkable panel toggles and explicit UI/Graphics settings launch. View retains camera reset, framing, best-view and highlight actions. Geometry actions
remain toolbar buttons and use draw-safe queued handlers. Workspace refreshes panel
positions when the UI viewport or scale changes. `draw_model_edit` launches
maximized like the production editor. The main document dashboard is removed;
status and file identity live in the bottom bar.

This correction supersedes the prior floating-window composition described in the
M06-03 handoff. M06-04 source remains implemented, but project-lead acceptance is
held until this prerequisite correction is validated.
