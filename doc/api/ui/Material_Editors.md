# Material and Drawable Editors

Use `<bg2e/ui/DrawMaterialEditor.hpp>`, `<bg2e/ui/DrawableEditor.hpp>` and
`<bg2e/ui/SubmeshSelector.hpp>`. The old MaterialEditor class/header is removed.

## `MaterialEditor`

The legacy class is removed; use [DrawMaterialEditor](#drawmaterialeditor).

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
    std::shared_ptr<draw::Drawable> editDrawable();      // first drawable in selection
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

## `DrawMaterialEditor`

**Header:** `<bg2e/ui/DrawMaterialEditor.hpp>`

Initialize DrawMaterialEditor with the draw
engine and a shared `draw::DefaultRenderLoopDelegate`, then optionally bind a
`SelectionManager`. Its `draw(UserInterface&)` method renders controls but
does not mutate the selected target inline. `queuePatch()` and
`queueClearTexture()` enqueue intent; weak target, renderer generation and
binding revision checks protect the later safe-boundary application.

```cpp
_drawMaterialEditor.init(engine, delegate);
_drawMaterialEditor.setSelectionManager(selection);
_drawMaterialEditor.draw(ui); // during UI drawing
```

Call `cleanup()` before the engine is destroyed. `onChanged()` lets the app
reset application-owned histories; the deferred renderer's upscaler reset is
handled automatically. A pending/error status is available through
`pending()` and `error()`.

## Baked indirect material ownership

Draw materials can mark their AO slot with
[IndirectTextureMode](../draw/BakedIndirectMaps.md). Keep stored material metadata
and the live GPU slot consistent when adopting a completed lightmap. The
integrated example handles this explicitly; the general material editor does
not become a bake scheduler or image exporter. Source edits invalidate bake
generations at a stopped boundary. Retain unbaked originals if restart must
regenerate independent UV2 and materials.


## Draw editor document guards and indirect/sampler controls

`DrawMaterialEditor::setEditGuard(std::function<bool()>)` lets an application
capture its current document generation. The guard runs before queued work;
rebinding it cancels earlier pending bindings. It complements the renderer,
selection identity, current-scene membership and weak lifetime checks. The draw
model editor rebinds on document/selection changes and uses `onChanged` to mark
dirty state and reset histories, including partial failures.

`queueTextureSettings(slot, edit)` applies a stopped callback to each target's
independent base settings, retaining that target's authored image/mips. The UI
exposes color, filters, U/V addressing, mip sampling and bias. It does not replace
other targets' source paths or untouched settings with the first displayed value.
`queueIndirectMode(mode)` requires an assigned map for baked AO/GI, retains/clones
actual GPU pixels, sets linear interpretation and UV set 1, and updates stored and
live attributes. Selecting an ordinary AO image deliberately resets baked mode;
clearing the AO slot resets it too. Missing maps or UV changes never implicitly
turn on baked interpretation. `queuePatch(modifier, ordinaryAo = false)` remains
source-compatible; ordinaryAo requests the explicit AO reset after application.

The [draw editor guide](../../apps/draw_model_edit.md) explains the actual
application integration. Generic production ComponentInspector/DrawableEditor
remain separate; these APIs do not certify arbitrary immediate component mutation.
