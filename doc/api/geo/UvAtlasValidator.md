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
| `message` | Failure diagnostic; empty on success. |
| `submeshIndex` | Submesh associated with an error, where available; overlap errors report the lower submesh index. |
| `triangleIndex` | Triangle index for the reported candidate, where available; for overlaps it comes from the first U-sorted candidate and may not correspond to the lower `submeshIndex`. |
| `vertexIndex` | Vertex associated with a coordinate error, where available. |
| `triangleCount` | Total input triangle count on success, including ignored zero-area triangles. |
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

## Signature and metric details

```cpp
static UvAtlasValidation validate(const Mesh& mesh, uint32_t uvSet);
```

Invalid layouts are returned as reports rather than validation exceptions.
Positive-area/overlap thresholds are `1e-12` in normalized UV area. `coverage`
is estimated on a fixed 512×512 center-sampling grid, independently of bake
resolution. Metrics remain default on an overlap failure rather than reporting
partial area. Range counts must be divisible by three; the implementation does
not explicitly check `firstIndex % 3`. The modifier has a stricter ordered,
contiguous-range input rule than this validator.

Sources: [header](../../../lib/include/bg2e/geo/UvAtlasValidator.hpp),
[implementation](../../../lib/src/bg2e/geo/UvAtlasValidator.cpp).
See [validation algorithm](../../architecture/lightmaps/uv_atlas.md).
