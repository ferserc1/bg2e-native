# Database API

The `bg2e::db` namespace loads and saves engine resources through the
production database formats. Its umbrella header is `<bg2e/db/all.hpp>`.

## Image formats and lightmap output

- [`ImageFormat`](ImageFormat.md) discovers PNG, JPEG, BMP, and TGA formats
  from extensions and filesystem paths.
- [`LightmapOutputWriter`](LightmapOutputWriter.md) preflights batch output
  paths and writes lightmap images and optional `.bg2` target copies.

## See also

- [Render API](../render/index.md)
- [Standalone lightmap generator CLI](../app/LightmapGenerator.md)

## Representation and experimental draw

[Scene references and draw resource loading](SceneReferences.md) explains raw
document preservation, graphics-free typed graphs and synchronous realization.

The new [integrated bake](../bake/index.md) publishes typed pixels and sampled
GPU maps, but the production LightmapOutputWriter above is not a bake-namespace
writer. Persisting indirectTextureMode metadata alone does not materialize a
GPU-only texture or guarantee complete draw model/scene export.

## Complete draw resources

[Complete draw resource IO](DrawResourceIO.md) covers native GPU texture archives,
MTL/glTF import, transactional scene/model publication and editor document state.
This explicit draw path materializes GPU-only HDR/mip resources; the production
LightmapOutputWriter retains its separate contract.

## Completed camera images

The [draw offscreen CLI](../../apps/draw_offscreen_application.md) uses
`db::saveImage(path, bytes, width, height, bpp)` and
`db::decodeImageRGBA8(encodedData, encodedSize, width, height)` from
`<bg2e/db/image.hpp>`. The writer selects PNG/JPEG/BMP/TGA from the extension;
the in-memory decoder exposes RGBA8 pixels for encoded PNG/JPEG. These calls own
no GPU transfer or job scheduling. The CLI selects PNG and compares its lossless
round-trip with completed GPU RGBA8 data. That camera-image path does not supply
complete GPU-only texture/material/scene resource export.
