# Phase 3 — `bg2e::ui::NodeEditor` + component editors (reusable engine widget)

**Area:** engine (`lib/include/bg2e/ui`, `lib/src/bg2e/ui`)
**Goal:** A generic, reusable node property editor that lists a node's components
and renders a built-in editor for each supported component type. Built
standalone; not wired into the app yet.

**End state:** the engine library compiles with `NodeEditor` available. Given a
single node it lists component names and shows editors for `Transform`,
`Drawable`, `Environment` and `Light`. Given multiple nodes it shows
`<multiple_selection>`.

---

## Files

- `lib/include/bg2e/ui/NodeEditor.hpp`
- `lib/src/bg2e/ui/NodeEditor.cpp`
- Add `#include <bg2e/ui/NodeEditor.hpp>` to `lib/include/bg2e/ui/all.hpp`.

Standard GPL license header.

## Public API

```cpp
namespace bg2e { namespace ui {

class BG2E_API NodeEditor {
public:
    using ChangedCallback = std::function<void()>;

    void init(bg2e::render::Engine * engine);   // needed to (re)load drawables/env

    void setNode(bg2e::scene::Node * node);     // single node
    void setNodes(const std::vector<bg2e::scene::Node*>& nodes); // for <multiple_selection>
    bg2e::scene::Node * node() const { return _node; }

    void draw();

    void onChanged(ChangedCallback cb) { _onChanged = cb; }

protected:
    bg2e::render::Engine * _engine = nullptr;
    bg2e::scene::Node * _node = nullptr;
    size_t _selectionCount = 0;

    // Reused engine editors
    bg2e::ui::LightEditor _lightEditor;
    bg2e::ui::PolarTransformControllerEditor _polarEditor;

    ChangedCallback _onChanged;
    void notifyChanged() const;

    void drawComponentList();
    void drawTransformEditor(bg2e::scene::TransformComponent * t);
    void drawDrawableEditor(bg2e::scene::DrawableComponent * d);
    void drawEnvironmentEditor(bg2e::scene::EnvironmentComponent * e);
    void drawLightEditor(bg2e::scene::LightComponent * l);
};

}}
```

## Drawing

`draw()`:
- If `_selectionCount > 1`: render `BasicWidgets::text("<multiple_selection>")`
  and return.
- If `_node == nullptr`: render nothing (or `text("No selection")`).
- Otherwise:
  1. `drawComponentList()` — `BasicWidgets::separator("Components")` then one
     `BasicWidgets::text(componentName(c))` per `_node->orderedComponents()`
     (use the free function `bg2e::scene::componentName(Component*)` or
     `c->typeName()`).
  2. Dispatch by querying typed components from the node (cheap and explicit;
     no need for a registry in this phase):
     ```cpp
     if (auto t = _node->getComponent<scene::TransformComponent>())   drawTransformEditor(t);
     if (auto d = _node->getComponent<scene::DrawableComponent>())    drawDrawableEditor(d);
     if (auto e = _node->getComponent<scene::EnvironmentComponent>()) drawEnvironmentEditor(e);
     if (auto l = _node->getComponent<scene::LightComponent>())       drawLightEditor(l);
     ```

### Transform editor

Decompose `t->matrix()` into translation / euler rotation / scale, expose three
`Input::vec3` fields, recompose on change. Keep it simple (no over-engineering):

- Translation: 4th column of the matrix (`m[3].xyz`).
- Scale: lengths of the first three column vectors
  (`length(m[0]), length(m[1]), length(m[2])`).
- Rotation: extract euler angles from the rotation part (columns normalized by
  scale). Use GLM helpers (`glm::extractEulerAngleXYZ` on the rotation matrix, or
  `glm::eulerAngles(glm::quat_cast(rot))`); display in degrees.
- On any field change: rebuild
  `T * R * S` (`glm::translate * eulerAngleXYZ(radians) * glm::scale`) and call
  `t->setMatrix(m)`, then `notifyChanged()`.
- Acceptable limitation: euler round-trips can drift for extreme rotations; fine
  for the minimal editor. Cache the displayed euler values per node so typing is
  stable (store last-edited values keyed by the current node pointer; reset the
  cache when `setNode` changes the node).

```cpp
BasicWidgets::separator("Transform");
if (Input::vec3("Position", pos)) changed = true;
if (Input::vec3("Rotation", eulerDeg)) changed = true;
if (Input::vec3("Scale", scale)) changed = true;
if (changed) { t->setMatrix(compose(pos, eulerDeg, scale)); notifyChanged(); }
```

### Drawable editor

```cpp
BasicWidgets::separator("Drawable");
if (BasicWidgets::button("Replace Model...")) {
    auto path = bg2e::app::FileDialog::getOpenFilePath({ { "bg2e 3D model", "bg2,vwglb" } });
    if (!path.empty()) {
        auto drawable = bg2e::db::loadDrawableBg2(path, _engine); // shared_ptr<Drawable>
        // Swap safely: drawable resources may be in use
        bg2e::app::MainLoop::current()->safeUpdateScene([d, drawable]() {
            d->setDrawable(drawable);
        });
        notifyChanged();
    }
}
```

(Full per-submesh material editing remains in the app's left "Model Properties"
panel; here only the replace-model action is required by the spec.)

### Environment editor

```cpp
BasicWidgets::separator("Environment");
BasicWidgets::text(std::filesystem::path(e->environmentImage()).filename().string());
if (BasicWidgets::button("Select Image...")) {
    auto path = bg2e::app::FileDialog::getOpenFilePath({ { "HDR Environments", "hdr,jpg,png" } });
    if (!path.empty()) { e->setEnvironmentImage(path.string()); notifyChanged(); }
}
```

The renderer picks up the new image via `imgHash()` on the next update; if a
manual refresh is needed, the app may call `scene->updateEnvironment()` in its
`onChanged` handler (Phase 4/5 wiring).

### Light editor

Reuse the existing widgets exactly as `EnvironmentSettings` did:

```cpp
BasicWidgets::separator("Light");
_lightEditor.setLightComponent(
    std::static_pointer_cast<scene::LightComponent>(l->shared_from_this()));
if (_lightEditor.draw()) notifyChanged();

if (auto polar = _node->getComponent<scene::PolarTransformControllerComponent>()) {
    _polarEditor.setComponent(polar);
    if (_polarEditor.draw()) notifyChanged();
}
```

(Check `LightEditor::setLightComponent` exact signature — it took a
`shared_ptr<LightComponent>` in `EnvironmentSettings`.)

## Includes

`NodeEditor.hpp` needs: `scene/Node.hpp`, the component headers (`Transform`,
`Drawable`, `Environment`, `Light`, `PolarTransformController`), `LightEditor.hpp`,
`PolarTransformControllerEditor.hpp`, `render/Engine.hpp` (forward-declare where
possible). `.cpp` adds `db/mesh_bg2.hpp`, `db/scene.hpp` (if needed),
`app/FileDialog.hpp`, `app/MainLoop.hpp`, `BasicWidgets.hpp`, `Input.hpp`, GLM
transform headers.

## Notes / constraints

- Single-node editing only; multiple selection → `<multiple_selection>`.
- No public component-editor registration API yet (dispatch is hard-coded). This
  is intentionally minimal per the spec.
- Do not modify CMake.

## Verification

- Engine library builds with the new files (when building is explicitly
  requested).
- Logic review of the transform decompose/compose round-trip and the dispatch.
  Visual verification happens in Phase 4.
