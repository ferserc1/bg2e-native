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
