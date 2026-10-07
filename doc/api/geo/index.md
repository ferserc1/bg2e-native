# Geometry API

The `bg2e::geo` namespace contains CPU-side mesh data, procedural geometry,
mesh modifiers, and geometry validation. Its umbrella header is
`<bg2e/geo/all.hpp>`.

UV-atlas generation and validation do not require an engine or graphics device:

```cpp
#include <bg2e/geo/all.hpp>

geo::GenerateUv2AtlasModifier atlas(mesh.get(), { 1024, 8 });
atlas.apply();
const auto uv2 = geo::UvAtlasValidator::validate(*mesh, 1);
```

`GenerateUv2AtlasModifier` packs every submesh into one xatlas atlas, preserves
UV1, and changes only CPU mesh data. `UvAtlasValidator` verifies that a chosen
UV set is suitable for atlas baking. For a loaded editor Drawable, GPU reload
must be scheduled separately through [`app::Uv2SafeReload`](../app/Uv2SafeReload.md).

## UV-atlas API

- [`GenerateUv2AtlasModifier`](GenerateUv2AtlasModifier.md)
- [`UvAtlasValidator`](UvAtlasValidator.md)

See [Lightmap baking](../../lightmap_baking.md) for the editor workflow and
[the render API](../render/index.md) for UV previews and baking.
