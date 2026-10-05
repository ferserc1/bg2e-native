# Step 02: Generate one atlas across submeshes

## Scope

Add a geo modifier for the standard Mesh. Feed all submeshes into one xatlas packing operation; preserve each submesh's triangle range and material association. Use xatlas source-vertex cross-references to duplicate vertices at seams while copying position, normal, tangent and UV1 exactly; assign only UV2. Reject multi-atlas output, invalid triangles and unrepresentable results without partial mesh mutation. Expose resolution, padding and atlas diagnostics.

## Acceptance and compile gate

CPU fixtures show exactly one nonoverlapping UV2 atlas and unchanged UV1 values after seam splits. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `geo::GenerateUv2AtlasModifier final : public geo::Modifier<geo::Mesh>` with `Uv2AtlasOptions { uint32_t resolution; uint32_t paddingPixels; }` and `Uv2AtlasResult { uint32_t width, height, chartCount; float utilization; }`. Expose `result() const` after `apply()`; failures throw and leave `Mesh` unchanged.
- Add each input submesh to **one** xatlas Atlas with its original vertex buffer and that submesh's index range. Generate charts and pack exactly one output atlas; set resolution and padding explicitly. Check `atlasCount == 1`, valid normalized coordinates and chart separation. Preserve submesh order, triangle count and material-to-submesh index.
- Build fresh arrays, mapping each xatlas output vertex through `xref` to the original source vertex. Copy position, normal, tangent and `texCoord0` exactly; set `texCoord1` from normalized xatlas UVs. Do not weld across charts. Commit both arrays and submesh ranges together only after validation succeeds.
- Test two submeshes, shared geometric vertices, existing UV2 replacement, invalid input and an atlas packing failure. UV1 must compare bit-for-bit against the corresponding original vertex. This modifier neither loads nor reloads GPU resources.

## API example

```cpp
geo::GenerateUv2AtlasModifier mod(&mesh, {.resolution = 512, .paddingPixels = 4});
mod.apply();
auto info = mod.result();
```

## Handoff

After this step, complete [prestep_03_uv_validation.md](prestep_03_uv_validation.md) for the next step (Validate and visualize atlas data).
