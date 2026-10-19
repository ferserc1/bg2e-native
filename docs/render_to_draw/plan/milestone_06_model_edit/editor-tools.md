# M06-03 editor tools implementation and review handoff

Delivery date: 2026-10-19. Starting/finishing HEAD remains
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`. Inherited/staged work is preserved;
no commit/reset/staging, CMake edit, configuration, compilation, shader command,
application run or C++ probe execution was performed. M06-02 is accepted by the
project lead's continuation instruction; backend/scenario coverage is unspecified.

## Actual application owners

- AppDelegate retains a draw SelectionManager and optional shared
  GizmoAndSelectionRenderer in RendererConfiguration.overlays. RendererDeferred
  owns overlay initialize/attach/prepare/resize/record/cleanup. StageScene owns a
  Scene catalog for picking/camera interaction, separate document root and editor
  camera/floor/fill fixtures. Only document nodes receive Selectable/Gizmo helpers;
  runtime helpers are omitted from export. Shared imported drawable/CPU mesh
  identities are cloned during tool preparation so ordinary node edits do not
  introduce mesh instancing.
- EditorActions.cpp owns queued selection, node metadata/transforms, geometry,
  independent node duplication, material copy/paste/JSON patches, environment
  transactions, stage floor/fill/camera controls and dirty/history notifications.
  Weak delegate/node/resource ownership plus document/renderer generations guard
  delayed work; current document membership is checked before mutation. Whole-mesh
  modifiers deduplicate selected mesh identities and rebuild uploaded/tracing data.
  Partial edit failures still mark dirty, redraw and invalidate histories.
- EditorUi/EditorTools.cpp use public UI wrappers for properties, submesh lists,
  additive selection, group/visibility edits, geometry buttons, environment/light
  controls, File toolbar/shortcuts, panel reopen and live Graphics settings. No
  application ImGui/Vulkan/Metal/SDL headers or texture registration were added.
  Node names/transforms and per-submesh metadata remain separate from material
  patches. Child removal retains the document root. Duplicating a root creates a
  group with two independent copies and advances document generation.
- DrawMaterialEditor follows typed selection and the draw UI session. The app
  invalidates its binding on selection changes and installs a captured document
  generation guard. onChanged updates dirty state and histories even after partial
  failure. Clipboard snapshots deep-copy actual GPU textures, including GPU-only
  maps, while sampler-only edits retain each target's authored image/mips.

## Shared API refinements

`DrawMaterialEditor::setEditGuard` supplements existing binding/renderer/resource
checks. `queueTextureSettings(slot, edit)` edits only requested settings fields,
retains actual image resources and updates the matching stored slot. Color,
filters, U/V addressing, mip sampling and LOD bias have actual controls.
`queueIndirectMode(mode)` requires an assigned baked map, clones actual pixels,
sets linear interpretation and UV set 1, and synchronizes stored/live metadata.
The UI deliberately resets AO interpretation when selecting an ordinary AO image
or clearing the AO slot. Sampler/color edits cannot make a baked map nonlinear;
UV selection by itself does not enable baking. M06-05 still owns bake jobs/UV2
regeneration/preview, not this material interpretation UI.

InputDelegate now has optional mouseInputCaptured(). InputManager tracks scene-
owned mouse presses, forwards their movement/releases across UI regions, and
rejects a UI-owned release as a scene click. SelectionManager ends capture on
deselect and requires an accepted left press before click picking. Other-button
releases cannot terminate an active left-button gizmo. Existing production users
keep their default capture hook; the common router fix needs regression checks.

Complete draw export now ignores ignored runtime helpers in one-model BG2
validation and rejects unresolved dirty texture slots. It must not export old
realizations as if a failed new texture request had succeeded. Raw/reference IO
and production overloads retain their separate contracts.

## Environment and publication

Environment UI uses the public deferred EnvironmentGenerator service at the
stopped boundary before graph adoption; failed generation retains its published
GPU snapshot. Document sky settings and five editable document lights are distinct
from floor/fill/camera fixtures. Controls cover light type/color/intensity/shadows,
source size, spot angle/cutoff in degrees, samples/reflection participation and
node transform/gizmos. Existing imports are not truncated to five lights.

Environment preset scenes contain Environment, independent world-space light
poses, floor state and a fill toggle. Schema/component/image/limit/singular-root
validation and GPU generation precede adoption. A restored lighting rig compensates
for the document root transform. Old unrelated nodes/custom content remain;
former light nodes can remain as transform nodes. Native resource publication
uses M06-02 complete IO. The explicit personal default uses
`settingsPath()/draw_model_edit_environment.json`, never production env.json.
Graphics preferences remain draw.model_edit.graphics and are edited through the
existing dynamic facade/window, including its explicit persistence/import controls.

Queued edits flush before Save snapshots or Open/close/Quit dirty decisions.
Candidates realize tools before replacing the open model. Document replacement
clears selection/capture and camera press state; old generations reject delayed
widget/actions even if old nodes remain alive. Reattachment clears renderer queues
before retired resources are cleaned. Widget bindings/previews detach before
renderer, stage tools and Engine teardown; cleanup preserves and rethrows the
first error after continuing releases.

## Evidence and next owners

Static GPL/header, opening-brace and lexical delimiter checks passed on 19 owned
C++ files, including the new manual probe. Native application include boundaries,
API signatures/call sites, weak targets/generations, partial updates, actual image
ownership, IO order, metadata and teardown were inspected. Markdown paths/fences
and working-tree/index whitespace checks pass. This is not compiler or runtime
verification. No production application implementation or CMake file was changed.

The [manual probe and interaction matrix](../../../../tests/draw/editor_tools/README.md)
cover partial patches, stored/live agreement, sampler image retention, independent
clones/geometry and stale generation rejection, plus picking/capture/DPI, presets,
failed images, pending-save/quit and production input regressions. The
[maintained editor guide](../../../../doc/apps/draw_model_edit.md) records usable
controls and format/lifecycle boundaries.

M06-03 and its Workspace parity correction are accepted by project-lead
continuation; M06-04 camera analysis is also accepted by continuation. M06-05 now
owns the integrated UV2/bake utility and source invalidation, with its handoff in
the milestone summary and step file. M06-06 owns cumulative parity/runtime/
packaging checks. M07 can reuse guarded widgets and application edit patterns;
M09 must prove new callers before retiring production widgets.
