# Database API

The `bg2e::db` namespace loads and saves engine resources through the
production database formats. Its umbrella header is `<bg2e/db/all.hpp>`.

## Image formats and lightmap output

- [`ImageFormat`](ImageFormat.md) discovers PNG, JPEG, BMP, and TGA formats
  from extensions and filesystem paths.
- [bake::LightmapOutputWriter](../bake/LightmapOutputWriter.md) owns protected batch
  output; the obsolete DB-specialized writer is removed.

## See also

- [Draw API](../draw/index.md)
- [Standalone draw lightmap CLI](../../apps/draw_lightmap_generator.md)

## Representation and draw

[Scene references and draw resource loading](SceneReferences.md) explains raw
document preservation, graphics-free typed graphs and synchronous realization.

The [bake API](../bake/index.md) publishes typed pixels and sampled GPU maps.
Its [standalone writer](../bake/LightmapOutputWriter.md) owns protected complete-
batch publication, native HDR maps and actual model snapshots. The production
DB writer above remains a separate legacy contract. Persisting mode metadata
alone does not materialize pixels or export a complete resource generation.

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

- [Shared scene/root/custom component contracts](../scene/index.md) and [Composer document workflow](../../apps/draw_composer.md).

## Generic file and native archive IO

[FileIO](FileIO.md) documents CPU/GPU TextureArchive encoding, native BG2TEX01
readback and atomic no-replace/explicit-replace publishFile. These generic helpers
remain distinct from specialized bake source protection and final-manifest policy.
ObjMesh::sourcePaths records original OBJ/read MTL inputs for dependency inventory.
