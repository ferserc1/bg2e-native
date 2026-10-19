# M06-01 model editor feature and API inventory

[Step](step_01_application-copy.md) · [Progress](../progress.md) ·
[New target](../../../../apps/draw/model_edit/README.md)

Date: 2026-10-19. Baseline HEAD: bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d,
plus preserved inherited M04/M05 changes. Production source was inspected in place;
no file in apps/model_edit was modified or copied into a new render-dependent
build. The draw app is a separate application adapted from the existing draw
runtime ownership pattern. No build/runtime feature parity is asserted.

## Startup and coexistence contract

The new source is apps/draw/model_edit with AppDelegate, StageScene, EditorUi and
main. AppDelegate owns a non-template draw::DefaultRenderLoopDelegate with a
unique RendererDeferred. MainLoop::run(application, EngineConfig) selects draw;
no production Engine/delegate/native callback is constructed by these app sources.
StageScene creates a procedural cube/floor, generated environment and point light
and retains all drawables before loading for partial-failure cleanup. A single
camera node has transform/orbit/camera components. Renderer cleanup happens before
stage GPU cleanup, and the Engine outlives both. Initial and repeat cleanup drain
all retained drawables even if a cleanup throws.

The new target is draw_model_edit, app ID org.bg2e.apps.draw.model-edit and graphics
preference scope draw.model_edit.graphics. It never auto-restores production render
preferences or reads the production env.json. Explicit draw settings persistence
uses the existing facade. Connect happens before first renderer attachment; weak
UI targets and catalog/renderer generations guard queued operations. No raw ImGui
or native graphics type is exposed by new app headers. Standard engine resources
are shared through the resource helper; the macOS bundle/target and any future
application shader prefix are distinct from model_edit. No custom shader is needed
for the startup fixture. bundle_app_sdl compiles/deploys the paired draw/bake tree
through the existing engine dependency/resource refresh helpers.

This step changes only apps/CMakeLists registration and adds the new target's own
CMakeLists, as required by the selected step. No generator, platform setting or
production-target configuration is changed. Compilation/configuration is lead-owned.

## Feature migration matrix

A row marked pending is a required remaining feature, not an intentional product
reduction. Subsequent steps must close these rows before M06 product acceptance.
All original sources are relative to apps/model_edit/src.

