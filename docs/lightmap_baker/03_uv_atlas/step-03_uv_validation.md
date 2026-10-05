# Step 03: Validate and visualize atlas data

## Scope

Add reusable geo validation for missing/degenerate/overlapping UV2 and an engine utility that renders UV wireframes for either UV1 or UV2 to a sampled Vulkan image. Keep the atlas modifier independent of GPU and make preview work for existing and newly generated UV sets. Include an explicit valid-texel/coverage preview where useful.

## Acceptance and compile gate

Both UV channels can be rendered and inspected without altering the mesh. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_04_ui_preview.md](prestep_04_ui_preview.md) for the next step (Add UI texture preview without exposing ImGui).
