# UvMapPreview

**Header:** `<bg2e/ui/UvMapPreview.hpp>`  
**Namespace:** `bg2e::ui`

`UvMapPreview` is an embeddable immediate-mode widget that displays a CPU
mesh's UV layout and validation result. It offers UV1/UV2 controls, shows
coverage or an error message, and uses `render::UvMapPreviewRenderer` plus the
existing `TextureWidgets` path to display the generated Vulkan image.

```cpp
ui::UvMapPreview preview;
preview.init(engine, 256);
preview.setMesh(drawable->mesh());
preview.setUvSet(1); // 0 = UV1, 1 = UV2

// From the UI delegate, once per UI frame:
preview.draw();
```

## Lifecycle and methods

| Member | Description |
|--------|-------------|
| `init(render::Engine*, uint32_t resolution = 256)` | Creates the renderer and image widget. Call before `draw()`. |
| `setMesh(std::shared_ptr<geo::Mesh>)` | Selects the shared CPU mesh; changing it marks the preview dirty. The widget retains the mesh, not its Drawable. |
| `mesh() const` | Returns the selected shared mesh. |
| `setUvSet(uint32_t)` / `uvSet() const` | Selects UV1 (`0`) or UV2 (`1`); values above 1 are clamped to 1. |
| `setResolution(uint32_t)` / `resolution() const` | Changes/reads the renderer target resolution. Recreating the target safely releases its displayed descriptor first. |
| `setDisplaySize(uint32_t)` / `displaySize() const` | Sets the square on-screen image side in pixels. |
| `refresh()` | Marks the current mesh/UV set dirty for re-render and re-validation on the next `draw()`. Call after modifying mesh data in place. |
| `draw()` | Draws the channel controls, validation status, and image. Call once per UI frame between `UserInterface::newFrame()` and `UserInterface::draw()`. |
| `cleanup()` | Releases the ImGui image descriptor and renderer resources. Call before the engine is destroyed. |

The widget defaults to UV2 and a 256-pixel display. `init()` must be called
before drawing. When regeneration replaces mesh data, call `setMesh()` with the
new mesh or `refresh()` after the safe reload completes. The widget stores a
shared CPU mesh only, so it does not retain a scene Drawable across scene
swaps.

## ImGui boundary

The public header and application code use the engine's UI wrappers; they do
not include or expose `<imgui.h>` types. Keep ImGui includes and direct ImGui
calls inside the engine UI implementation. The preview image is rendered by
`render::UvMapPreviewRenderer`; the widget binds it through `TextureWidgets`.

## See also

- [`render::UvMapPreviewRenderer`](../render/UvMapPreviewRenderer.md)
- [`geo::UvAtlasValidator`](../geo/UvAtlasValidator.md)
- [`app::Uv2SafeReload`](../app/Uv2SafeReload.md)
- [Lightmap baking](../../lightmap_baking.md)