| Original owner/workflow | Production dependencies / assumptions | Draw replacement and concrete remaining owner |
|---|---|---|
| main.cpp / MyApplication | Production render delegate, MainLoop default run, org.bg2engine.model-edit | Implemented M06-01: typed draw run, separate application identity/backend CLI, common window/input/UI lifecycle. |
| AppDelegate init/update/render/resize | Templated render::DefaultRenderLoopDelegate<RendererDeferred>, VkExtent2D/VkCommandBuffer/VkImageLayout, FrameResources, private renderer ownership | Implemented M06-01: non-template draw default delegate, Renderer unique_ptr, native-free callbacks/FrameContext and renderer-owned slot data. |
| StageScene initial scene | Production Engine and scene::Drawable, nested orbit transform, HDR environment, light/floor/gizmo nodes | M06-01 creates an explicit startup scene and same-node orbit camera; full production stage controls and authorable state remain M06-03/M06-04. The temporary cube is not document data. |
| Document.cpp/.hpp | Path/dirty state directly updates AppDelegate status items | Pending M06-02: independent document state, path and dirty/publication generation, save-as semantics and status. M06-03 connects edit callbacks/status UI. |
| StageScene loadModel/saveModel | db::loadSceneBg2/storeDrawableBg2 with production Engine and node-based save to preserve joints | Pending M06-02: complete draw load/save, live resources and relative paths, joint/custom component metadata and error-safe publication. |
| StageScene importObj/importGltf/importModel | Production loadDrawableObj/loadGltf; extension routing and target drawable list; imported scenes retained for multi-mesh switching | Pending M06-02: draw-aware OBJ/MTL/glTF realization, format routing, stable source/target identity, preserving target names and actual material images; no reference-only save relabeling. |
| StageScene close/selectTargetNode | Deselect, cancel bake, clear scene owners/target list; index selects a drawable and rewraps active target | Pending M06-02/M06-03: replace at a stopped boundary, validate document generation, retain all old GPU owners until retired; cancel/invalidate bake and clear previews/selection. |
| ToolBar file menu and AppDelegate::fileDropped | Open .bg2/.vwglb, import .obj/.gltf/.glb, Save/Save As, native dialogs and drop routing | Pending M06-02: usable loading/saving and filters; M06-03: menu/shortcut/drop wiring, dirty checks and diagnostics. The startup app does not offer inactive save buttons. |
| checkUnsavedChanges / quit / Document statuses | Cancel/Save/No dialog, save-path prompting, path shown and dirty status; global exit warning | Pending M06-02/M06-03: consistent open/drop/close/quit guards, preserve current document on failed/cancelled save or load, no misleading all-saved state after failed export. |
| SubmeshWindow target list / DrawableEditor | Production targetDrawable/renderMaterial, selected submesh groups, render-based widget caches | Pending M06-03: typed draw selection, app-owned submesh names/groups/visibility panel with weak generation guards. Existing generic DrawableEditor is not presumed draw-safe. |
| SubmeshWindow MaterialEditor | Production MaterialEditor/TextureWidgets, updateTextures and onChanged dirty hooks | Pending M06-03: DrawMaterialEditor/DrawTextureWidgets/TexturePreview, stopped partial MaterialModifier edits, stored/live consistency, history/redraw/dirty notifications and UI-session retirement. |
| ToolBar geometry actions | ConvertAxis ZtoY, center geometry, put on floor, cm-to-m transform modifiers; direct live mesh replacement | Pending M06-03: draw Drawable modifiers at a stopped device boundary, recalculate bindings/BLAS/selection/history, preserve independent clones and notify dirty state. Keep original action labels/semantics reviewed. |
| Selection/highlight and gestures | Production SelectionManager, GizmoComponent, renderer()->gizmoAndSelectionRenderer, four highlight intensities; click-pick versus drag | Pending M06-03: typed draw manager plus optional overlay, stable submesh item IDs, input capture and equivalent highlight modes. M06-01 only forwards orbital input. |
| Workspace / ToolBar / StatusBar | Toolbar, left SubmeshWindow, right EnvironmentSettings, status bar, panel toggles and shortcuts | Implemented in M06-03 correction: `bg2e::ui::Workspace` arranges a full-width top toolbar, docked Model Properties left, Environment Options right and bottom StatusBar. File/View/Window menus include checkable panel toggles; UI/Graphics settings open only from Window. Workspace resizes with the UI viewport. Startup is maximized like production. |
| RenderSettingsWindow and render preferences | Legacy hardcoded render settings, auto-load, 60-second/exit persist timer, toolbar AO/GI/reflection toggles | M06-01 connects live draw RenderSettingsPreferences and GraphicsSettingsWindow with a distinct scope and explicit persistence. Pending M06-03: reviewed restore/save and toolbar convenience policy using actual live effect/mode accessors, no copied closed renderer config enum, no dangling timer capture. |
| EnvironmentSettings sky/lighting | Sky visibility/blur and six preset/custom HDR buttons, renderer environment upload, light editor/polar transform, max five editor lights | Pending M06-03: EnvironmentComponent plus draw environment services and queued reflected light/transform controls; retain preset paths, custom picker and app-specific five-light policy unless explicitly revised. Raw LightEditor/PolarTransformControllerEditor setters need app queuing, not assumed safe. |
| EnvironmentSettings camera/floor | CameraSettings, show/hide floor and height; direct component UI edits | Pending M06-03/M06-04: stopped component editing and camera intrinsics; floor fixture remains excluded from bake tracing and selection/export policy is explicit. |
| Environment save/restore | db::saveScene/loadScene, settingsPath/env.json, named Environment/Lights/Floor extraction, SafeUpdateToken with queued reparent/reload/gizmo rebuild | Pending M06-02 for complete neutral/draw IO; M06-03 for distinct draw preference file, weak stage/document generation, required-node validation, selection/job/preview rebinding and failure-atomic adoption. Do not import production env.json automatically. |
| Camera framing / centerCameraOnTarget | Production render::ViewpointAnalyzer, bestSample, cameraVerticalFieldOfView/applyViewpointToOrbitCamera | Implemented M06-04 in apps/draw/model_edit: bounds-based frame selection/document plus explicit stopped utils::ViewpointAnalyzer sampling, candidate scoring and identifier-to-submesh statistics. Reset remains a separate fixed pose. See viewpoint-camera.md. |
| ModelLightmapWindow UV2 | Production Uv2SafeReload::regenerate, SafeUpdateToken, UvMapPreview, invalidate active bake and dirty document on success | Implemented M06-05 in `apps/draw/model_edit/src/ModelLightmapWindow.cpp`: draw::Uv2SafeReload::regenerateDraw with target/document guards, public bake::UvMapPreviewRenderer and draw-session preview handles. UV2 replacement refreshes selection/previews and marks the document dirty. |
| AppDelegate bake state / ModelLightmapWindow | Production IntegratedBakerContext/IntegratedLightmapBaker, RTAO/RGB8, prepareFrame/update native commands and delayed readback | Implemented M06-05: bake::IntegratedBakerContext records AO/GI samples into submitted draw frames, confirms completion before exact progress advances, and retains stable scene/environment/target owners. Cancel, source edits, document replacement and teardown retire in-flight work before cleanup. |
| Baked pixels/material/preview | Temporary RGB PNG, production render::Texture/TextureWidgets, AO slot + UV2 + updateTextures, last output path and dirty state | Implemented M06-05: composed/dilated bake result preview; apply updates the live AO-slot draw texture and CPU material definition with linear UV2 plus explicit baked AO/GI mode. M06-02 complete resource export writes generated GPU images to `.bg2tex` sidecars. No camera FrameAccumulator/PNG reuse as UV accumulation. |
| Resources / packaging | Production bundle_app, engine shaders/assets and historical texture_gi app shader sources/build helpers | M06-01: unique draw_model_edit + bundle_app_sdl; standard paired engine draw/bake resource trees. Legacy texture_gi is not referenced in current app source/CMake and is not duplicated. M06-06 static inspection confirms recursive package target collection and standard runtime/resource install rules; native package execution and runtime parity remain project-lead checks. See [product validation](product-validation.md). |
| Async/safe mutations and shutdown | Production SafeUpdateToken/raw-this callbacks, timers, document/stage/job/widget owners | Pending M06-02/03/05: weak instance plus distinct document/resource/job generations, stopped producer boundary and preview/job cleanup before Engine/UI release. Startup reset already uses weak stage + renderer generation. |

