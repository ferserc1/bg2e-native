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

For M02-06 viewpoint framing, use `utils::ViewpointAnalyzer` with the draw Engine and a stopped scene/GPU boundary. The analyzer performs synchronous ID and D32 readbacks; invoke it as an explicit user action, not during frame/UI preparation. Map samples through `analyzedSubmeshes()` and apply a chosen candidate with the draw `applyViewpointToOrbitCamera` overload.

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

## M02-08 actual edit notification

Use the [M02 review](../milestone_02_manipulation/review.md): the material widget
queues weak resource/selection targets and renderer generations, while general
scene/reflection mutation remains unimplemented by M02. Composer edits must also
validate current scene membership and document generation, mark unsaved changes,
and notify render histories, bake jobs, selection and previews after material,
geometry, component or scene replacement. A renderer generation alone does not
protect an async import or document swap.

## M03-10 reviewed implementation requirement

Reuse actual visible settings ownership and synchronized light/material/transform controls. Persist requested values separately from availability/status; remap document light targets and mark unsaved state. Invalidate scene/job/renderer generations appropriately when nodes/resources or output profiles change; direct in-place uploads require explicit history notification.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M06-03 guarded tool baseline

Reuse [actual editor tool contracts](../milestone_06_model_edit/editor-tools.md):
DrawMaterialEditor document guards, stopped sampler edits retaining GPU pixels,
explicit indirect interpretation and completion-retired previews. Scene/property
mutations still need application weak membership/generation guards, dirty/history
notifications on partial failure and tool invalidation before retirement. Shared
input now tracks scene-owned drags; do not recreate native texture registration or
assume generic immediate ComponentInspector mutations are safe. Composer light
limits must follow its own product policy rather than copying the editor's five.


## M06-04 camera framing handoff

The draw model editor provides world-bounds framing plus synchronous, explicitly
requested scored view analysis using `utils::ViewpointAnalyzer(draw::Engine*)`,
`bestSample`, the current vertical FOV and `applyViewpointToOrbitCamera`.
Candidate ID arrays are interpreted through the same analyzer's
`analyzedSubmeshes()` table. Analysis is a stopped GPU action, not a UI draw or
background job; failed/empty/stale results leave the prior camera pose in place.
Composer may adopt these helpers, but must preserve its own stable target, camera
projection and job/scene ownership contracts. See M06
[implementation record](../milestone_06_model_edit/viewpoint-camera.md).

The M06 technical review confirms `DrawMaterialEditor` owns queued material
patches and weak renderer/resource checks, while its application caller owns the
document-generation guard, dirty state, history resets and graph-level mutation.
Reuse the widget and typed selection API; Composer still implements scene-tree,
multi-node selection/edit policy and safe graph transactions. `ViewpointAnalyzer`
borrows draw Engine/resources and runs synchronously with producers stopped, so
scene scope and UI responsiveness remain Composer responsibilities.
