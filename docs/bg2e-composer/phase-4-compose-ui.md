# Phase 4 — Compose the interface in bg2e_composer

**Area:** app (`apps/bg2e_composer`)
**Goal:** Turn the stub `SceneEditor` right panel (from Phase 1) into the real
composer: `bg2e::ui::SceneTree` on top, `bg2e::ui::NodeEditor` on the bottom,
split vertically. Tree selection drives the node editor.

**End state:** the app compiles and runs. The right panel shows the scene tree
(camera, lights, environment, imported models) over the editable scene root;
clicking a node shows and edits its components below. Selection bridging to the
viewport/material panel is **not** done here — that is Phase 5.

---

## `SceneEditor` window

Replace the Phase 1 stub body with the composed layout.

### Header

```cpp
class SceneEditor : public bg2e::ui::Window {
public:
    void init(AppDelegate * delegate);
    void cleanup();

    // Exposed so the selection bridge (Phase 5) can sync programmatically
    bg2e::ui::SceneTree & sceneTree() { return _sceneTree; }
    bg2e::ui::NodeEditor & nodeEditor() { return _nodeEditor; }

private:
    AppDelegate * _appDelegate = nullptr;
    bg2e::ui::SceneTree _sceneTree;
    bg2e::ui::NodeEditor _nodeEditor;
};
```

### init

```cpp
void SceneEditor::init(AppDelegate * delegate) {
    _appDelegate = delegate;
    setTitle("Scene");

    _nodeEditor.init(delegate->renderer()->engine()); // or however Engine* is reached
    _nodeEditor.onChanged([&]() {
        _appDelegate->stage()->document()->setUnsavedChanges(true);
        // environment image / lights may have changed; refresh scene references
        auto scene = _appDelegate->stage()->sceneRoot()->scene();
        if (scene) scene->updateAll();
    });

    // Tree selection -> show node in the node editor.
    _sceneTree.onSelectionChanged([&]() {
        auto nodes = _sceneTree.selectedNodes();
        if (nodes.size() == 1) _nodeEditor.setNode(nodes.front());
        else                   _nodeEditor.setNodes(nodes);
        // The mesh/material/SelectionManager bridge is added in Phase 5.
    });

    setDrawFunction([&]() {
        // Keep the tree root in sync with the current editable scene
        _sceneTree.setRootNode(_appDelegate->stage()->editableRoot().get());

        // Vertical split: top tree, bottom node editor.
        const float avail = ImGui::GetContentRegionAvail().y;
        const float treeHeight = avail * 0.5f;

        ImGui::BeginChild("scene_tree", ImVec2(0, treeHeight), true);
        _sceneTree.draw();
        ImGui::EndChild();

        ImGui::BeginChild("node_editor", ImVec2(0, 0), true);
        _nodeEditor.draw();
        ImGui::EndChild();
    });
}
```

Notes:
- Reach the `render::Engine*` for `NodeEditor::init` through whatever accessor the
  app already exposes (the `StageScene` holds `_engine`; consider adding
  `StageScene::engine()` or pass it from `AppDelegate`). The renderer also has
  the engine; pick the cleanest existing path and add a small accessor if needed.
- Including `imgui.h` in `SceneEditor.cpp` is fine (other app windows that need
  raw child regions do the same — verify against existing usage; if the project
  prefers wrapping, add `BasicWidgets` helpers, but a direct `BeginChild` keeps
  this minimal).
- The two `BeginChild` regions give the required vertical division and
  independent scrolling for tree and editor.

## `AppDelegate`

- Replace the stub `SceneEditor` member usage: it is already the right panel from
  Phase 1, so only the `init` changes. Confirm `initWorkspace` calls
  `_sceneEditor.init(this)` and passes `&_sceneEditor` as the right panel.
- `cleanup`: call `_sceneEditor.cleanup()` if it owns resources (the embedded
  editors' `cleanup`, e.g. `_nodeEditor` reuses `LightEditor` etc. — release as
  needed).

## ToolBar

- The Window-menu "Scene" toggle (added in Phase 1) already toggles the right
  panel; no change required.

## Verification (manual, when explicitly allowed to run)

- Right panel shows the scene tree with the default nodes (Camera, Directional
  Light, Environment) and any imported models.
- Expand/collapse works; clicking a node shows its components below.
- Editing a Transform moves/rotates/scales the node in the viewport.
- "Replace Model…" on a drawable node swaps the model.
- "Select Image…" on the environment node changes the environment.
- The light node shows the light editor and edits apply.
- No viewport-selection coupling yet (that is Phase 5).