## Exact API and member-call audit

The following source inventory captures each distinct qualified render/UI/database/
manipulation/utils/geometry reference and renderer/material/widget member-call token
found in every production .hpp/.cpp. It complements the workflow matrix: calls
through local variables are assigned by their containing workflow, not treated as
proof of a reusable production widget. References are source-snapshot line numbers;
future changes must regenerate or reconcile the inventory before closing parity.

### AppDelegate.cpp

Owner: M06-01 lifecycle; M06-03 UI/input; M06-05 bake.

| Reference / call | Lines |
|---|---|
| `VkCommandBuffer` | 60 |
| `VkExtent2D` | 29 |
| `VkImageLayout` | 59 |
| `_renderPrefs->load` | 292 |
| `_renderPrefs->persist` | 297 |
| `_renderPrefs.get` | 305, 306 |
| `_renderSettingsWindow.draw` | 177 |
| `_renderSettingsWindow.init` | 305 |
| `_renderSettingsWindow.isOpen` | 175 |
| `add` | 295 |
| `addItem` | 312, 313 |
| `bg2e::db::saveImage` | 144 |
| `bg2e::manipulation::SelectionManager` | 25 |
| `bg2e::render::DefaultRenderLoopDelegate` | 24 |
| `bg2e::render::Engine` | 22 |
| `bg2e::render::IntegratedBakerContext` | 105 |
| `bg2e::render::LightmapMode::RTAO` | 110 |
| `bg2e::render::LightmapPixelFormat::RGB8` | 111 |
| `bg2e::render::LightmapSettings` | 90, 109 |
| `bg2e::render::RenderSettingsPreferences` | 291 |
| `bg2e::render::RendererDeferred` | 24 |
| `bg2e::render::vulkan::FrameResources` | 37, 65 |
| `bg2e::render::vulkan::Image` | 62, 63, 64 |
| `bg2e::ui::StatusBar::AlignLeft` | 312 |
| `bg2e::ui::StatusBar::AlignRight` | 313 |
| `bg2e::ui::StatusItem` | 310, 311 |
| `checkUnsavedChanges` | 221 |
| `cleanup` | 258, 259 |
| `clear` | 114 |
| `completedFrames` | 76, 132 |
| `createBaker` | 112 |
| `data` | 144 |
| `document` | 159, 317 |
| `draw` | 170, 173, 177, 181 |
| `environmentResources` | 106 |
| `extension` | 219 |
| `get` | 305, 306 |
| `gizmoAndSelectionRenderer` | 333, 336, 339, 342 |
| `importGltf` | 236 |
| `importObj` | 232 |
| `init` | 26, 244, 303, 304, 305, 306, 307, 308 |
| `isOpen` | 171, 175, 179 |
| `leftPanelSize` | 302 |
| `load` | 292 |
| `loadModel` | 228 |
| `material` | 152, 153, 154, 156 |
| `mouseButtonDown` | 195 |
| `mouseButtonUp` | 209 |
| `mouseMove` | 188 |
| `mouseWheel` | 214 |
| `persist` | 297 |
| `pick` | 203 |
| `prepareFrame` | 74 |
| `readPixels` | 142 |
| `renderMaterial` | 155 |
| `renderMaterial(` | 155 |
| `renderer()->environmentResources` | 106 |
| `renderer()->gizmoAndSelectionRenderer` | 333, 336, 339, 342 |
| `renderer()->scene` | 102, 188, 195, 203, 209, 214 |
| `renderer()->setSkyboxBlurLevel` | 246 |
| `reset` | 125, 126, 127, 164, 165, 257 |
| `resize` | 32 |
| `rootNode` | 102, 103, 188, 195, 209, 214 |
| `scene` | 102, 188, 195, 203, 209, 214 |
| `selectedSubmesh` | 205 |
| `setAoScale` | 154 |
| `setAoTexture` | 152 |
| `setAoUVSet` | 153 |
| `setEditMaterial` | 206 |
| `setEnvironmentResources` | 106 |
| `setLineIntensity` | 333, 336, 339, 342 |
| `setMaterialAttributes` | 156 |
| `setMaterialAttributes(` | 156 |
| `setSkyboxBlurLevel` | 246 |
| `setUnsavedChanges` | 159 |
| `settings` | 76, 137 |
| `setup` | 319 |
| `submeshesCount` | 150 |
| `targetDrawable` | 146 |
| `targetModelNode` | 96 |
| `timeout` | 295 |
| `update` | 75 |
| `updateStatus` | 317 |
| `updateTextures` | 157 |
| `updateTextures(` | 157 |
| `what` | 53, 83 |

