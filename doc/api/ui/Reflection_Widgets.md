# Reflection Widgets

**Headers:** `<bg2e/ui/ReflectionWidget.hpp>`, `<bg2e/ui/ComponentInspector.hpp>`

**Namespace:** `bg2e::ui`

Runtime-generated ImGui forms backed by `bg2e::reflection` metadata.

```cpp
class BG2E_API ReflectionWidget {
public:
    static bool drawProperties(void* instance, const reflection::TypeInfo& info,
                               uint32_t depth = 0);
    static void drawActions(void* instance, const reflection::TypeInfo& info);
};

class BG2E_API ComponentInspector {
public:
    using ResourceChangedCallback = std::function<bool(
        scene::Component*, const std::string& propertyName,
        const std::filesystem::path& previousPath,
        const std::filesystem::path& selectedPath)>;
    void setNode(scene::Node* node);
    scene::Node* node() const;
    void draw();
    void onChanged(std::function<void()> callback);
    void onResourceChanged(ResourceChangedCallback callback);
};
```

## `ReflectionWidget`

`drawProperties()` groups properties by category in first-appearance order,
uses `metadata.displayName` as the visible label, attaches tooltips, and returns
`true` if any setter accepted a changed value. Read-only properties remain
visible inside a disabled group.

| Property type | Generic editor |
|---------------|----------------|
| `Bool` | Checkbox |
| `Int`, `UInt`, `Float`, `Double` | Input, slider, drag, or angle editor according to metadata |
| `String` | Text field |
| `Vec2`, `Vec3`, `Vec4`, `Mat4` | Matching vector/matrix editor |
| `Color` | Color picker |
| `Enum` | Combo box populated by `enumOptions` |
| `Path` | Visible path label |
| `Resource` | [`ResourcePicker`](ResourcePicker.md), preserving String/Path storage |
| `Object` | Nested reflected tree |
| `PolymorphicObject` | Subtype selector plus base and derived reflected fields |

Enum access is uniformly erased as `int64_t`. An empty option list produces a
disabled `<no enum options>` item. If the current value is not registered, the
combo adds `Unknown (<value>)` without changing the object; selecting a known
option replaces it. Enum properties use a combo regardless of an explicit
non-combo editor hint.

### Numeric interaction

Scalar and vector properties with neither `metadata.min` nor `metadata.max` use
mouse-drag numeric fields. Drag horizontally to change the value; Ctrl+click or
double-click a field to enter a number directly. This includes TransformComponent
translation and scale, and the decomposed rows of matrix editors. The metadata
step controls drag speed (default 1 for integers, 0.1 for floating-point values).
Unsigned integers and doubles retain their native storage precision while dragging.

Properties with bounds retain their metadata-selected editor. Transform rotation,
for example, continues to use its bounded angle slider. Read-only fields remain
disabled. The same behavior applies to transient values edited through `editValue`.

### Object properties

An `Object` is edited in place through its object accessors. Missing reflection
data, null objects, and depth beyond `reflection::maxObjectDepth` produce safe
diagnostic labels. A read-only parent disables the complete nested form. Nested
actions are drawn after its fields.

### Polymorphic object properties

The selector is built from `TypeRegistry::subtypes(base)`, optionally filtered
and ordered by `polymorphicSubtypeKeys`. A property with a replacer can construct
and install a registered subtype; otherwise the selector is disabled. Null
values can still be configured when a replacer and factory are available.

After selection, the widget reacquires the pointers and draws both base and
active-derived reflection metadata. Registered checked casts are used before
derived fields are exposed. Null values, unknown dynamic types, missing subtype
reflection, unavailable allowlist entries, and failed replacement are reported
without unsafe casts. The normal depth limit and read-only behavior also apply.

`drawActions()` renders one button per `ActionInfo` and invokes it on click.

## `ComponentInspector`

### Queued application intent mode

The default inspector still edits production components directly and uses the
existing safe removal path. Draw Composer reuses the same Add Component/dropdown
and collapsible Remove headers with three optional callbacks:

- `onAddRequested(std::function<void(const std::string&)>)`: receives a registered
  type name instead of constructing/attaching it during UI drawing.
- `onRemoveRequested(std::function<void(std::shared_ptr<scene::Component>)>)`:
  receives the component instead of scheduling the default removal.
- `onDrawComponent(std::function<void(std::shared_ptr<scene::Component>)>)`:
  draws application-controlled property contents instead of immediate reflected
  property/action calls. This lets the app stage CPU properties without cloning
  Drawable GPU resources during UI drawing.

Applications own weak node/document/renderer guards, stopped resource realization,
validation/rollback, dirty state and bake/history invalidation. Empty callbacks
restore the corresponding default path. Keep widget IDs/scopes balanced in custom
draw callbacks and clear callbacks before app teardown.

The inspector iterates `node->orderedComponents()`, resolves each component's
`TypeInfo`, and delegates fields and actions to `ReflectionWidget`. Components
without reflection metadata are still listed and removable. Removal is deferred
until iteration has finished. `setNode(nullptr)` draws `No selection`.

`onChanged()` fires once for an accepted property change or component removal.
For top-level Resource properties, the inspector snapshots the previous path
and calls `onResourceChanged(component, propertyName, previous, selected)`.
Returning `false` rejects the change and restores the previous reflected value;
returning `true` accepts it and then triggers `onChanged()` once. Applications
use this hook to validate and load selected resources.

## See also

- [ResourcePicker](ResourcePicker.md)
- [Reflection API](../reflection/index.md)
- [NodeEditor](SceneEditors.md#nodeeditor)

## Transient value editing

`ReflectionWidget::editValue` uses transient accessor proxies for a supplied
value without invoking the original target setters or traversing raw object
pointers. GraphicsSettingsWindow uses it before queueing live changes; see
[draw settings](../draw/Settings.md).


## Reversible live property edits

`ComponentInspector::onEditRequested(EditIntentHandler)` connects typed
Begin/Update/Commit/Cancel intents to an exclusive mutation owner. The handler
receives the component, interaction ID and `ReflectionEditIntent`; the widget
does not call the live property setter or `onChanged` in this mode.
`commitInteraction()` asks the owner to adopt the final value;
`cancelInteraction()` requests restoration. `interactionActive()` stays true
until the session is settled, including pending or failed cancellation. Retain
and retry cancellation before changing nodes, callbacks or destroying the editor.

Composer captures the baseline once and applies each accepted Update at its
renderer boundary. Supported scalar/vector/color edits, including environment
colors and skybox blur, therefore appear during the gesture and produce one
history entry at Commit. Environment image replacement retains its prepared
resource command. Paths and subtypes are resolved and validated at operation
time; custom components and competing transform writers still require adapters.
