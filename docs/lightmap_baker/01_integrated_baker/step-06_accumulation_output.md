# Step 06: Add accumulation and CPU/GPU results

## Scope

Add per-baker progressive accumulation over configurable frames; reset history after scene, target or parameter invalidation. Use full-resolution intermediate images and filtering. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use a native-resolution path when it cannot apply without camera reprojection.

## Acceptance and compile gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_07_application_integration.md](prestep_07_application_integration.md) for the next step (Integrate into model_edit and bg2e_composer).
