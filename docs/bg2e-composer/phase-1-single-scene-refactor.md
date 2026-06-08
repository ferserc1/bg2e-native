# Phase 1 — Single-Scene refactor + File menu cleanup

**Area:** app (`apps/bg2e_composer`)
**Goal:** Replace the model-editing scaffolding of `StageScene` with a single
editable `scene::Scene`. Clean the File menu (open scenes, import `.bg2` only).
Remove the environment editor and the obj/gltf import paths. Add an empty
`SceneEditor` right-panel placeholder. Drive `SubmeshWindow` from the selection.

**End state:** the app compiles and runs, renders the default scene (camera +
directional light + environment as normal nodes), can open scene files and
import `.bg2` models. No environment editor; the right panel is an empty
"Scene" window. Viewport picking still fills the material editor.

---

## 1. Rewrite `StageScene`

### Scene structure

```
containerRoot (stable, owned by renderer via initScene; not editable, not shown in tree)
└── editableRoot ("Scene")          <- the whole editable scene; swapped on Open
    ├── Camera        (Transform + Camera + OrbitCamera)
    ├── Directional Light (Transform + Light + PolarTransformController + ...)
    └── Environment   (Environment + Transform)
    └── ... imported model nodes (Transform + Drawable + Selectable)
```

`init()` returns `containerRoot` (this is what `createScene()` passes to the
renderer). `containerRoot` has exactly one child: `editableRoot`.

### Header `StageScene.hpp` — new shape

Keep the file's license header. Replace the class body with roughly:

```cpp
class StageScene {
public:
    StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate);

    // Builds containerRoot + default editable scene; returns containerRoot
    std::shared_ptr<bg2e::scene::Node> init();

    // Replace the whole editable scene with one loaded from disk
    void openScene(const std::filesystem::path& path);

    // Save the editable scene to disk
    void saveScene(const std::filesystem::path& path);

    // Import a .bg2 model created with model_edit as a new node in the scene
    void importModelBg2(const std::filesystem::path& path);

    // Empty the editable scene (deselect first)
    void close();

    void cleanup();

    bool checkUnsavedChanges();   // keep, but use scene filters (see below)

    inline bool isSceneValid() const { return _editableRoot != nullptr; }

    inline std::shared_ptr<bg2e::scene::Node> sceneRoot() const { return _containerRoot; }
    // The editable subtree shown in the scene tree
    inline std::shared_ptr<bg2e::scene::Node> editableRoot() const { return _editableRoot; }

    // Resolved from the current scene content (not stored)
    bg2e::scene::OrbitCameraComponent * orbitCamera();
    bg2e::scene::CameraComponent * cameraComponent();

    inline Document * document() { return _document.get(); }

protected:
    bg2e::render::Engine * _engine;
    AppDelegate * _appDelegate;
    std::unique_ptr<Document> _document;

    std::shared_ptr<bg2e::scene::Node> _containerRoot;
    std::shared_ptr<bg2e::scene::Node> _editableRoot;

    std::shared_ptr<bg2e::app::SafeUpdateToken> _swapToken;

    std::shared_ptr<bg2e::scene::Node> buildDefaultScene();
    void setEditableRoot(std::shared_ptr<bg2e::scene::Node> newEditableRoot);
    void ensureMainCameraProjection(bg2e::scene::Scene * scene);
};
```

**Remove** from the header (all the model/environment scaffolding):
`loadModel`, `saveModel`, `importModel/importObj/importGltf`, `multiMeshScene`,
`targetNames`, `selectTargetNode`, `selectedTargetNodeIndex`, `targetDrawable`,
`isModelValid`, `environment()`, `lights()`, `iterateLights`, `maxLights`,
`addLight`, `removeLight`, `showFloor`, `isFloorVisible`, `setFlorHeight`,
`floorHeight`, `save/restoreEnvironmentSettings`, `updateLightMesh`,
`createLightNode`, `getLightMesh`, `createFloorNode`, `instantUpdateLightMesh`,
and all the related member variables (`_targetNode`, `_targetDrawable(s)`,
`_targetScene`, `_targetNames`, `_lightsNode`, `_floorNode`, `_showFloor`,
`_floorHeight`, `_environment`, `_environmentNode`, `_restoringEnvironment`,
`_restoreToken`, the light-mesh members).

### Implementation `StageScene.cpp`

- **`init()`**: create `_containerRoot = make_shared<Node>("Container")`;
  `_editableRoot = buildDefaultScene()`; `_containerRoot->addChild(_editableRoot)`;
  return `_containerRoot`.

