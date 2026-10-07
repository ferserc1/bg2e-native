# GenerateUv2AtlasModifier

**Header:** `<bg2e/geo/GenerateUv2AtlasModifier.hpp>`  
**Namespace:** `bg2e::geo`

`GenerateUv2AtlasModifier` creates a UV2 atlas for a standard `geo::Mesh` whose
vertex type is `VertexPNUUT`. UV1 is `texCoord0`; UV2 is `texCoord1`.

```cpp
geo::Uv2AtlasOptions options;
options.resolution = 1024;
options.paddingPixels = 8;

geo::GenerateUv2AtlasModifier modifier(mesh.get(), options);
modifier.apply();
const geo::Uv2AtlasResult& result = modifier.result();
```

## Options and result

| Type | Member | Meaning |
|------|--------|---------|
| `Uv2AtlasOptions` | `resolution` | Requested square atlas packing resolution; defaults to 512. Must be greater than zero. |
| `Uv2AtlasOptions` | `paddingPixels` | xatlas chart padding in pixels; defaults to 4. |
| `Uv2AtlasResult` | `width`, `height` | Dimensions of the generated atlas. |
| `Uv2AtlasResult` | `chartCount` | Number of generated charts. |
| `Uv2AtlasResult` | `utilization` | xatlas packing utilization for the generated atlas. |

`result()` is meaningful only after `apply()` succeeds. Invalid input, xatlas
errors, multiple output atlases, invalid output faces, or overlapping charts
cause `apply()` to throw `std::runtime_error`.

## Mesh and transaction behavior

The complete source mesh is submitted as **one xatlas input mesh**. Previous
UV2 values are ignored, and all submesh triangles are packed into one atlas
image. The modifier reconstructs the
vertex and index arrays only after validating all output; if `apply()` fails,
the original mesh remains unchanged. Submesh order and triangle counts remain
the same, preserving the association between submeshes and their materials.

xatlas may duplicate vertices at chart seams. Output vertices use xatlas's
source-vertex mapping to copy the original position, normal, tangent, and UV1
values bit-for-bit; only UV2 is replaced. UV1 values are not repacked or
modified.

The operation is **CPU-only**. It does not create, update, or reload GPU
resources. For an already loaded scene Drawable, use the safe editor reload
flow described in [`Uv2SafeReload`](../app/Uv2SafeReload.md) after applying
mesh changes; a headless caller can apply the modifier before loading the
mesh into the renderer.

## xatlas license

This modifier uses xatlas by Jonathan Young, copyright 2018–2020, under the
MIT License. The upstream license and required copyright/permission notice are
included in [`lib/third_party/xatlas/LICENSE.txt`](../../../lib/third_party/xatlas/LICENSE.txt).
Redistributions containing xatlas must retain that notice.

## See also

- [`UvAtlasValidator`](UvAtlasValidator.md) — test whether a UV set is usable.
- [`UV map preview`](../ui/UvMapPreview.md) — inspect UV1 and UV2 in an editor.
- [Lightmap baking](../../lightmap_baking.md) — editor flow and safe regeneration.
