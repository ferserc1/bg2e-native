# TexturePreview

**Header:** `<bg2e/ui/TexturePreview.hpp>`  
**Namespace:** `bg2e::ui`

`TexturePreview` is an opaque, session-scoped preview for a draw texture. It
exposes `valid()`, `drawImage(width, height)` and
`imageButton(id, width, height)` without exposing ImGui or native GPU types.
The preview is created and registered by the UI implementation; applications
normally use it through [`DrawTextureWidgets`](DrawTextureWidgets.md).

Preview registrations and resources are retained until submitted UI draws have
completed. A preview becomes invalid when its `UserInterface` session is
cleaned up, so do not retain or draw it across UI/engine teardown. See the
[UI overview](index.md) for the production and draw widget split.


## Snapshot and size contract

```cpp
auto preview = ui.texturePreview(texture);  // shared_ptr<draw::Texture>
preview->drawImage(128, 64);
if (preview->imageButton("texture-details", 128, 64))
{
    openTextureDetails();
}
```

Create previews on the UI thread in an initialized draw session, using a sampleable
color texture from its engine/device. The preview retains the exact image/sampler
snapshot, so subsequent texture reloads do not replace that snapshot. Request a new
preview to display replacement resources.

`drawImage(uint32_t width, uint32_t height)` and `imageButton(id, width, height)`
keep their existing dimensions: values pass directly to UI drawing without the
icon system's automatic scaling or aspect fitting. Invalid previews draw nothing;
imageButton returns false. A valid preview used with another current UI context
is rejected. Cleanup invalidates even handles retained by callers.

[PNG icons](Icons.md) reuse the private registration/drawing and GPU-completion
lifetime mechanisms, but add a separate permanent path cache. Previews do not
become permanent cached icons. Their public signatures remain unchanged and no
ImGui/native types enter public headers. Use [Image](Image.md) or icon buttons when
the source is a PNG path rather than a live draw texture.

## Draw bake previews

[BakeLightmapPreview](../bake/BakeLightmapPreview.md) registers retained completed
bake/UV diagnostic images through the draw UI session. Use
[bake::UvMapPreviewRenderer](../bake/UvMapPreviewRenderer.md) for CPU UV1/UV2
inspection and rebind images after geometry/resolution/session changes. Production
UvMapPreview remains a separate render-compatible widget. Clear draw preview
handles before UI/Engine teardown; no ImGui/native type enters application code.