### AppDelegate.hpp

Owner: M06-01 lifecycle; M06-03 UI/input; M06-05 bake.

| Reference / call | Lines |
|---|---|
| `VkCommandBuffer` | 48 |
| `VkExtent2D` | 42 |
| `VkImageLayout` | 47 |
| `bg2e::manipulation::SelectionManager` | 89, 156 |
| `bg2e::render::DefaultRenderLoopDelegate` | 31 |
| `bg2e::render::Engine` | 40 |
| `bg2e::render::IntegratedBakerContext` | 169 |
| `bg2e::render::IntegratedLightmapBaker` | 170 |
| `bg2e::render::LightmapSettings` | 115 |
| `bg2e::render::RenderSettingsPreferences` | 146 |
| `bg2e::render::Renderer` | 35 |
| `bg2e::render::RendererDeferred` | 31 |
| `bg2e::render::vulkan::FrameResources` | 45, 53 |
| `bg2e::render::vulkan::Image` | 50, 51, 52 |
| `bg2e::ui::RenderSettingsWindow` | 144 |
| `bg2e::ui::StatusBar` | 150 |
| `bg2e::ui::StatusItem` | 94, 98, 148, 149 |
| `bg2e::ui::UISettingsWindow` | 143 |
| `bg2e::ui::UserInterfaceDelegate` | 33 |
| `bg2e::ui::Workspace` | 80, 84, 139 |
| `get` | 73, 77 |
| `renderer` | 37 |

### Document.cpp

Owner: M06-02 document; M06-03 status.

| Reference / call | Lines |
|---|---|
| `empty` | 53 |
| `fileStatus` | 55, 59 |
| `saveStatus` | 64, 68 |
| `setText` | 55, 59, 64, 68 |
| `string` | 59 |

### EnvironmentSettings.cpp

Owner: M06-02 IO; M06-03 scene controls; M06-04 camera.

