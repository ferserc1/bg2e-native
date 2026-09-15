# Scene Editors

**Headers:** `<bg2e/ui/SceneTree.hpp>`, `<bg2e/ui/NodeEditor.hpp>`,
`<bg2e/ui/LightEditor.hpp>`, `<bg2e/ui/CameraSettings.hpp>`,
`<bg2e/ui/PolarTransformControllerEditor.hpp>`
**Namespace:** `bg2e::ui`

Widgets that visualize and edit the scene graph. The leaf editors
(`LightEditor`, `CameraSettings`, `PolarTransformControllerEditor`) all share
the same shape — *set component → `draw()` per frame → `onChanged(cb)` →
`cleanup()`* — with the component stored as a `weak_ptr`. `SceneTree` and
`NodeEditor` are the composition-level widgets.

---

## `SceneTree`

```cpp
class BG2E_API SceneTree {
public:
    void setRootNode(scene::Node* root);        scene::Node* rootNode() const;
    void setSelectionManager(manipulation::SelectionManager*);
    manipulation::SelectionManager* selectionManager() const;
    void draw();
};
```

Draws the children of `rootNode()` recursively as tree nodes (named by
`Node::name()`, `(unnamed)` when empty). It is **stateless**: expansion state
lives in ImGui, selection lives in the `SelectionManager`.

| Manager | Behavior |
|---------|----------|
| `nullptr` (or never set) | Read-only hierarchy preview: no highlight, clicks do nothing. |
| set | Row highlight follows `SelectionManager::isSelected(node)`; click replaces the selection (`deselect()` + `addToSelectedItems(node)`). |
| set + `multiSelection()` + Ctrl held | Click **toggles** the node in the selection. |

Notes:

- Selection made elsewhere (3D pick) appears in the tree automatically — the
  widget queries the manager each row, every frame.
- The root node itself is not shown (its children are the top-level rows).
- Pair it with a `SelectionManager` shared by the 3D view, the status bar and
  the property panels so all stay in sync (that is the intended "single source
  of truth" design).

---

## `NodeEditor`

```cpp
class BG2E_API NodeEditor {
public:
    void init(render::Engine* engine);          // needed by "Replace Model..."

    void setNode(scene::Node* node);
    void setNodes(const std::vector<scene::Node*>& nodes);   // >1 -> placeholder
    scene::Node* node() const;

    void draw();
    void onChanged(std::function<void()> cb);
};
```

A per-node property panel. Sections appear automatically based on which
components the node owns:

```
<name text field>      Node::name()
[ ] Enabled            Node::enabled()
--- Components ---     list of comp->typeName() (informational)
--- Transform ---      Position / Rotation(deg) / Scale  (Polar decomposition of TransformComponent)
--- Drawable ---       "Replace Model..." button (file dialog -> db::loadDrawableBg2)
--- Environment ---    current HDR file + "Select Image..." button
--- Light ---          embedded LightEditor (+ PolarTransformControllerEditor when present)
--- Camera ---         embedded CameraSettings
```

Gotchas worth knowing:

- **Multi-selection**: `setNodes({a, b, ...})` with more than one node draws
  `<multiple_selection>` and nothing else — no bulk edit support (yet).
- **Euler cache**: the transform rows use a rotation cache keyed by the *node
  pointer*; switching selection re-extracts angles, editing the same node is
  stable (same mechanism as [`Vector::mat4`](Vector.md#matrix-editor-mat4-and-the-euler-cache)).
- **`"Replace Model..."` is a GPU-thread affair**: the loaded drawable is
  applied through `app::MainLoop::safeUpdateScene()` so the scene graph is
  never mutated while a visitor is traversing it. When you build similar
  buttons, do the same (see `doc/safe_update_scene.md`).
- `init(engine)` is required only for the drawable section (model loading);
  the editor works without it for everything else.
- Changes in the embedded editors propagate through the single `onChanged`
  callback.

---

## `LightEditor`

```cpp
class BG2E_API LightEditor {
public:
    void setLightComponent(scene::LightComponent*);          // or shared_ptr overload
    void setIntensityRange(float min, float max);            // default 0 .. 30
    bool draw();
    void cleanup();
    void onChanged(std::function<void()> cb);
};
```

Edits the `base::Light` inside a `scene::LightComponent`. Layout:

```
Basic properties
  Light Type        combo: OMNI / SPOT / DIRECTIONAL / DISABLED
  Light Color       colorPicker
  Light Intensity   slider (_intensityMin .. _intensityMax)
Shadows
  [ ] Cast Shadows
  [ ] Affect Reflections
  Source Size       slider 0.01..5      (only while Cast Shadows)
  Shadow Samples    slider 1..32        (only while Cast Shadows)
Spot
  Spot Angle        slider 1..90        (only when TypeSpot)
  Spot Cutoff       slider 1..90        (only when TypeSpot)
```

The component is held weakly: `draw()` returns `false` when the node or
component has died. Adjust `setIntensityRange()` to your renderer's expected
photometric range (the default 0–30 suits the forward renderer's units).

---

## `CameraSettings`

```cpp
class BG2E_API CameraSettings {
public:
    void setCameraComponent(scene::CameraComponent*);        // or shared_ptr overload
    bool draw();
    void cleanup();
    void onChanged(std::function<void()> cb);

    static const std::vector<std::string>& sensorNames();    // 4 presets
    static float sensorSize(uint32_t index);                 // 22/35/50/75 mm
    static uint32_t getSensorIndex(float frameSize);         // nearest preset
};
```

Edits only the **optical** projection parameters (focal length, sensor
size). Behavior branches on whether the camera already has a
`math::OpticalProjection`:

- **No optical projection** → the widget shows a single *"Configure
  Projection"* button that replaces the camera's projection with an
  `OpticalProjection` (defaults 35 mm / 35 mm frame / far 1000).
- **Optical projection present** → focal-length slider (18–200 mm) + sensor
  combo (`APS-C 22`, `Full Frame 35`, `Medium Format 50`, `Large Format 75`).

`setCameraComponent()` snapshots focal length / frame size from the
projection (or the 35 mm defaults) — so if you edit the projection
programmatically, re-set the component to refresh the widget fields.
`getSensorIndex()` is a public helper to map an arbitrary frame size to the
closest preset (used for initial combo selection).

---

## `PolarTransformControllerEditor`

```cpp
class BG2E_API PolarTransformControllerEditor {
public:
    void setComponent(scene::PolarTransformControllerComponent*);   // or shared_ptr overload
    bool draw();
    void cleanup();
    void onChanged(std::function<void()> cb);
};
```

Edits the `scene::PolarTransformControllerComponent` (the orbit controller
used by scene cameras):

```
Position      Azimuth 0..360  Elevation -90..90  Distance 0..50
Orientation   Euler X/Y/Z -360..360
Target        vec3 input
```

All ranges are fixed; the target is the orbit pivot in world space.
`NodeEditor` embeds this editor automatically under a node's Light section
when the node has both a `LightComponent` and a
`PolarTransformControllerComponent` (the typical sun/sky-light rig).

---

## See also

- [quick_start — Recipes 11 & 12](quick_start.md#recipe-12-scene-tree-selection)
- [Material_Editors.md](Material_Editors.md)
- `doc/safe_update_scene.md`, `manipulation::SelectionManager` header.
