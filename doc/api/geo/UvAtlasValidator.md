# UvAtlasValidator

**Header:** `<bg2e/geo/UvAtlasValidator.hpp>`  
**Namespace:** `bg2e::geo`

`UvAtlasValidator::validate` checks whether UV1 or UV2 forms a usable atlas.
It is a CPU operation and does not infer whether UV2 was authored or copied
from UV1.

```cpp
const geo::UvAtlasValidation validation =
    geo::UvAtlasValidator::validate(*mesh, 1); // 0 = UV1, 1 = UV2

if (!validation.valid)
{
    std::cerr << validation.message << '\n';
}
```

Only UV-set indices 0 (UV1 / `texCoord0`) and 1 (UV2 / `texCoord1`) are
supported. The validator rejects empty meshes, missing or invalid submesh
ranges, indices outside the vertex array, non-finite or out-of-range UV
coordinates, UV maps without any positive-area triangle, and overlaps with
positive area. Zero-area mapped triangles cover no texels and are ignored
by the overlap and coverage checks. Shared triangle edges and vertices are
allowed. All mesh vertex coordinates in the selected set are checked,
including vertices not referenced by an index.

## Result fields

| Field | Meaning |
|-------|---------|
| `valid` | `true` if validation succeeded. |
| `error` | A `UvAtlasError` code; `None` indicates success. |
| `message` | Human-readable success/error context. |
| `submeshIndex` | Submesh associated with an error, where available; overlap errors report the lower submesh index. |
| `triangleIndex` | Triangle index within its submesh, where available. |
| `vertexIndex` | Vertex associated with a coordinate error, where available. |
| `triangleCount` | Number of validated triangles on success. |
| `mappedArea` | Sum of absolute UV triangle areas; overlapping layouts can make it exceed 1. |
| `coverage` | Approximate fraction of the unit square covered by the UV map. |

`UvAtlasError` values are `None`, `UnsupportedUvSet`, `EmptyMesh`,
`MissingSubmeshes`, `InvalidSubmeshRange`, `OverlappingSubmeshRanges`,
`IncompleteSubmeshCoverage`, `IndexOutOfRange`, `NonFiniteCoordinate`,
`CoordinateOutOfRange`, `DegenerateTriangle`, and `OverlappingTriangles`.
Fields unrelated to a particular failure retain their default values.

An importer may populate UV2 by copying UV1 when the source has no second
channel. If those coordinates satisfy the atlas rules, validation succeeds:
coordinate equality is not treated as evidence of a missing atlas.

## See also

- [`GenerateUv2AtlasModifier`](GenerateUv2AtlasModifier.md) — create a UV2 atlas.
- [`UV map preview`](../ui/UvMapPreview.md) — view the selected UV set and validation report.
