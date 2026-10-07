# Step 03: Validate and visualize atlas data

## Scope

Add reusable geo validation for missing/degenerate/overlapping UV2 and an engine utility that renders UV wireframes for either UV1 or UV2 to a sampled Vulkan image. Keep the atlas modifier independent of GPU and make preview work for existing and newly generated UV sets. Include an explicit valid-texel/coverage preview where useful.

## Acceptance and compile gate

Both UV channels can be rendered and inspected without altering the mesh. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Promote the private usable-UV2 check from integrated step 02 into `geo::UvAtlasValidator::validate(const geo::Mesh&, uint32_t uvSet)`, returning a diagnostic result with `valid`, error category, submesh/triangle IDs and coverage statistics. Check finite coordinates, [0,1] range, nonzero mapped area, valid triangle indices and submesh ranges, and positive-area overlap between different triangles. Shared edges/vertices are allowed. Update integrated baker target validation to use the public validator and delete the private duplicate. Phase 3 will reuse it for CLI skip behavior.
- Add `render::UvMapPreviewRenderer` that creates a sampled/color-attachment Vulkan image from either UV1 or UV2 and draws wireframe triangles plus atlas boundaries in UV space. It must not mutate the mesh or depend on `bg2e::ui`. Recreate its target safely on resolution changes and expose a `render::Texture`-compatible image handle.
- A copied UV1 fallback that overlaps must fail the usable-UV2 test; a copied UV1 that is already a valid unique atlas may pass. Do not invent source-UV2 provenance based on coordinate equality. Compare validation with xatlas output and known bad fixtures.

## API example

```cpp
auto check = geo::UvAtlasValidator::validate(mesh, 1);
preview.render(mesh, 0); // inspect UV1
preview.render(mesh, 1); // inspect UV2
```

## Handoff

After this step, complete [prestep_04_ui_preview.md](prestep_04_ui_preview.md) for the next step (Add UI texture preview without exposing ImGui).
