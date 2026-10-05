# Handoff for Add UI texture preview without exposing ImGui

Next implementation step: [step-04_ui_preview.md](step-04_ui_preview.md). Complete this file **after** finishing step 03; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
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
