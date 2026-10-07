# Handoff for Add UI texture preview without exposing ImGui

Next implementation step: [step-04_ui_preview.md](step-04_ui_preview.md). Updated after completing step 03.

- Changed files and relevant API decisions: added `lib/include/bg2e/geo/UvAtlasValidator.hpp` + `lib/src/bg2e/geo/UvAtlasValidator.cpp` (public, promoted from the deleted private `lib/src/bg2e/render/UvAtlasValidation.{hpp,cpp}`); `LightmapBaker::validateTarget` now calls `geo::UvAtlasValidator::validate(*mesh, 1)`. Added `lib/include/bg2e/render/UvMapPreviewRenderer.hpp` + `lib/src/bg2e/render/UvMapPreviewRenderer.cpp` and shaders `shaders/src/glsl/uv_map_preview.{vert,frag}.glsl`; exported through `bg2e/render/all.hpp` and `bg2e/geo/all.hpp`. `UvAtlasValidator::validate(const geo::Mesh&, uint32_t uvSet)` returns `UvAtlasValidation { valid, error (UvAtlasError), message, submeshIndex, triangleIndex, vertexIndex, triangleCount, mappedArea, coverage }`. Note: the validator requires in-bounds, non-overlapping submesh ranges covering the whole index buffer, but (unlike the modifier) does not require them ordered. `UvMapPreviewRenderer(engine, resolution)` renders UV1/UV2 wireframes (barycentric in-shader edges — `fillModeNonSolid` is NOT enabled by the engine) plus a dim per-submesh coverage fill and a white [0,1] boundary; `render(mesh, uvSet)` is synchronous and leaves the image in `SHADER_READ_ONLY`; `texture()` exposes a `render::Texture*` usable with the existing TextureWidgets path.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile and test verification of step 03 remains outstanding (`test_04_uv2_atlas` gained validator fixtures; `test_02_uv_surface` gained a preview smoke test).
- Runtime/fixture evidence: fixtures written but not executed by the agent. `test_04` covers valid/overlapping/degenerate/out-of-range/non-finite/bad-submesh/unsupported-uvSet cases and validates xatlas modifier output; `test_02` renders both UV channels, checks covered/uncovered texels and the atlas boundary via readback, and exercises `setResolution`.
- Remaining limitations or regressions: the promoted validator is stricter than the deleted private one (full submesh coverage and no overlapping ranges); targets with gaps in the index buffer are now rejected. Barycentric wireframe width is uniform in barycentric space, not screen space. `render()` blocks on `immediateSubmit`.
- Resources, ownership and synchronization cautions for the next agent: `texture()` is invalidated by `setResolution()` (target recreated after `waitIdle`); the UI must re-fetch the texture and must not retain it or any Drawable across mesh regeneration or scene swaps. `render()` reads only the CPU `geo::Mesh` (works with `scene::Mesh`, which is a typedef of `geo::MeshPNUUT`); call `Drawable::reload()` separately after regeneration — the preview does not do it. Target format is `VK_FORMAT_R8G8B8A8_UNORM` with a linear sampler and `COLOR_ATTACHMENT|SAMPLED|TRANSFER_SRC` usage. No ImGui types appear in the new public headers.
- Exact next action: Implement the scope in step-04_ui_preview.md.

## Next-step instructions

Add a bg2e::ui UV preview widget/window using existing TextureWidgets::drawImage and render::Texture wrapping where possible; add a small wrapper only if necessary. Keep imgui.h and ImGui types entirely inside lib/src/bg2e/ui, never in applications or public engine headers. Manage descriptor/image lifetime during resize and texture replacement.

## Next-step acceptance gate

Applications can embed a UV preview using only bg2e::ui public types. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `bg2e::ui::UvMapPreview` as a `Window` or embeddable widget with `setMesh(std::shared_ptr<geo::Mesh>)`, `setUvSet(uint32_t)` and `draw()`. It owns or references a `render::UvMapPreviewRenderer` and uses `TextureWidgets::setDeferredTexture` plus `drawImage` to display its sampled image. Keep any Vulkan/ImGui descriptor setup in engine implementation files.
- Present UV1 and UV2 choices, current map validity and submesh/chart outlines. Show a clear empty state for no target mesh; refresh after regeneration or model/scene swaps without retaining a destroyed Drawable.
- `imgui.h`, `imgui_impl_vulkan.h`, `ImTextureID` and `ImVec*` are forbidden in public headers and application sources. The public interface uses engine types only. Clean up descriptors when the image is replaced or the widget closes.
