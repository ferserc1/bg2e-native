# Phase 2 — `bg2e::ui::SceneTree` (reusable engine widget)

**Area:** engine (`lib/include/bg2e/ui`, `lib/src/bg2e/ui`)
**Goal:** A generic, reusable scene-graph tree view. Built standalone; not wired
into the app yet (the engine library compiles with the new files thanks to
CMake auto-glob).

**End state:** the engine library compiles with `SceneTree` available. The widget
renders a node hierarchy, supports expand/collapse and node selection
(single/multi), and reports selection changes via a callback.

---

## Files

- `lib/include/bg2e/ui/SceneTree.hpp`
- `lib/src/bg2e/ui/SceneTree.cpp`
- Add `#include <bg2e/ui/SceneTree.hpp>` to `lib/include/bg2e/ui/all.hpp`.

Use the standard GPL license header (copy from any existing `ui` file).

## Design

`SceneTree` operates on a `scene::Node*` root and owns its **own** node-selection
set (independent of `manipulation::SelectionManager`, which cannot represent
nodes without a drawable). It does not modify the scene graph — navigation and
selection only.

### Public API

```cpp
namespace bg2e { namespace ui {

class BG2E_API SceneTree {
public:
    using SelectionChangedCallback = std::function<void()>;

    void setRootNode(bg2e::scene::Node * root);   // node whose CHILDREN are listed,
                                                  // or the node itself as the root row
    bg2e::scene::Node * rootNode() const { return _root; }

    void setMultiSelection(bool enabled) { _multiSelection = enabled; }
    bool multiSelection() const { return _multiSelection; }

    // Draw the tree (call inside a Window draw function / child region)
    void draw();

    // Current selection (expired weak_ptrs are skipped)
    std::vector<bg2e::scene::Node*> selectedNodes() const;
    bg2e::scene::Node * primarySelectedNode() const;   // last clicked, or null

    void setSelectedNodes(const std::vector<bg2e::scene::Node*>& nodes);
    void selectNode(bg2e::scene::Node * node, bool additive = false);
    void clearSelection();
    bool isSelected(const bg2e::scene::Node * node) const;

    // Called whenever the selection set changes (from UI interaction or setters
    // that originate from user code may choose to call notify or not — see notes)
    void onSelectionChanged(SelectionChangedCallback cb) { _onSelectionChanged = cb; }

protected:
    bg2e::scene::Node * _root = nullptr;
    bool _multiSelection = true;
    std::vector<std::weak_ptr<bg2e::scene::Node>> _selected;
    std::weak_ptr<bg2e::scene::Node> _primary;
    SelectionChangedCallback _onSelectionChanged;

    void drawNode(bg2e::scene::Node * node);
    void handleClick(bg2e::scene::Node * node);   // applies ctrl/shift modifiers
    void notifyChanged() const;
};

}}
```

### Drawing

In `draw()`, iterate the children of `_root` and call `drawNode` on each (the
container root from Phase 1 is not shown; the app passes the *editable* root as
`_root`, so its children — camera, lights, environment, models — are the
top-level rows). Implement `drawNode(node)` with ImGui tree nodes:

- Use `ImGui::TreeNodeEx` with flags:
  - `ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth`
  - `ImGuiTreeNodeFlags_Selected` when `isSelected(node)`
  - `ImGuiTreeNodeFlags_Leaf` when `node->children().empty()`
- Use a stable id per node: push `node` pointer as the id
  (`ImGui::PushID(node)` / `TreeNodeEx((void*)node, flags, "%s", name)`).
- Label: `node->name()` (fall back to a placeholder like `"(unnamed)"`).
- After the node line, if it is open and not a leaf, recurse into
  `node->children()` then `ImGui::TreePop()`.
- Detect selection with `ImGui::IsItemClicked()` → `handleClick(node)`.

Keep it dependency-light: include `imgui.h` in the `.cpp` only (consistent with
other `bg2e::ui` implementations — check how `BasicWidgets.cpp` includes ImGui).

### Selection logic (`handleClick`)

- If `_multiSelection` and Ctrl is held (`ImGui::GetIO().KeyCtrl`): toggle the
  node in `_selected` (additive).
- Else: replace the selection with just this node.
- Update `_primary` to the clicked node.
- Call `notifyChanged()`.

`notifyChanged()` invokes `_onSelectionChanged` if set.

`selectedNodes()` / `setSelectedNodes()` / `selectNode()` / `clearSelection()`
operate on `_selected` (store `weak_ptr` via `node->shared_from_this()`; skip
expired entries on read). Programmatic setters used by the bridge (Phase 5) must
**not** re-trigger the callback in a way that loops; document that
`setSelectedNodes`/`selectNode`/`clearSelection` update state **without** firing
`onSelectionChanged` (only user clicks fire it). The bridge relies on this to
avoid feedback loops.

## Notes / constraints

- No node creation/deletion/reparenting in this phase (out of scope).
- The widget must compile and be safe to draw with a `nullptr` root (draw
  nothing).
- Do not modify CMake (auto-glob picks up the new files).

## Verification

- The engine library builds with the new header/source (when building is
  explicitly requested).
- Logic review: selecting, ctrl-multi-select, clear, and `isSelected` behave per
  the description. Full visual verification happens in Phase 4 once composed.
