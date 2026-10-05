# Handoff for Add accumulation and CPU/GPU results

Next implementation step: [step-06_accumulation_output.md](step-06_accumulation_output.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-06_accumulation_output.md.

## Next-step instructions

Add per-baker progressive accumulation over configurable frames; reset history after scene, target or parameter invalidation. Use full-resolution intermediate images and filtering. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use a native-resolution path when it cannot apply without camera reprojection.

## Next-step acceptance gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