| Reference / call | Lines |
|---|---|
| `_cameraSettings->draw` | 90 |
| `_cameraSettings->setCameraComponent` | 89 |
| `_lightEditor->draw` | 127 |
| `_lightEditor->setLightComponent` | 125 |
| `_polarEditor->draw` | 133 |
| `_polarEditor->setComponent` | 132 |
| `addLight` | 96 |
| `begin` | 104 |
| `bg2e::render::Renderer` | 26 |
| `bg2e::ui::Button::button` | 53, 57, 61, 65, 69, 73, 77, 94, 135, 149, 161, 168, 173, 185 |
| `bg2e::ui::Button::checkBox` | 41, 45 |
| `bg2e::ui::CameraSettings` | 33 |
| `bg2e::ui::Group::collapsingHeader` | 122 |
| `bg2e::ui::Layout::spacing` | 86, 92, 144, 167 |
| `bg2e::ui::LightEditor` | 31 |
| `bg2e::ui::Numeric::number` | 154 |
| `bg2e::ui::PolarTransformControllerEditor` | 32 |
| `bg2e::ui::Text::separator` | 88, 93, 145 |
| `bg2e::ui::Value::comboBox` | 108 |
| `cameraComponent` | 38 |
| `draw` | 90, 127, 133 |
| `drawSkybox` | 39 |
| `empty` | 80, 179, 191 |
| `end` | 104 |
| `environment` | 37 |
| `floorHeight` | 153 |
| `isFloorVisible` | 146 |
| `light` | 126 |
| `lights` | 99, 113, 119 |
| `openFile` | 189 |
| `ownerNode` | 129 |
| `removeLight` | 137 |
| `reset` | 138 |
| `restoreEnvironmentSettings` | 195 |
| `saveEnvironmentSettings` | 170, 181 |
| `saveFile` | 177 |
| `setCameraComponent` | 89 |
| `setComponent` | 132 |
| `setDrawSkybox` | 43 |
| `setEnvironmentImage` | 55, 59, 63, 67, 71, 75, 82 |
| `setFilters` | 176, 188 |
| `setFlorHeight` | 156 |
| `setLightComponent` | 125 |
| `setSkyboxBlurLevel` | 47 |
| `showFloor` | 151, 163 |
| `size` | 99 |
| `skyboxBlurLevel` | 40 |
| `stage` | 37, 38, 96, 99, 113, 119, 137, 146, 151, 153, 156, 163, 170, 181, 195 |
| `string` | 82 |
| `type` | 126 |

### EnvironmentSettings.hpp

Owner: M06-02 IO; M06-03 scene controls; M06-04 camera.

| Reference / call | Lines |
|---|---|
| `bg2e::render::Renderer` | 28 |
| `bg2e::ui::CameraSettings` | 35 |
| `bg2e::ui::LightEditor` | 33 |
| `bg2e::ui::PolarTransformControllerEditor` | 34 |
| `bg2e::ui::Window` | 24 |

### ModelLightmapWindow.cpp

Owner: M06-05 UV/bake; M06-02 image export.

| API / owner | Use |
|---|---|
| `bake::UvMapPreviewRenderer` | CPU-rasterized UV1/UV2 diagnostics uploaded as sampled images |
| `ui::BakeLightmapPreview` | Draw-session preview handles for UV maps and completed bake results |
| `app::Uv2SafeReload::regenerateDraw` via `AppDelegate::regenerateUv2` | Weak target/document guarded UV2 creation and GPU drawable replacement |
| `bake::LightmapSettings` | AO/GI mode, resolution, samples, ray controls, dilation and CPU precision |
| `AppDelegate::startLightmapBake` / `cancelLightmapBake` | Queued job start and synchronized cancellation |
| `AppDelegate::lightmapBakeProgress` / `lightmapResult` | Exact committed samples and only validated completed output |
| `AppDelegate::applyLightmapBake` | Explicit adoption into target material definitions and live AO-slot texture |
| `cleanup` / `draw` | Cancel pending UV2 on utility close; release preview/session handles before Engine teardown |

### ModelLightmapWindow.hpp

Owner: M06-05 UV/bake; M06-02 image export.

| Reference / call | Lines |
|---|---|
| `bg2e::bake::UvMapPreviewRenderer` | public draw-backend UV diagnostics |
| `bg2e::ui::BakeLightmapPreview` | completion-aware UI preview resources |
| `bg2e::app::SafeUpdateToken` | cancel pending UV2 work on close, target change or cleanup |
| `bg2e::ui::Window` | menu-launched utility window |

### EditorBaking.cpp

Owner: M06-05 UV/bake lifecycle; M06-02 complete image export.

