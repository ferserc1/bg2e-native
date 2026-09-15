# Material and Drawable Editors

**Headers:** `<bg2e/ui/MaterialEditor.hpp>`, `<bg2e/ui/DrawableEditor.hpp>`,
`<bg2e/ui/SubmeshSelector.hpp>`
**Namespace:** `bg2e::ui`

The PBR editing widgets. They come in two flavors of data binding: **manual**
(you hand them a `render::MaterialBase`) and **selection-driven** (you hand
them a `manipulation::SelectionManager` and they follow the 3D picks).

---

## `MaterialEditor`

```cpp
class BG2E_API MaterialEditor {
public:
    // Manual mode
    void setEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    void addEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    void clearMaterial();

    // Selection-driven mode (disables the manual setters)
    void setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>& sm);

    std::shared_ptr<render::MaterialBase> editMaterial();          // main material
    bool draw();
    void cleanup();
    void onChanged(std::function<void()> cb);
};
```

### What it draws

A collapsing header (`"<name>'s Material Attributes"`) with one section per
PBR map: **Albedo** (color, texture, scale, UV set, transparency, refraction),
**Normal**, **Metallic**, **Roughness** (value, texture, channel, invert,
scale, UV set), **Fresnel Tint**, **Sheen** (intensity, color), **Ambient
Occlusion**, **Light Emission**. Texture slots are
[`TextureWidgets`](TextureWidgets.md) `selectTexture()` rows — picking a file
writes a new `base::Texture` into the material attributes and calls
`material->updateTextures()` for every material being edited.

### Multi-material editing

The editor always holds a **list** of materials:

- Manual mode: `setEditMaterial()` replaces the list with one material;
  `addEditMaterial()` appends more (the first one added defines the values
  shown in the fields).
- Selection mode: on every selection change, the list becomes *all submesh
  materials of the first selected drawable* — i.e. selecting two submeshes of
  the same model edits both in lockstep.

Every widget edit is applied to **all** materials in the list, and
`onChanged()` is invoked per material. `editMaterial()` returns the main
(first) material, or `nullptr` when nothing is selected.

### Mode exclusivity

Once `setSelectionManager()` is set, the manual setters are silently ignored
(early return). If you need both behaviors, keep one editor per mode.

### Lifetime gotcha

`setSelectionManager()` registers a callback with `[&]` capture on the
selection manager's `onSelect` signal. The editor must not outlive the
selection manager — or vice versa, the manager must not fire the callback
after the editor is destroyed. Put both under the same owner (the delegate)
and clean the editor up first:

```cpp
void AppDelegate::cleanup()
{
    _materialEditor.cleanup();      // releases the 6 texture descriptor sets
}
```

---

## `DrawableEditor` and `SubmeshSelector`

```cpp
class BG2E_API DrawableEditor {
public:
    void init(const std::shared_ptr<manipulation::SelectionManager>& sm);
    bool draw();
    void cleanup();
    void onChanged(std::function<void()> cb);

    int32_t selectedItem() const;                 // first selected submesh, or -1
    std::vector<uint32_t> selectedItems() const;
    SubmeshSelector& submeshSelector();
};

class BG2E_API SubmeshSelector {
public:
    void init(std::shared_ptr<manipulation::SelectionManager> sm);
    std::shared_ptr<scene::Drawable> editDrawable();      // first drawable in selection
    int32_t selectedItem() const;
    std::vector<uint32_t> selectedItems() const;
    void addSelectedItem(uint32_t index) const;           // programmatic selection
    bool draw();
    void cleanup();
};
```

`SubmeshSelector` is *always* selection-driven: `init()` installs an
`onSelect` hook that snapshots the first non-expired `SelectionItem` owning a
drawable — that drawable becomes the "edit drawable".

`DrawableEditor` composes a `SubmeshSelector` with a small property panel.
Its `draw()` renders:

```
[<drawable name>]                 (collapsing header; hidden when nothing selected)
  Submeshes                       SelectableList, one row per submesh:
    0 - Body          (click toggles selection in the SelectionManager)
    1 - Glass
    Clear Selection   (deselects everything)
  Submesh Properties              shown when selectedItem() != -1
    Name              edits ONLY the first selected submesh
    Group Name        applied to ALL selected submeshes
    Visibility        applied to ALL selected submeshes
```

When more than one submesh is selected the header reads
`"Submesh Properties (N items selected)"` and the Name field shows the
*effective* (first) name in parentheses to signal that it is not a shared
value.

### Programmatic access

```cpp
auto drawable = _drawableEditor.submeshSelector().editDrawable();
if (drawable && _drawableEditor.selectedItem() >= 0) {
    auto idx = _drawableEditor.selectedItem();
    drawable->setSubmeshName("Wheel", idx);
}
```

Selection queries (`selectedItem()`, `selectedItems()`, `editDrawable()`)
delegate to the live `SelectionManager` state, so they are valid in any
callback, not only during `draw()`.

> **Why the "first drawable only"?** A multi-selection spanning two different
> models cannot express "submesh 3 of the *other* model" as a single value, so
> the widget restricts submesh-level editing to the first selected drawable.
> The 3D view still highlights every selected submesh.

### Cleanup

Both editors hold weak/shared references to engine data and (indirectly)
descriptor-backed texture widgets. Call `cleanup()` from your delegate's
`cleanup()`, before the engine is torn down.

---

## See also

- [quick_start — Recipes 10 & 11](quick_start.md#recipe-10-material-editor-driven-by-a-selectionmanager)
- [TextureWidgets](TextureWidgets.md) — the slot widget used internally.
- `manipulation::SelectionManager` docs / header for the selection model.
