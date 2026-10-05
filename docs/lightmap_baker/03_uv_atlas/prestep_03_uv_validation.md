# Handoff for Validate and visualize atlas data

Next implementation step: [step-03_uv_validation.md](step-03_uv_validation.md). Complete this file **after** finishing step 02; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-03_uv_validation.md.

## Next-step instructions

Add reusable geo validation for missing/degenerate/overlapping UV2 and an engine utility that renders UV wireframes for either UV1 or UV2 to a sampled Vulkan image. Keep the atlas modifier independent of GPU and make preview work for existing and newly generated UV sets. Include an explicit valid-texel/coverage preview where useful.

## Next-step acceptance gate

Both UV channels can be rendered and inspected without altering the mesh. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
