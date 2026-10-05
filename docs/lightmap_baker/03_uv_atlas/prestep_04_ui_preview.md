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