| API / owner | Use |
|---|---|
| `bake::IntegratedBakerContext` / `IntegratedLightmapBaker` | Bake document-root snapshot using a copied deferred-renderer environment resource set |
| `DefaultRenderLoopDelegate::render(FrameContext)` | Append one UV-space AO/GI sample to the current draw-frame command buffer |
| `DefaultRenderLoopDelegate::prepareFrame` + `IntegratedBakerContext::completeFrame` | Wait for submitted work before committing progress, final output, cancellation or teardown |
| `Uv2SafeReload::regenerateDraw` | Invalidate bake state before UV source replacement and rebuild independent draw/tracing resources |
| `MaterialBase::setTexture` / `Drawable::materialDefinition` | Bind the GPU result and persist baked-AO/GI mode, linear interpretation and UV2 metadata in memory |
| `notifyEdited` / `invalidateLightmapSources` | Stop stale bake output on document edits, environment changes and bake application |
| `db::storeDrawableBg2` / `db::saveScene` complete overloads | Export live GPU texture through native `.bg2tex` resource sidecars |

### StageScene.cpp

Owner: M06-02 IO; M06-03 stage; M06-04 framing.

| Reference / call | Lines |
|---|---|
| `addChild` | 68, 69, 74, 82, 100, 107, 111, 114, 132, 198, 244, 381, 448 |
| `addComponent` | 41, 43, 51, 67, 71, 75, 76, 81, 129, 195, 196, 242, 243, 391, 414, 483, 484, 492, 494, 524, 525, 536 |
| `analyze` | 592 |
| `at` | 234 |
| `begin` | 168 |
| `bg2e::db::loadDrawableObj` | 192 |
| `bg2e::db::loadGltf` | 207 |
| `bg2e::db::loadScene` | 347 |
| `bg2e::db::loadSceneBg2` | 128 |
| `bg2e::db::saveScene` | 328 |
| `bg2e::db::storeDrawableBg2` | 142 |
| `bg2e::geo::createPlane` | 386, 528 |
| `bg2e::manipulation::GizmoComponent` | 71, 76, 412, 414, 494 |
| `bg2e::manipulation::SelectableComponent` | 129, 196, 243, 524 |
| `bg2e::render::Engine` | 31 |
| `bg2e::render::ViewpointAnalyzer` | 584 |
| `bg2e::render::ViewpointAnalyzer::bestSample` | 593 |
| `bg2e::utils::applyViewpointToOrbitCamera` | 601 |
| `bg2e::utils::cameraVerticalFieldOfView` | 588 |
| `byName` | 357, 361 |
| `camera` | 48 |
| `cancelLightmapBake` | 150 |
| `cbegin` | 166 |
| `cend` | 167 |
| `children` | 544, 558 |
| `clear` | 152, 157, 261 |
| `clearChildren` | 155, 237 |
| `config` | 587 |
| `deselect` | 151, 248 |
| `drawable` | 130, 140, 220, 383, 393, 397 |
| `empty` | 287, 292, 364, 423 |
| `enabled` | 404 |
| `environment` | 77, 377 |
| `extension` | 164 |
| `find` | 212, 354, 358, 362 |
| `get` | 142, 234, 328, 592 |
| `light` | 97, 98, 99, 496, 497 |
| `lightComponents` | 312, 409, 454, 456 |
| `load` | 388, 531 |
| `lock` | 215, 370 |
| `matrix` | 402 |
| `name` | 222 |
| `ownerNode` | 411, 459, 543, 557 |
| `path` | 287, 299 |
| `push_back` | 221, 222 |
| `removeChild` | 376, 460 |
| `reset` | 153, 154, 156, 258, 259, 260, 262, 578, 596 |
| `rootNode` | 354, 358, 362, 378 |
| `safeUpdateScene` | 367 |
| `saveFile` | 291 |
| `scene` | 312, 406, 409, 449, 454, 456 |
| `selectionManager` | 151, 248 |
| `setAzimuth` | 486 |
| `setCenter` | 597 |
| `setConfig` | 590 |
| `setDistance` | 60, 488 |
| `setElevation` | 487 |
| `setEnabled` | 516, 523 |
| `setEulerX` | 94, 489 |
| `setEulerY` | 95, 490 |
| `setFar` | 47 |
| `setFilters` | 290 |
| `setFocalLength` | 45 |
| `setFrameSize` | 46 |
| `setInitialDistance` | 62 |
| `setIntensity` | 97, 496 |
| `setMaxDistance` | 61 |
| `setMaxX` | 54 |
| `setMaxY` | 55 |
| `setMaxZ` | 56 |
| `setMesh` | 387, 530 |
| `setMinX` | 57 |
| `setMinY` | 58 |
| `setMinZ` | 59 |
| `setPanSpeed` | 64 |
| `setPath` | 134, 143, 297 |
| `setPriority` | 491 |
| `setProjection` | 48 |
| `setRayTracingEnabled` | 390, 397, 534 |
| `setRotation` | 65 |
| `setSpotAngle` | 98 |
| `setSpotCutoff` | 99 |
| `setStatus` | 158, 200, 229, 263 |
| `setTranslation` | 507, 526 |
| `setType` | 497 |
| `setUnsavedChanges` | 135, 145, 185 |
| `setWheelSpeed` | 63 |
| `shared_from_this` | 77, 78, 377, 378, 460 |
| `showError` | 436 |
| `size` | 364, 427, 431, 454, 477 |
| `string` | 164 |
| `transform` | 400, 402, 505, 507, 526 |
| `unsavedChanges` | 269 |
| `updateAll` | 406 |
| `updateLights` | 449 |
| `worldPosition` | 597 |

