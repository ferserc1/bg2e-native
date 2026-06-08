# Phase 5 — Selection bridge

**Area:** app (`apps/bg2e_composer`)
**Goal:** Connect the two selection concepts so the viewport, the scene tree and
the left "Model Properties" panel stay consistent:

- Picking a mesh in the viewport selects its owner **node** in the tree.
- Selecting a node in the tree updates `SelectionManager` (selecting the node's
  drawable when present, clearing the mesh/material selection otherwise) and
  clears the material editor.
- The left `SubmeshWindow` shows the drawable/material editor only for a single
  drawable node, and `<multiple_selection>` when more than one drawable node is
  selected.

**End state:** the app compiles and runs with bidirectional viewport ↔ tree
selection and a material panel that respects single vs. multiple selection.

---

## Background

- `manipulation::SelectionManager` selection items
  (`SelectionItem`) carry `node`, `drawable` (`DrawableComponent`), `mesh`
  (`Drawable`) and `submesh`. They exist only for **drawable** nodes — used for
  viewport picking and highlight.
- `bg2e::ui::SceneTree` keeps its own node-selection set (any node type).
- The bridge keeps these in sync **without feedback loops**. Recall (Phase 2):
  programmatic `SceneTree` setters (`setSelectedNodes`, `selectNode`,
  `clearSelection`) do **not** fire `onSelectionChanged`; only user clicks do.
  Similarly, guard the SelectionManager `onSelect` handler so a tree-initiated
  update does not bounce back. Use a simple reentrancy flag
  (`bool _syncingSelection`).

## 1. Tree → SelectionManager + material panel

Extend the `SceneTree.onSelectionChanged` handler (set in Phase 4's
`SceneEditor::init`) so that, in addition to driving `NodeEditor`:

```cpp
_sceneTree.onSelectionChanged([&]() {
    if (_syncingSelection) return;
    _syncingSelection = true;

    auto nodes = _sceneTree.selectedNodes();

    // Node editor (from Phase 4)
    if (nodes.size() == 1) _nodeEditor.setNode(nodes.front());
    else                   _nodeEditor.setNodes(nodes);

    // Bridge to SelectionManager (mesh highlight) + material panel
    auto sm = _appDelegate->selectionManager();
    sm->deselect();                          // clear mesh selection + highlight
    _appDelegate->submeshWindow().clearMaterialSelection();  // clear material editor

    // Re-add drawable nodes to the SelectionManager so they highlight
    for (auto * node : nodes) {
        if (auto dc = node->getComponent<scene::DrawableComponent>()) {
            // submesh 0 (whole drawable). Adjust if per-submesh node selection
            // is needed later.
            sm->addToSelectedItems(node, dc, 0);
        }
    }

    _syncingSelection = false;
});
```

Notes:
- `SelectionManager::deselect()` must be called before nodes are removed/replaced
  (it is here, before re-adding).
- Add a small `AppDelegate::submeshWindow()` accessor (or route through an
  existing one) so the bridge can clear the material selection. Add a
  `SubmeshWindow::clearMaterialSelection()` that calls
  `_materialEditor.clearMaterial()` (and clears the drawable editor selection if
  needed).
- Selecting non-drawable nodes (camera/light/environment) leaves the
  SelectionManager empty — correct, since they have no mesh to highlight.

## 2. Viewport pick → tree

`AppDelegate::mouseButtonUp` currently does:

```cpp
if (_selectionManager->pick(renderer()->scene(), x, y)) {
    auto selection = _selectionManager->selectedSubmesh();
    _submeshPanel.setEditMaterial(selection);
}
```

Extend it to reflect the picked node into the tree:

```cpp
if (_selectionManager->pick(renderer()->scene(), x, y)) {
    _submeshPanel.setEditMaterial(_selectionManager->selectedSubmesh());

    // Reflect into the scene tree without bouncing back
    if (auto * node = _selectionManager->selectedNode()) {
        _syncingSelection = true;
        _sceneEditor.sceneTree().setSelectedNodes({ node }); // does NOT fire callback
        _sceneEditor.nodeEditor().setNode(node);             // keep node editor in sync
        _syncingSelection = false;
    }
}
```

Decide where `_syncingSelection` lives. Cleanest: make it a member of
`AppDelegate` and have `SceneEditor` read it via the delegate, or move the bridge
logic into `AppDelegate` entirely and have `SceneEditor` only expose
`sceneTree()`/`nodeEditor()` (recommended — keeps `SceneEditor` a thin view and
all selection policy in one place). If moved, set the tree's
`onSelectionChanged` from `AppDelegate` after `SceneEditor::init`.

When the pick clears the selection (empty pick with
`clearSelectionOnEmptyPick`), also clear the tree:
`_sceneEditor.sceneTree().clearSelection();` and `_nodeEditor.setNode(nullptr);`.

## 3. Left panel: single vs. multiple drawable selection

In `SubmeshWindow::setDrawFunction`, gate the editors on the number of selected
**drawable** nodes (from `SelectionManager::selectedItems()`):

```cpp
auto sm = _appDelegate->selectionManager();
auto count = sm->selectedItems().size();

if (count == 0) {
    BasicWidgets::text("No selection");
    return;
}
if (count > 1) {
    BasicWidgets::text("<multiple_selection>");
    return;
}

// Exactly one drawable selected: show drawable + material editors as today
auto drawable = sm->selectedMesh();   // scene::Drawable*
if (drawable) {
    if (_drawableEditor.draw()) {
        _materialEditor.clearMaterial();
        for (auto sel : _drawableEditor.selectedItems())
            _materialEditor.addEditMaterial(drawable->renderMaterial(sel));
    }
    _materialEditor.draw();
}
```

(`selectedItems()` counts only drawable nodes, since only those can be added to
the SelectionManager — matching the spec: editing is allowed only when a single
drawable component is selected.)

## Edge cases

- Opening a scene / `close()` already calls `SelectionManager::deselect()`; also
  clear the tree selection and reset `NodeEditor` to `nullptr` there (add to
  `StageScene::setEditableRoot`'s safe-update lambda or in the delegate after the
  swap).
- After a content swap the previously selected nodes are gone; the weak_ptrs in
  `SceneTree` expire and are skipped — verify no dangling raw pointers are used
  past a swap (always re-resolve through the tree's `selectedNodes()`).

## Verification (manual, when explicitly allowed to run)

- Click a model in the viewport → its node highlights in the tree and its
  material shows in the left panel.
- Click a node in the tree → matching mesh highlights in the viewport (drawable
  nodes) or selection clears (camera/light/environment), and the material panel
  clears.
- Ctrl-select two drawable nodes → left panel shows `<multiple_selection>` and
  disables editing.
- Open a scene → selection clears in viewport, tree and panels.