- **`buildDefaultScene()`**: create a node `"Scene"` and add the default content,
  reusing the construction logic from the old `init()` (lines that built the
  camera rig, the directional light and the environment node) but **without** the
  floor, the second point light, the gizmo/light-mesh logic, the sphere, and the
  environment-settings restore. Concretely:
  - **Camera node**: `Transform` (translated `0,0,2`), `CameraComponent` with the
    `OpticalProjection` setup from the old `init()`, and an `OrbitCameraComponent`
    configured as in the old `init()` (this is the single editor camera, now a
    normal node). Keep the camera node as a direct child of `"Scene"`.
  - **Directional light node**: `Transform`, `LightComponent`
    (`Light::TypeDirectional`, white, intensity ~5), `PolarTransformControllerComponent`.
    A light gizmo mesh is *not* required for Phase 1 (it was editor chrome); omit
    it to keep this minimal. (If a visible gizmo is desired later, add it then.)
  - **Environment node**: `EnvironmentComponent(PlatformTools::assetPath(),
    "mirrored_hall_4k.hdr")` + `Transform`.
  - Return the `"Scene"` node.

- **`setEditableRoot(newEditableRoot)`**: the safe-swap helper.
  ```cpp
  _swapToken = std::make_shared<bg2e::app::SafeUpdateToken>();
  bg2e::app::MainLoop::current()->safeUpdateScene(
      [this, newEditableRoot]() {
          _appDelegate->selectionManager()->deselect();
          if (_editableRoot) _containerRoot->removeChild(_editableRoot);
          _editableRoot = newEditableRoot;
          _containerRoot->addChild(_editableRoot);
          auto scene = _containerRoot->scene();
          scene->updateAll();
          // Re-resolve the main camera from the new content
          if (auto cam = cameraComponent()) scene->setMainCamera(cam);
          ensureMainCameraProjection(scene);   // see below
      },
      _swapToken
  );
  ```

- **`ensureMainCameraProjection(scene)`**: a loaded scene may contain a camera
  without a projection, which makes the aspect ratio render wrong. After the
  swap, get the **main camera from the `Scene` instance** and, if it has no
  projection, assign an `OpticalProjection` configured with a **50 mm film
  size** and a **55 mm lens**:
  ```cpp
  void StageScene::ensureMainCameraProjection(bg2e::scene::Scene * scene) {
      if (!scene) return;
      auto cam = scene->mainCamera();          // use the Scene instance
      if (!cam || cam->projection()) return;   // keep an existing projection
      auto projection = new bg2e::math::OpticalProjection();
      projection->setFrameSize(50.0f);         // film size: 50 mm
      projection->setFocalLength(55.0f);       // lens: 55 mm
      projection->setFar(1000.0f);             // keep a sane near/far (match default camera)
      cam->setProjection(projection);
  }
  ```
  Declare `ensureMainCameraProjection(bg2e::scene::Scene*)` in the header. The
  resize visitor sets the viewport on the projection, so the aspect ratio is
  correct on the next frame.

- **`openScene(path)`**:
  ```cpp
  auto newScene = bg2e::db::loadScene(path, *_engine);
  if (!newScene || !newScene->rootNode()) { /* MessageBox error; return; */ }
  auto newRoot = std::static_pointer_cast<bg2e::scene::Node>(
      newScene->rootNode()->shared_from_this());
  setEditableRoot(newRoot);
  _document->setPath(path);
  _document->setUnsavedChanges(false);
  ```
  Hold `newScene` alive long enough for the swap (capture it in the lambda or keep
  a member); the grafted node keeps living because `_editableRoot` and the
  container's child vector own it.

- **`importModelBg2(path)`**:
  ```cpp
  auto drawable = bg2e::db::loadDrawableBg2(path, _engine); // shared_ptr<Drawable>
  auto node = std::make_shared<bg2e::scene::Node>(path.stem().string());
  node->addComponent(new bg2e::scene::TransformComponent());
  node->addComponent(new bg2e::scene::DrawableComponent(drawable));
  node->addComponent(new bg2e::manipulation::SelectableComponent());
  bg2e::app::MainLoop::current()->safeUpdateScene([this, node]() {
      _editableRoot->addChild(node);
      _containerRoot->scene()->updateAll();
  });
  _document->setUnsavedChanges(true);
  ```

- **`saveScene(path)`**: `bg2e::db::saveScene(_editableRoot.get(), path);`
  then `_document->setPath(path); _document->setUnsavedChanges(false);`

- **`close()`**: deselect; replace the editable root with a fresh empty
  `buildDefaultScene()` (or an empty `"Scene"` node) via `setEditableRoot`;
  `_document->setStatus("", false)`.

