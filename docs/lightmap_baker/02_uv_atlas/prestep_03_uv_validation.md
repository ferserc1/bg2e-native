# Handoff for Validate and visualize atlas data

Next implementation step: [step-03_uv_validation.md](step-03_uv_validation.md). Updated after completing step 02.

- Changed files and relevant API decisions: added `lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp` (public, no xatlas include) and `lib/src/bg2e/geo/GenerateUv2AtlasModifier.cpp` (only file including `xatlas.h`); exported the header through `bg2e/geo/all.hpp`. Added `tests/test_04_uv2_atlas/` (CPU fixtures) and registered it in `tests/CMakeLists.txt`. The modifier is `geo::GenerateUv2AtlasModifier final : public geo::Modifier<geo::Mesh>` with aggregate `Uv2AtlasOptions { resolution = 512, paddingPixels = 4 }` and `Uv2AtlasResult { width, height, chartCount, utilization }`; `apply()` throws `std::runtime_error` on any failure and leaves the mesh unchanged. It requires submesh ranges to be ordered, non-overlapping and to cover the whole index buffer, and preserves them verbatim in the output. UV1/normal/tangent/position are copied struct-wise (bit-for-bit) via xatlas `xref`.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification of step 01 (deps.cmake xatlas integration) and step 02 remains outstanding. The lead's first step-02 build surfaced a latent error in `lib/include/bg2e/geo/modifiers.hpp` (`glm::compMax` used without `<glm/gtx/component_wise.hpp>`); fixed by adding that include to `modifiers.hpp`. Rebuild verification still pending.
- Runtime/fixture evidence: fixtures written in `tests/test_04_uv2_atlas/src/main.cpp` (two submeshes with shared vertices, UV2 replacement, invalid input, multi-atlas packing failure) but not compiled or executed by the agent; the project lead compiles and runs them.
- Remaining limitations or regressions: the packing-failure fixture assumes a 20x20 disconnected-quad grid cannot pack into 16x16 with 4 px padding (expects the multi-atlas rejection); adjust if a future xatlas version packs it. The internal chart-separation check is a texel-center rasterization at atlas resolution, not the full positive-area triangle-overlap test required from `geo::UvAtlasValidator`.
- Resources, ownership and synchronization cautions for the next agent: keep xatlas types out of public headers (the modifier cpp is the only xatlas consumer besides the deps.cmake private include). The step-03 validator should reuse the same submesh-range rules (ordered, contiguous, full coverage) so modifier output and validator input stay consistent. The private usable-UV2 check to promote lives in the integrated baker code from phase 1 (`render` namespace); replace it with the public validator and delete the duplicate.
- Exact next action: Implement the scope in step-03_uv_validation.md.

## Next-step instructions

Add reusable geo validation for missing/degenerate/overlapping UV2 and an engine utility that renders UV wireframes for either UV1 or UV2 to a sampled Vulkan image. Keep the atlas modifier independent of GPU and make preview work for existing and newly generated UV sets. Include an explicit valid-texel/coverage preview where useful.

## Next-step acceptance gate

Both UV channels can be rendered and inspected without altering the mesh. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Promote the private usable-UV2 check from integrated step 02 into `geo::UvAtlasValidator::validate(const geo::Mesh&, uint32_t uvSet)`, returning a diagnostic result with `valid`, error category, submesh/triangle IDs and coverage statistics. Check finite coordinates, [0,1] range, nonzero mapped area, valid triangle indices and submesh ranges, and positive-area overlap between different triangles. Shared edges/vertices are allowed. Update integrated baker target validation to use the public validator and delete the private duplicate. Phase 3 will reuse it for CLI skip behavior.
- Add `render::UvMapPreviewRenderer` that creates a sampled/color-attachment Vulkan image from either UV1 or UV2 and draws wireframe triangles plus atlas boundaries in UV space. It must not mutate the mesh or depend on `bg2e::ui`. Recreate its target safely on resolution changes and expose a `render::Texture`-compatible image handle.
- A copied UV1 fallback that overlaps must fail the usable-UV2 test; a copied UV1 that is already a valid unique atlas may pass. Do not invent source-UV2 provenance based on coordinate equality. Compare validation with xatlas output and known bad fixtures.
