# Rendering API Reference

UV and lightmap-related public symbols declared by
`<bg2e/render/all.hpp>`:

| Symbol | Header | Documentation |
|--------|--------|---------------|
| `UvMapPreviewRenderer` | `render/UvMapPreviewRenderer.hpp` | [UvMapPreviewRenderer](UvMapPreviewRenderer.md) |
| `LightmapMode`, `LightmapPixelFormat`, `LightmapSettings` | `render/LightmapSettings.hpp` | [Integrated Lightmap Baker](LightmapBaker.md#settings-and-output) |
| `LightmapPixels`, `LightmapBaker` | `render/LightmapBaker.hpp` | [Integrated Lightmap Baker](LightmapBaker.md) |
| `IntegratedBakerContext`, `IntegratedLightmapBaker` | `render/IntegratedBakerContext.hpp` | [Integrated Lightmap Baker](LightmapBaker.md) |

These facilities use production `bg2e::render` Vulkan resources. They are
separate from the experimental `bg2e::gpu` abstraction.
