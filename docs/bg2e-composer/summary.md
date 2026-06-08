# bg2e_composer — Implementation Plan

## Goal

Turn `apps/bg2e_composer` (currently a clone of the `model_edit` 3D-asset
editor) into a **scene editor** for the bg2 engine. The application edits a
single `bg2e::scene::Scene`: it can open scene files, import `.bg2` models
created with `model_edit`, browse the scene graph in a tree, and edit the
components of the selected node (transform, drawable, environment, light).

Two reusable widgets are added to the **engine UI layer** so other
applications can reuse them:

- `bg2e::ui::SceneTree` — generic scene-graph tree view.
- `bg2e::ui::NodeEditor` — node property editor that hosts per-component editors.

## Key concepts / decisions

- **Single editable scene.** The renderer owns one `scene::Scene` whose root is
  set once in `RendererDeferred::initScene` and cannot be reset at runtime
  (it registers a cleanup that resets `_scene`). We therefore keep that root as
  a **thin, stable container node** and put the whole *editable scene* in a
  single child subtree. "Open Scene" swaps that child subtree using
  `MainLoop::safeUpdateScene(...)` followed by `scene()->updateAll()`. This is
  the same pattern the old `StageScene::restoreEnvironmentSettings` already used.

- **Camera is a normal node.** The camera is an ordinary node with `Transform`,
  `Camera` and `OrbitCamera` components, living *inside* the editable scene
  content — edited like any other node. The initial scene contains a camera,
  a directional light and an environment, all as editable nodes. If the user
  deletes a required node (e.g. the camera) the app may fail to render; a
  fallback (a compute shader that clears the image when required elements are
  missing) is **out of scope for this plan** and will be designed later.

- **Two selection concepts, bridged by the app.**
  - `SelectionManager` selection items require a drawable/mesh, so they can only
    represent drawable nodes (used for viewport picking + highlight).
  - `SceneTree` keeps its **own node-selection set** so camera/light/environment
    nodes (which have no mesh) can also be selected.
  - The app bridges the two: picking a mesh in the viewport selects its owner
    node in the tree; selecting a node in the tree updates `SelectionManager`
    (selecting its drawable when present, clearing the mesh/material selection
    otherwise).

- **Material/mesh editing is preserved.** The left "Model Properties" panel
  (`SubmeshWindow`) keeps the existing `MaterialEditor`/`DrawableEditor`, but is
  driven by the current selection. It shows the editor only when a single
  drawable node is selected, and `<multiple_selection>` when more than one is.

- **Engine widgets stay app-agnostic.** `SceneTree` and `NodeEditor` depend only
  on engine types (`scene::*`, `db::*`, `app::FileDialog`, existing
  `bg2e::ui::*` editors). The app only *composes* them and provides the
  selection bridge.

## Layout

The `Workspace` keeps the same slots (`AppDelegate::initWorkspace`):

| Slot         | Before                          | After                                    |
|--------------|---------------------------------|------------------------------------------|
| toolBar      | `ToolBar`                       | `ToolBar` (cleaned File menu)            |
| left panel   | `SubmeshWindow` (Model Props)   | `SubmeshWindow`, driven by selection     |
| right panel  | `EnvironmentSettings`           | `SceneEditor` = `SceneTree` + `NodeEditor` |
| bottom panel | `nullptr`                       | `nullptr`                                |
| status bar   | `StatusBar`                     | `StatusBar`                              |

The right panel is split vertically: **top = scene tree**, **bottom = node
(component) editor**.

## Phases

Each phase ends with a project that **compiles** (per `CLAUDE.md`, do not modify
CMake and do not compile unless explicitly requested; "compiles" describes the
intended end state of each phase, not an action to perform).

| Phase | Scope | Area | File |
|------|-------|------|------|
| 1 | Single-scene refactor of `StageScene`, File menu cleanup, remove environment editor & obj/gltf imports, stub `SceneEditor` panel, selection-driven `SubmeshWindow` | app | [phase-1-single-scene-refactor.md](phase-1-single-scene-refactor.md) |
| 2 | `bg2e::ui::SceneTree` reusable widget (standalone) | engine | [phase-2-scenetree-widget.md](phase-2-scenetree-widget.md) |
| 3 | `bg2e::ui::NodeEditor` reusable widget + component editors (standalone) | engine | [phase-3-nodeeditor-widget.md](phase-3-nodeeditor-widget.md) |
| 4 | Compose `SceneEditor` = `SceneTree` (top) + `NodeEditor` (bottom) | app | [phase-4-compose-ui.md](phase-4-compose-ui.md) |
| 5 | Selection bridge (viewport ↔ tree ↔ material panel) | app | [phase-5-selection-bridge.md](phase-5-selection-bridge.md) |

Dependency order: 1 → (2, 3 independent) → 4 → 5.

## Out of scope (future work)

- Fallback rendering (compute-shader clear) when required nodes are missing.
- Refactor/extension of the component edit controls beyond the minimal set.
- A public component-editor registration API on `NodeEditor`.
- Creating/deleting/reparenting nodes from the tree (only navigation + selection
  are required now).
