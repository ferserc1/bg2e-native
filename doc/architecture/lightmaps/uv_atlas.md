# UV atlas construction and validation

## Atlas addressing

UV1 (`VertexPNUUT::texCoord0`) retains ordinary material texture coordinates.
UV2 (`texCoord1`) supplies an independent parameterization for the bake. A
surface triangle occupies a region in the unit square; the baker later
rasterizes that triangle in UV2 while interpolating its world-space attributes.
Positive-area overlaps are ambiguous because one atlas texel cannot represent
two unrelated surface points. Seams instead duplicate vertices so that one 3D
point can have different UV2 coordinates on adjacent charts.

One atlas is created for the complete `geo::Mesh`, not one atlas per material.
Submeshes remain material/index ranges within that atlas. Two separate target
Drawables may reuse the full unit square because they have separate images.
Packing happens in mesh coordinates; placement and object transforms affect
world-space bake positions later, without repacking UV2.

Sources: [vertex layout](../../../lib/include/bg2e/geo/Vertex.hpp),
[modifier](../../../lib/src/bg2e/geo/GenerateUv2AtlasModifier.cpp).

## GenerateUv2AtlasModifier

The modifier derives from `Modifier<Mesh>` and holds a borrowed mesh pointer,
`Uv2AtlasOptions` and the last successful `Uv2AtlasResult`. `apply()` is CPU-only.
It validates nonempty triangle data, vertex indices and ordered, contiguous
submesh ranges covering the complete index buffer before calling xatlas.

The integration supplies one `xatlas::MeshDecl` with positions and normals at
`sizeof(VertexPNUUT)` stride and UInt32 indices. It supplies neither existing UV2
nor UV1 as chart coordinates and does not supply material boundaries as face
partitioning data. Charts can therefore be determined across material ranges.
The engine uses default `ChartOptions`: chart construction/parameterization is
owned by xatlas, rather than a custom engine unwrapping algorithm. The vendored
API defines `Generate` as `ComputeCharts` followed by `PackCharts`.

```cpp
// Extract from GenerateUv2AtlasModifier::apply().
xatlas::ChartOptions chartOptions;
xatlas::PackOptions packOptions;
packOptions.resolution = _options.resolution;
packOptions.padding = _options.paddingPixels;
packOptions.bilinear = true;
packOptions.blockAlign = false;
packOptions.createImage = false;
xatlas::Generate(atlas.get(), chartOptions, packOptions);
```

Source: [implementation](../../../lib/src/bg2e/geo/GenerateUv2AtlasModifier.cpp).
See [vendored option definitions](../../../lib/third_party/xatlas/src/xatlas.h)
and [chart/packing implementation](../../../lib/third_party/xatlas/src/xatlas.cpp).

With `texelsPerUnit` left at zero, xatlas estimates density to approximately
match the requested resolution. The returned width and height are authoritative;
they are not blindly assumed equal to the requested value. Bilinear padding
reserves space for texture filtering; engine chart padding adds separation.
The engine requires exactly one nonempty atlas and one output mesh and rejects
multi-atlas output, suggesting higher resolution or less padding.

## Remapping and commit

xatlas returns pixel-space chart coordinates, remapped indices and `xref`
source-vertex references. For each output vertex the engine copies the complete
source `VertexPNUUT`, then replaces only UV2:

$$
u = x_{atlas}/W_{atlas},\qquad v = y_{atlas}/H_{atlas}.
$$

Values beyond a `1e-4` tolerance outside the unit square are rejected; values
within that tolerance are clamped. Source position, normal, tangent and UV1 are
copied unchanged. Indices retain original face-slot order, so the existing
submesh first/count ranges and material associations remain valid despite seam
vertex duplication. Output triangle count must match input.

Zero-area 3D faces that xatlas cannot chart have negative `chartIndex`; their
new UV2 is `(0,0)`, preserving triangles without rasterized coverage. This does
not make an entirely unmapped mesh a usable bake target.

The modifier rasterizes output triangle centers at the actual atlas resolution
into a chart-owner grid. Signed edge cross-products determine inclusion; a cell
claimed by two different chart IDs produces a warning without rejecting the atlas. This check detects
cross-chart texel collisions at that resolution, not every continuous overlap
within a chart. The public validator provides the separate geometric test.

All replacement arrays and metrics are local until these checks succeed.
Only then are vertices, indices, submeshes and result committed. A thrown
`apply()` leaves the mesh untouched. This transaction ends at CPU commit:
a subsequent GPU reload failure is not rolled back by the modifier.

## UvAtlasValidator

`validate(mesh, uvSet)` reads UV1 for zero or UV2 for one. It validates every
vertex coordinate, including unused vertices, for finiteness and strict `[0,1]`
bounds. It validates index ranges, disjoint submesh coverage and referenced
indices. Range order is sorted for validation, unlike the modifier's stricter
ordered-input requirement. The implementation checks submesh count divisibility
by three and bounds; it does not explicitly check `firstIndex % 3`.

For a UV triangle, area is:

$$
A = \tfrac12 |(u_1-u_0)(v_2-v_0)-(v_1-v_0)(u_2-u_0)|.
$$

Triangles with area at most `1e-12` are excluded from overlap and coverage.
If all are excluded, validation returns `DegenerateTriangle`. On success,
`triangleCount` still counts all input triangles, including excluded ones.
`mappedArea` sums positive triangle areas.

The overlap broad phase sorts triangles by minimum U and scans candidates until
minimum U reaches the current triangle's maximum U; disjoint V bounds are
excluded. The narrow phase uses Sutherland–Hodgman clipping against the other
triangle's three half-planes, using the same zero boundary for classification and
intersection. Polygon area is accumulated relative to its first vertex to avoid
cancellation. Strict validation rejects intersections larger than `1e-12`;
the `UvAtlasOverlapPolicy::Warn` overload used by bakers records them in `warning`
and continues, preserving area and coverage statistics. Shared edges and vertices have zero intersection area and
are allowed. Sorting reduces candidate work but the worst case remains quadratic.

`coverage` is a separate estimate: triangle-center rasterization into a fixed
512×512 binary grid divided by its cell count. It is not xatlas utilization,
not the bake's actual coverage mask, and not an overlap test. Small triangles
can pass continuous validation yet cover no texel at a lower bake resolution.

`UvAtlasValidation` returns an error enum, message and applicable vertex,
triangle/submesh indices. Unrelated fields stay default. For overlap errors the
reported submesh is the lower index, while `triangleIndex` comes from the first
candidate in the U-sorted scan; it need not belong to that lower-index submesh.
Validation tests usability, not provenance: equal UV1 and UV2 coordinates are
accepted if valid.

```cpp
bg2e::geo::GenerateUv2AtlasModifier atlas(mesh.get(), {1024, 8});
atlas.apply();
auto check = bg2e::geo::UvAtlasValidator::validate(*mesh, 1,
    bg2e::geo::UvAtlasOverlapPolicy::Warn);
if (!check.valid)
{
    throw std::runtime_error(check.message);
}
if (!check.warning.empty())
{
    std::clog << "Warning: " << check.warning << '\n';
}
// GPU-load only after this CPU operation in a standalone workflow.
```

Adapted from [modifier](../../../lib/src/bg2e/geo/GenerateUv2AtlasModifier.cpp)
and [validator](../../../lib/src/bg2e/geo/UvAtlasValidator.cpp).
Continue with [surface projection](uv_surface_projection.md).
