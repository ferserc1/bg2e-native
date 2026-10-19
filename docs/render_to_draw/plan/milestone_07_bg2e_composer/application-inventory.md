# M07-01 Composer application inventory

Current reconciliation: [M07-06 product validation](product-validation.md) maps
all workflow families to implementation and lead checks. The original table below
is the historical M07-01 gap inventory. M07-06 also restores Chain component
add/remove controls through guarded queued edits; actual native/package coverage
remains unspecified.

[Step](step_01_application-copy.md) · [Summary](summary.md) · [Progress](../progress.md)

## Delivered foundation

`apps/draw/bg2e_composer` is an app-owned copy of the accepted M06 editor
foundation, in namespace `bg2e_composer`. It reuses engine implementations rather
than copying renderer, DB, selection, reflection, material, viewpoint or bake
services into new libraries. App-owned orchestration is intentionally separate
so Composer can evolve without changing Model Edit behavior. The legacy Composer
sources/target remain intact.

`main.cpp` selects a typed draw EngineConfig through MainLoop, accepts
`--backend vulkan|metal`, wires render/input/UI delegates and a guarded close
callback, and requests a maximized window. `draw_bg2e_composer` has its own app ID,
macOS bundle identity, graphics preference scope, environment default path and
temporary bake prefix. The SDL bundle helper deploys shared engine shaders and
assets. No production app shaders are copied.

AppDelegate owns StageScene and the renderer overlay/selection. EditorUi holds a
weak delegate and connects the live RenderSettingsPreferences before the first
renderer attachment. GraphicsSettingsWindow and RendererDebugWidget share the
live facade; diagnostic state remains transient. Resolution/upscaler persistence
uses the existing catalog/codec and stable implementation IDs, with queued
renderer updates inherited from M06. External upscaler injection and runtime
availability reporting remain engine responsibilities.

StageScene retains one document subtree (`Scene`, initially containing a cube)
inside a container holding editor camera/floor/fill-light fixtures. Startup now
assigns Document::root and generation 1 and prepares selection resources, so
editing and complete save work before the first file open. The initial fixture
is clean with no source/save path. The inherited dirty/generation checks, pending
IO adoption boundary, renderer reattachment, retired-resource cleanup and tool
teardown remain in place. This establishes a baseline, not complete scene-camera
or component-factory validation.

## Production workflow comparison

| Production source / behavior | Delivered baseline or actual replacement | Remaining owner |
|---|---|---|
| AppDelegate: production Engine/RendererDeferred, Vulkan frame types | draw::DefaultRenderLoopDelegate, RendererDeferred, typed EngineConfig and FrameContext; no direct Vulkan frame API in app sources | M07-06 backend/product checks |
| StageScene: containerRoot/editableRoot, scene open/save, main camera/projection, close and unsaved prompts | Document root separated from fixtures; engine-aware draw loadScene/saveScene/storeDrawableBg2; guarded replacement and complete GPU resource export | M07-02 camera adoption/projection, root round-trips, prefab/import/export-selected and new/close semantics |
| Document: path/name/modified state | App Document tracks root, sourcePath, savePath, dirty and generation | M07-02 reconcile Composer file/status semantics |
| SceneEditor: SceneTree, NodeEditor, reflected components, Chain controls | Scene Properties uses inherited guarded node/submesh/material controls; shared reflection wrappers available | M07-03 actual tree/NodeEditor, component add/remove, Chain and custom components |
| ToolBar: Scene node creation under selection, camera-relative placement, duplicate/remove | Inherited geometry and node editing; independent draw Node/Drawable clones; light-only gizmos | M07-03 selection-parent insertion, scene node menu, placement, cameras/environment gizmos and highlight modes |
| SubmeshWindow/material editing | DrawMaterialEditor and MaterialModifier, explicit indirect texture interpretation and live previews | M07-03 Composer panel/selection parity |
| StageScene export selected models (single file or multi-selection folders, no child traversal) | Complete draw resource writers exist; no Composer selected-export action yet | M07-02 |
| File imports: BG2 insertion and glTF subtree import | Open BG2/glTF/OBJ replaces document; labels explicitly say Open | M07-02 insertion/units/coordinate transforms and resource semantics |
| asyncLoadGuarded, loader UI, safe scene swaps | Synchronous queued IO at stopped renderer boundary, weak document/renderer guards | M07-04 asynchronous work, request/scene generations, cancellation/error UI and late-result rejection |
| ImportServer: POST /import, GET /status, occupied request slot and fulfil | Not started; server is not bound by the new app | M07-04 service/settings/stop-join and request outcome parity |
| SceneImporter: canonical path table, weak node tracking, reimport wrapper and parent | No app importer table yet | M07-04 retain identifiers/parent, unitsScale, YUp/ZUp wrapper, stale scene/request rejection and table clear |
| ImportSettings/Window: enabled setting, port default 8643/range 1024–49151 | No import preferences are read or changed | M07-04 distinct scope and UI; avoid both apps binding the same port unexpectedly |
| StageScene viewpoint camera analysis | utils::ViewpointAnalyzer and applyViewpointToOrbitCamera, stopped update with document/renderer/target validity | M07-03 Composer active camera and target semantics |
| SceneLightmapWindow and multi-target LightmapBakeJob vector | M07-05 serial selected-subtree/scene scheduler retains all outputs before staged texture preparation and material adoption; target-specific previews and complete-save sources | M07-06 runtime AO/GI, progress/cancel, invalidation and complete-save regression; see scene-baking.md |
| RenderSettingsPreferences and production settings UI | draw live settings/preferences, GraphicsSettingsWindow plus separate RendererDebugWidget | M07-06 persistence/backend/SDK fallback checks |
| app resources/distribution | Separate target uses shared SDL bundle deployment; distribution collection recursively discovers registered executable targets | M07-06 launch/resource/package verification |