### StageScene.hpp

Owner: M06-02 IO; M06-03 stage; M06-04 framing.

| Reference / call | Lines |
|---|---|
| `bg2e::render::Engine` | 38, 151 |
| `bg2e::render::ViewpointAnalyzer` | 193 |
| `get` | 76, 80, 101, 117 |
| `lightComponents` | 122 |
| `lock` | 76, 80 |
| `scene` | 122 |
| `size` | 57 |

### SubmeshWindow.cpp

Owner: M06-03 selection/materials.

| Reference / call | Lines |
|---|---|
| `_drawableEditor.cleanup` | 99 |
| `_drawableEditor.draw` | 69 |
| `_drawableEditor.init` | 32 |
| `_drawableEditor.onChanged` | 34 |
| `_drawableEditor.selectedItems` | 73 |
| `_materialEditor.addEditMaterial` | 75, 92 |
| `_materialEditor.cleanup` | 98 |
| `_materialEditor.clearMaterial` | 62, 71, 91 |
| `_materialEditor.draw` | 79 |
| `_materialEditor.init` | 40 |
| `_materialEditor.onChanged` | 27 |
| `_materialEditor.setSelectionManager` | 39 |
| `addEditMaterial` | 75, 92 |
| `bg2e::ui::SelectableList::beginList` | 53 |
| `bg2e::ui::SelectableList::endList` | 66 |
| `bg2e::ui::SelectableList::item` | 59 |
| `bg2e::ui::Text::separator` | 52 |
| `cleanup` | 98, 99 |
| `clearMaterial` | 62, 71, 91 |
| `document` | 29, 36 |
| `draw` | 69, 79 |
| `engine` | 40 |
| `get` | 48 |
| `init` | 32, 40 |
| `isModelValid` | 46, 88 |
| `onChanged` | 27, 34 |
| `renderMaterial` | 75, 92 |
| `renderMaterial(` | 75, 92 |
| `selectTargetNode` | 61 |
| `selectedItems` | 73 |
| `selectedTargetNodeIndex` | 58 |
| `selectionManager` | 32, 39 |
| `setSelectionManager` | 39 |
| `setUnsavedChanges` | 29, 36 |
| `size` | 50, 55 |
| `stage` | 29, 36, 44, 46, 86, 88 |
| `submeshesCount` | 89 |
| `targetDrawable` | 46, 88 |
| `targetNames` | 45, 87 |

### SubmeshWindow.hpp

Owner: M06-03 selection/materials.

| Reference / call | Lines |
|---|---|
| `bg2e::ui::DrawableEditor` | 35 |
| `bg2e::ui::MaterialEditor` | 34 |
| `bg2e::ui::Window` | 23 |

### ToolBar.cpp

Owner: M06-02 file actions; M06-03 tools; M06-04 framing; M06-05 bake.

