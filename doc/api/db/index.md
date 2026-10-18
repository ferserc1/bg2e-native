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
