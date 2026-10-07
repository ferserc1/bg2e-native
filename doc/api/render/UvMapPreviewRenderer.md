# UvMapPreviewRenderer

**Header:** `<bg2e/render/UvMapPreviewRenderer.hpp>`  
**Namespace:** `bg2e::render`

`UvMapPreviewRenderer` draws UV1 or UV2 triangles, tinted by submesh, over a
coverage fill and the unit-square boundary. It consumes a CPU `geo::Mesh`,
does not mutate it, and has no dependency on `bg2e::ui`.

```cpp
render::UvMapPreviewRenderer preview(engine, 512);
preview.render(*mesh, 1); // 0 = UV1, 1 = UV2
render::Texture* texture = preview.texture();
```

| Member | Description |
|--------|-------------|
| `UvMapPreviewRenderer(Engine*, uint32_t resolution = 512)` | Creates the square render target; engine must be non-null and resolution positive. |
| `setResolution(uint32_t)` | Recreates the target after waiting for pending device work; zero is invalid. |
| `resolution() const` | Current target side length in pixels. |
| `render(const geo::Mesh&, uint32_t uvSet)` | Synchronously renders UV1 (`0`) or UV2 (`1`) and waits for submission completion. Other set values throw `std::invalid_argument`. |
| `texture()` | Non-owning pointer to the sampled result, valid until the next `setResolution()` or renderer destruction. |

The returned texture is in `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` after
`render()` completes. The renderer owns it. Consumers must stop using its image
before changing the resolution or destroying the renderer. A render handles
malformed indices as a diagnostic view by skipping invalid triangles; use
[`UvAtlasValidator`](../geo/UvAtlasValidator.md) when validity, rather than
visualization, is required.

## See also

- [`ui::UvMapPreview`](../ui/UvMapPreview.md) — embeddable widget with UV-set selection and validation feedback.
- [Lightmap baking](../../lightmap_baking.md) — UV generation and editor integration.
