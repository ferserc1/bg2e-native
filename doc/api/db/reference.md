# Database API Reference

Image-format and lightmap-output symbols declared by `<bg2e/db/all.hpp>`:

| Symbol | Header | Documentation |
|--------|--------|---------------|
| `ImageFormat` | `db/image.hpp` | [ImageFormat](ImageFormat.md) |
| `imageFormatFromExtension`, `imageFormatFromPath` | `db/image.hpp` | [ImageFormat](ImageFormat.md#format-discovery) |
| `extensionsForImageFormat`, `canonicalImageExtension` | `db/image.hpp` | [ImageFormat](ImageFormat.md#extension-mapping) |
| `saveImage` | `db/image.hpp` | [ImageFormat](ImageFormat.md#saving-images) |

See [SceneReferences](SceneReferences.md) for the separate representation and
draw Engine IO. Complete resource saves return bool noexcept and require explicit
false handling; the specialized output writer lives in bake.
