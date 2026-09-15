# Reflection Widgets

**Headers:** `<bg2e/ui/ReflectionWidget.hpp>`, `<bg2e/ui/ComponentInspector.hpp>`
**Namespace:** `bg2e::ui`

Generic UI generated at runtime from `bg2e::reflection` metadata. They turn a
[`TypeInfo`](../reflection/TypeInfo.md) description into an ImGui form without
any per-type UI code. See the
[reflection API docs](../reflection/index.md) for the metadata model these
widgets consume.

```cpp
class BG2E_API ReflectionWidget {
public:
    static bool drawProperties(void* instance,
                               const reflection::TypeInfo& info,
                               uint32_t depth = 0);   // true if any property changed
    static void drawActions(void* instance,
                            const reflection::TypeInfo& info);
    // protected: drawProperty / drawScalarProperty / drawObjectProperty
};

class BG2E_API ComponentInspector {
public:
    void setNode(scene::Node* node);        scene::Node* node() const;
    void draw();
    void onChanged(std::function<void()> cb);
};
```

---

## `ReflectionWidget::drawProperties`

For every `PropertyInfo` in `info.properties` it picks a widget from the
combination `(PropertyType, PropertyEditor, metadata.min/max/step)` and wires
it to the type-erased accessors:

| `PropertyType` | Widget(s) |
|----------------|-----------|
| `Bool` | `BasicWidgets::checkBox` |
| `Int` / `UInt` | number input; `Slider` → `sliderInt(min,max)`; `Drag` → `drag(speed=step)` |
| `Float` | number input; `Slider` → `sliderFloat(min,max)`; `Drag` → `drag(step, clamped only when metadata gives a range)`; `Angle` → slider when a range exists, else drag (speed default `0.5`) |
| `Double` | number input; other editors go through a `float` temporary |
| `String` | `Input::text` |
| `Vec2/3/4`, `Mat4`, `Color` | matching `Input` widget |
| `Enum` | **fallback label** `"<name>: <enum not supported>"` (see limitation below) |
| `Path` | **read-only label** `"<name>: <path>"` |
| `Resource` / unknown | label `"<name>: <not supported>"` |
| `Object` | nested tree (see below) |

Mechanics:

- **Grouping**: properties are grouped by `metadata.category` preserving
  first-appearance order; each non-empty category becomes a collapsed tree
  (`beginTree`), so registration order controls layout.
- **Labels**: the visible name is `metadata.displayName` (fallback `name`),
  and the ImGui id is forced unique with `"##name"` — safe to draw the same
  `TypeInfo` for several instances side by side.
- **Read-only**: `isReadOnly()` properties are wrapped in
  `beginDisabled()/endDisabled()` — visible, not editable (for `Object`
  properties: no `objectMutableGetter`).
- **Tooltips**: `metadata.tooltip` is attached to the last drawn widget.
- **Write-back**: the getter's `std::any` is `any_cast` to the concrete type,
  edited through the widget, and only pushed back through the setter when the
  widget reports a change (matching the
  [reflection conventions](../reflection/index.md)).

### Object properties (`PropertyType::Object`)

Rendered as a collapsible tree. Consumption mirrors the reflection rules:

- Sub-object resolved via `objectGetter`; if its `objectTypeName` is **not
  registered** (or the instance pointer is null) a disabled
  `"<name>: <not registered>"` label appears instead of a tree.
- Recurses via `drawProperties(sub, objectInfo, depth + 1)` — capped at
  `reflection::maxObjectDepth`; deeper levels draw `"<name>: <max depth
  reached>"`.
- Read-only sub-objects draw their whole form inside `beginDisabled()` and
  changes are swallowed (so a read-only parent never mutates a mutable
  child by accident). Actions of the sub-object are drawn too.

### `Enum` limitation (v1)

The concrete enum type is erased inside `std::any` by the reflection getters,
and `std::any_cast` requires the exact type — so the widget cannot read a
generic enum value back. It therefore falls back to a label instead of a
combo. Until this is resolved, edit enums with a hand-written
`Input::comboBox()` (as `LightEditor` does) rather than relying on the
generic form. `Path` is likewise label-only for now.

### `drawActions`

One `BasicWidgets::button` per `ActionInfo` (`displayName` or `name`, ID from
`name`), calling `action.invoke(instance)` on click, with tooltip.

---

## `ComponentInspector`

The node-component front-end for `ReflectionWidget`:

```
<Node name>
[ Add Component ]            <- stub (no implementation yet)
--- Components ---
> [Transform]              (collapsingHeader, display name from TypeInfo)
    ... reflected properties/actions via ReflectionWidget ...
> [Drawable]
    No reflection data for 'Drawable'
    [Remove]
```

- Iterates `node->orderedComponents()`; per component it looks up
  `TypeRegistry::get().type(comp->typeName())`.
- Components **without** reflection data are still listed (with the
  "No reflection data" note) and can still be removed.
- The **Remove** button is right-aligned on the header row
  (`sameLine(-calcButtonWidth("Remove") - spacing)`).
- Removal is *deferred* until after the iteration finishes (a `pendingRemove`
  shared_ptr) so the component vector is never mutated mid-loop — you can
  click Remove on any component safely.
- `onChanged()` fires on any property change **or** component removal —
  hook your dirty flags to it.
- `setNode(nullptr)` → draws `"No selection"`.

Because both widgets read the registry at draw time, registering a type after
startup shows up in the next frame with no invalidation.

---

## Wiring a full inspector panel

```cpp
// left panel: hierarchy            right panel: properties
_sceneTree.setRootNode(scene->rootNode());
_sceneTree.setSelectionManager(_selectionMgr.get());

// react to selection (3D pick or tree click) by pointing the inspector
_selectionMgr->onSelect([this]() {
    std::vector<scene::Node*> nodes;
    for (const auto& item : _selectionMgr->selectedItems()) {
        if (auto* n = item->nodePtr()) nodes.push_back(n);
    }
    _nodeEditor.setNodes(nodes);
    _inspector.setNode(nodes.size() == 1 ? nodes.front() : nullptr);
});
```

---

## See also

- [quick_start — Recipe 13](quick_start.md#recipe-13-reflection-driven-inspectors)
- [reflection API](../reflection/index.md) — `PropertyInfo`, `TypeInfo`,
  `PropertyEditor`, object properties, `maxObjectDepth`.
- [NodeEditor](SceneEditors.md#nodeeditor) — the hand-written alternative.
