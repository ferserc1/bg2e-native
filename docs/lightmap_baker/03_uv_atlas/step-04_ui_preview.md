# Step 04: Add UI texture preview without exposing ImGui

## Scope

Add a bg2e::ui UV preview widget/window using existing TextureWidgets::drawImage and render::Texture wrapping where possible; add a small wrapper only if necessary. Keep imgui.h and ImGui types entirely inside lib/src/bg2e/ui, never in applications or public engine headers. Manage descriptor/image lifetime during resize and texture replacement.

## Acceptance and compile gate

Applications can embed a UV preview using only bg2e::ui public types. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_05_safe_reload.md](prestep_05_safe_reload.md) for the next step (Plan safe editor mesh replacement).
