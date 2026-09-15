# TextureWidgets

**Header:** `<bg2e/ui/TextureWidgets.hpp>`
**Namespace:** `bg2e::ui`

Renders a single `render::Texture` slot as an ImGui image: a preview, a
clickable image-button, and a ready-made "pick a file" row. It is the widget
behind every texture slot of the [`MaterialEditor`](Material_Editors.md).

Internally it maintains a `VkDescriptorSet` created with
`ImGui_ImplVulkan_AddTexture()` that references the texture's sampler and
image view — which is why this widget has a lifecycle and a deferred-update
rule that plain widgets don't.

```cpp
class BG2E_API TextureWidgets {
public:
    void setEditTexture(std::shared_ptr<render::Texture> tex);   // immediate rebind
    void setDeferredTexture(std::shared_ptr<render::Texture> tex); // next draw rebind
    void clearTexture();

    void drawImage(uint32_t width, uint32_t height, bool sameLine = false);
    bool imageButton(const std::string& id, uint32_t w, uint32_t h, bool sameLine = false);

    bool selectTexture(const std::string& label,
                       std::function<std::shared_ptr<render::Texture>(base::Texture* tex)> textureCallback);

    void cleanup();
};
```

---

## The immediate vs deferred rule

`setEditTexture()` releases the old descriptor set *synchronously*. Releasing
an ImGui Vulkan texture calls `device().waitIdle()` first, because the set may
be referenced by in-flight draw data:

```
mid-frame (inside draw / callbacks)   ->  use setDeferredTexture()
between frames (events, loaders)      ->  setEditTexture() is fine
```

`setDeferredTexture()` merely stores the new texture; the swap is performed
on the next `drawImage()`/`imageButton()` call via `updateDeferredTexture()`,
which runs at the start of the frame — safe and invisible to the caller.

> Practical consequence: inside a `selectTexture` callback (which runs
> *during* drawing) the widget already applies the result deferredly — you do
> not need to call `setEditTexture` yourself for the preview to update.

---

## `selectTexture` — the picker row

```cpp
bool picked = widget.selectTexture("Albedo", [&](base::Texture* tex) {
    auto owned = std::shared_ptr<base::Texture>(tex);      // you take ownership!
    material->materialAttributes().setAlbedoTexture(owned);
    material->updateTextures();                            // upload for rendering
    return material->albedoTexture();                      // what the preview shows
});
```

Draws a 42×42 image button (the current preview), a **Clear** button and the
label text. Behavior:

| User action | Callback receives | Effect |
|-------------|-------------------|--------|
| Click preview → choose file | non-null `base::Texture*` configured with the file path, linear filters, mipmaps | you return the `render::Texture` to display |
| Click **Clear** | `nullptr` | you return the (now empty) display texture, e.g. `nullptr` |

- The raw `base::Texture*` is **heap-allocated and owned by your callback** —
  always wrap it (`std::shared_ptr<base::Texture>`) or delete it; the widget
  never frees it.
- The returned `std::shared_ptr<render::Texture>` is queued with
  `setDeferredTexture`.
- The return value is `true` only when a **file was picked** (Clear returns
  `false`).
- If `label` starts with `##` the caption is hidden and the widget generates
  internal IDs (`"imagePick" + label`) — use that for icon-only slots;
  otherwise the label is drawn to the right of the preview.
- If no texture is bound, `imageButton()` draws nothing and returns `false`
  (there is no placeholder image); `selectTexture` still draws the Clear
  button and label.

---

## Ownership and cleanup

- The widget holds `shared_ptr<render::Texture>` — it *keeps the texture
  alive* while bound. When the owning material dies, call `cleanup()` (or
  bind a new texture) or the texture (and its GPU memory) leaks by reference.
- `cleanup()` releases the descriptor set (with `waitIdle`) and drops both the
  current and deferred texture references. Engine composite widgets
  (`MaterialEditor`, `model_edit` panels) call it from their own `cleanup()`.
- `initDS()` dereferences `texture->sampler()` / `image()->imageView()`: the
  texture must already be **uploaded** (`render::Texture::create()` / engine
  texture cache) before binding it to the widget.

---

## See also

- [quick_start — Recipe 9](quick_start.md#recipe-9-texture-widgets-and-deferred-texture-swaps)
- [Material_Editors.md](Material_Editors.md) — the six-slot usage.
- `render/Texture.hpp` for the resource side.