- **`orbitCamera()` / `cameraComponent()`**: resolve from the current scene using
  `FindCameraVisitor` / `FindNodeComponentVisitor<OrbitCameraComponent>` over
  `_editableRoot.get()`, returning the first found or `nullptr`.

- **`checkUnsavedChanges()`**: keep the logic but change the save path to call
  `saveScene` and use scene file filters (e.g. `{ "bg2e scene", "json,vitscnj" }`).

## 2. `ToolBar` — File menu cleanup

In `ToolBar::init`:
- **File menu** items: *Open Scene* (`Ctrl+O`, scene filters →
  `stage()->openScene`), *Import bg2 Model* (`.bg2,.vwglb` →
  `stage()->importModelBg2`), *Save* (`Ctrl+S` → `stage()->saveScene`,
  prompting for a path when `document()->path()` is empty, scene filters),
  *Save As…* (`Ctrl+Shift+S` → `stage()->saveScene`), separator, *Quit*.
- **Remove** the *Import GLTF* and *Import OBJ* items.
- **Window menu**: rename the "Environment" item to "Scene"; keep it toggling the
  right panel (`workspace().toggleRightPanel()` / `rightPanelVisible()`).
- The mesh-modifier toolbar buttons ("Y-axis > Z-axis", "Center Geometry",
  "cm to m") operated on `targetDrawable()`. Since `targetDrawable()` is gone,
  either remove these buttons in Phase 1 or repoint them to the currently
  selected drawable (`selectionManager()->selectedMesh()`). **Recommended:**
  remove them now; they can return as node-context actions later. Likewise remove
  the right-aligned "Environment" button or rename to "Scene".

## 3. `AppDelegate`

- Remove the `EnvironmentSettings _environmentPanel` member and its include.
- `initWorkspace`: remove `_environmentPanel.init(...)`; instantiate the new stub
  `SceneEditor` window (see §5) and pass it as the **right panel** in
  `_workspace.setup(...)`. Keep `_submeshPanel` as the left panel.
- `fileDropped`: keep only two branches —
  `.bg2/.vwglb` → `stage()->importModelBg2(path)`;
  scene files (`.json`/`.vitscnj`) → `stage()->openScene(path)`.
  Remove the obj/gltf branches.
- `createScene()`: unchanged in spirit — `_stage = make_shared<StageScene>(...)`,
  `auto scene = _stage->init();`, keep `setSkyboxBlurLevel(2)`, `initWorkspace()`.
- `mouseButtonUp`: keep the pick → `_submeshPanel.setEditMaterial(selection)`
  call (selection-driven material editing continues to work).

## 4. `SubmeshWindow` — drive from selection

`SubmeshWindow` currently reads `stage()->targetNames()/targetDrawable()/
selectedTargetNodeIndex()/selectTargetNode()/isModelValid()`, which no longer
exist. Adapt to the selection:
- Remove the target-names `SelectableList` block.
- Obtain the drawable from the selection:
  `auto drawable = _appDelegate->selectionManager()->selectedMesh();`
  (a `scene::Drawable*`; wrap/guard for null). Use the existing
  `_drawableEditor` / `_materialEditor` flow against it.
- `setEditMaterial(submeshIndex)`: read the drawable from the selection instead
  of `targetDrawable()`.
- This panel's full single/multiple-selection behaviour is finished in Phase 5;
  Phase 1 only needs it to compile and show the selected mesh's material.

## 5. Delete `EnvironmentSettings`, add stub `SceneEditor`

- Delete `src/EnvironmentSettings.hpp` and `src/EnvironmentSettings.cpp` and any
  remaining references/includes.
- Add `src/SceneEditor.hpp` / `src/SceneEditor.cpp`: a `bg2e::ui::Window`
  subclass with `init(AppDelegate*)`, `setTitle("Scene")`, and a `setDrawFunction`
  that for now renders a placeholder (e.g. `BasicWidgets::text("Scene tree")`).
  This becomes the real composer in Phase 4.

## Verification (manual, when explicitly allowed to run)

- App launches and renders the default scene.
- File ▸ Open Scene loads a scene file and replaces the current one without
  crashing (resources safely swapped); selection is cleared on load.
- After opening a scene whose camera has no projection, the aspect ratio is
  correct (an `OpticalProjection` with 50 mm film / 55 mm lens is assigned).
- File ▸ Import bg2 Model adds a model node.
- Right panel shows the empty "Scene" window; no environment editor anywhere.
- Clicking a mesh still shows its material in the left "Model Properties" panel.