## Dependency and ownership contract

Production Engine/Renderer/material types map to draw::Engine/RendererDeferred/
Material; production Vulkan meshes/textures map to draw::Drawable/Texture over gpu.
Selection uses the typed draw SelectionManager and GizmoAndSelectionRenderer.
Raw/schema/reference IO remains graphics-free; complete scene/model IO takes an
explicit draw Engine and materializes GPU-only HDR/mip resources via `.bg2tex`.
Node::clone/Drawable::clone are the independent resource path; factory/custom
component coverage is a M07-02 check, not established by this copy.

IntegratedBakerContext borrows the live Engine, permits one live baker and one
in-flight sample, and is released after recorded work retires. It is not the
Composer multi-target scheduler. No legacy standalone or production baker is
instantiated. Source edits/document replacement invalidate the inherited job;
M07-05 must extend validity to every target and request. No new gpu dependency,
engine implementation, permanent compatibility adapter or ResourceManager is added.

## Verification and lead checklist

Agent inspection: new files preserve the complete GPL header; private includes
resolve locally; application sources contain no production render includes/types,
Vulkan frame types, direct ImGui use or old Model Edit identity. Registration and
shared resource-helper paths were inspected. Whitespace checks pass. These are
nonexecuting checks; no configure/build/shader/runtime/package command was run.

Project lead checks after compilation:

1. Launch draw_bg2e_composer on applicable Metal/Vulkan backends and confirm the
   initial cube, floor, camera, selection and Workspace panels render.
2. Check --help/backend selection, window/target identity and independent graphics
   preferences alongside draw_model_edit and legacy bg2e_composer.
3. Edit the initial scene, save as a scene document, reopen it, and exercise
   Save/Discard/Cancel on replacement/quit. Confirm resources resolve after restart.
4. Open/reopen Graphics Settings and diagnostics; test resize/resolution/upscaler
   changes and supported/unavailable RT/SDK reporting.
5. Exercise inherited camera/material controls and close while a bake is active;
   inspect lifetime/resource errors. Composer feature parity is not asserted.

Starting HEAD: `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`, with inherited staged and
unstaged work preserved. M06-08 accepted by project lead continuation; backend and
package coverage unspecified. M07-01 awaits lead validation.

## M07-02 inventory amendment

The table above records the M07-01 baseline. Its M07-02 gaps are now implemented
at the source level: document camera and validation, single/multi-root adoption,
new/close and canonical complete saves, selected-only model export, prefab/model
insertion with unit/axis wrappers and independent user-component/camera clones.
See [actual contracts and remaining scope](scene-io-cloning.md). M07-03 retains
scene-tree/component/controller UI and fixture/light policy; M07-04 retains
asynchronous/server import; M07-05 retains multi-target bake scheduling.
M07-01 was accepted by lead continuation, with backend/package coverage unspecified.
M07-02 awaits lead validation.


## M07-03 inventory amendment

Scene-tree intents, primary-node component/graph editing, node-only/submesh
selection, general transform gizmos, active camera input, camera/light validation
and the existing material/viewpoint/settings tools are now integrated. See
[scene editing contracts and lead checks](scene-editing.md). M07-02 is accepted
by continuation. M07-03 runtime/production regression validation is pending;
async/service and multi-target bake obligations remain M07-04/05.


## M07-04 inventory amendment

The earlier baseline's HTTP/import/settings gaps are now implemented by app-local
ImportServer/ImportSettings/SceneImporter and ImportActions/ImportUi, with joined
CPU readers, bounded local queue, weak document/destination guards, paused Loader,
canonical remote reimport, publication rollback and independent preferences.
See [actual contracts](async-import-service.md) and the
[lead matrix](../../../../tests/draw/composer_import/README.md). M07-03 including
its compiler correction is accepted by continuation; M07-04 awaits runtime/native
HTTP validation. Scene-wide multi-target baking remains M07-05.

## M07-05 implementation reconciliation

The earlier M07-05 future-work labels above describe the initial foundation.
[Scene baking](scene-baking.md) now implements app-local selected-subtree/scene
jobs, frozen-source sequencing, per-target result identities/previews and staged
temporary byte/HDR texture adoption. Safe UV2 and complete scene IO are reused;
the integrated bake API remains single-live-target. M07-06 owns actual product
regression evidence, including cancel/import/rebind, HDR display limits and
save/reload. No runtime/package coverage is inferred from static delivery.