| Reference / call | Lines |
|---|---|
| `_renderPrefs->indirectLightingEnabled` | 210, 236 |
| `_renderPrefs->rtReflectionsEnabled` | 215, 240 |
| `_renderPrefs->setIndirectLightingEnabled` | 210 |
| `_renderPrefs->setRTReflectionsEnabled` | 215 |
| `_renderSettingsWindow->open` | 142 |
| `addMenuItem` | 41, 52, 63, 74, 89, 100, 101, 108, 112, 119, 127, 135, 136, 140, 144 |
| `applyModifier` | 158, 170, 182, 197 |
| `applyModifier(` | 158, 170, 182, 197 |
| `bg2e::render::RenderSettingsPreferences` | 27 |
| `bg2e::ui::MenuItem` | 40, 107, 118 |
| `bg2e::ui::RenderSettingsWindow` | 26 |
| `bg2e::ui::Toolbar::draw` | 243 |
| `bg2e::ui::UISettingsWindow` | 25 |
| `centerCameraOnTarget` | 114 |
| `document` | 76, 154, 166, 178, 190 |
| `empty` | 47, 58, 69, 77, 84, 95 |
| `exit` | 103 |
| `get` | 156, 168, 180, 192 |
| `importGltf` | 60 |
| `importObj` | 71 |
| `indirectLightingEnabled` | 210, 236 |
| `leftPanelVisible` | 125 |
| `lightmapWindow` | 146, 204 |
| `loadModel` | 49 |
| `open` | 138, 142, 146, 204 |
| `openFile` | 45, 56, 67 |
| `path` | 76 |
| `rightPanelVisible` | 133 |
| `rtReflectionsEnabled` | 215, 240 |
| `saveFile` | 81, 93 |
| `saveModel` | 86, 97 |
| `setFilters` | 44, 55, 66, 80, 92 |
| `setIndirectLightingEnabled` | 210 |
| `setOnExitFunction` | 35 |
| `setRTReflectionsEnabled` | 215 |
| `setUnsavedChanges` | 154, 166, 178, 190 |
| `stage` | 49, 60, 71, 76, 86, 97, 114, 154, 155, 166, 167, 178, 179, 190, 191 |
| `targetDrawable` | 155, 167, 179, 191 |
| `toggleLeftPanel` | 121, 221 |
| `toggleRightPanel` | 129, 228 |
| `toggleSelectionHighlight` | 110 |
| `workspace` | 121, 125, 129, 133, 221, 228 |

### ToolBar.hpp

Owner: M06-02 file actions; M06-03 tools; M06-04 framing; M06-05 bake.

| Reference / call | Lines |
|---|---|
| `bg2e::render::RenderSettingsPreferences` | 41, 48 |
| `bg2e::ui::RenderSettingsWindow` | 40, 47 |
| `bg2e::ui::Toolbar` | 35 |
| `bg2e::ui::UISettingsWindow` | 39, 46 |

### main.cpp

Owner: M06-01 startup / M06-06 coexistence.

| Reference / call | Lines |
|---|---|
| `init` | 41 |
| `initWindowConfig` | 37 |
| `run` | 42 |

## Acceptance and pending-step handoff

M06-01 needs lead compilation/run of draw_model_edit on available Vulkan/Metal,
resize/minimize/restore, orbit/pan/zoom/reset, UI/settings lifecycle, fallback and
production model_edit coexistence. Agent work is static: public signatures/types,
resource helper registration, owned GPL/style, weak callbacks/cleanup and dependency
inspection. No configure/compiler/shader/runtime/packaging execution is claimed.
The explicit tutorial rewrite requested with this step reconstructs all of example
13 from ordered blocks and explains each API; it is independently checked by
concatenating those blocks against its current main.cpp. This does not authorize
implementing the remaining M06 feature rows in this step.


## Delivery reconciliation through M06-03

The initial matrix above is the M06-01 gap inventory; its pending labels are
superseded by [resource IO](resource-io.md) and [editor tools](editor-tools.md).
M06-02 delivered Document/Open/Save/drop/dirty operations and complete resources.
M06-03 delivers typed selection/picking/gizmos, independent node/mesh/material/GPU
clones, names/groups/visibility, geometry actions, partial material JSON patches,
texture histories/previews/sampling, explicit baked interpretation, File shortcuts/
toolbar/panel reopen/status, live graphics controls, environment presets/custom
images, document lights and transform controls, stage floor/fill and intrinsics.
Environment save/restore uses a separate explicit draw default and validated scene
presets. Common input-router changes preserve scene drag release across UI and
reject UI-owned clicks. API/capability settings remain in GraphicsSettingsWindow;
closed production RT configuration/toggle enums are not copied.

M06-04 implements camera bounds framing and scored candidate analysis; UV/bake jobs and
previews M06-05, and cumulative runtime/parity/package validation M06-06. Static
package inspection is recorded in [product validation](product-validation.md); no
build or interaction evidence is inferred from source delivery. The manual tool
matrix includes production input regressions and pending-edit save/quit coverage.
