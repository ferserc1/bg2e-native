# Step 02: Build the UV-space surface pass

## Scope

Rasterize the complete target Drawable, including every submesh and its transform, into full-resolution UV2-space surface data (world position, normal, material/submesh identity and valid-texel mask). Keep UV1 material sampling intact. Define atlas overlap and missing-UV2 rejection. Ensure texture padding and empty texels are distinguishable. Add shaders and Vulkan resources needed for this pass.

## Acceptance and compile gate

A diagnostic result can identify covered texels and submeshes on a known UV2 fixture; code compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add a private `render::UvSurfacePass` and dedicated shaders; the pass is owned by each `LightmapBaker`. Its input is one standard `scene::Drawable`, including all submeshes and node/drawable/submesh transforms. It renders triangles with UV2 as clip-space XY: `2 * texCoord1 - 1`, with a documented Vulkan Y convention.
- Output full-resolution world position, world normal, submesh/material ID and a separate valid-texel mask. Use float formats for position/normal, integer identity and exact clearing for uncovered pixels. Apply inverse-transpose normal transformation; keep UV1 available for source material sampling. Reject triangles outside the atlas or with near-zero UV area.
- Use submesh-specific draw ranges. Record barriers from attachment writes to compute/ray-tracing reads. Do not reuse the camera G-buffer's depth test to cull overlapping UV islands: overlap is a validation failure.
- Add a private CPU usable-UV2 validator now: finite coordinates, [0,1] bounds, nondegenerate mapped triangles, valid index/submesh ranges and no positive-area overlap except shared boundaries. `createBaker` must invoke it before allocating targets. Phase 2 reuses this implementation; phase 3 exposes it as `geo::UvAtlasValidator`. The mere existence of `texCoord1` is insufficient because current importers may copy UV1 into it.
- A diagnostic readback/visual fixture must show the same coverage for two submeshes in one atlas and no light leaking through uncovered texels. The ordinary renderer remains unchanged at this step.

## API example

`UvSurfacePass::record(cmd, targetDrawable, targetNode.worldMatrix(), extent)` is private; no new public scene or material API is needed.

## Handoff

After this step, complete [prestep_03_scene_bindings.md](prestep_03_scene_bindings.md) for the next step (Reuse frame RT data safely).
