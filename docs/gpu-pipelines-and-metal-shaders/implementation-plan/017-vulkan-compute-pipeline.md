# Step 017 — Vulkan `vk::ComputePipeline` (`VkPipeline` compute)

## Objective

Implement `vk::ComputePipeline`: build a compute `VkPipeline` from a compute `VkShaderModule` +
`VkPipelineLayout`, and override `vk::Device::createComputePipeline`.

## Context

`gpu::ComputePipeline` + factory hook exist (016). Reuse `vk::ShaderModule` (004) and
`vk::PipelineLayout` (007). Bind point is `VK_PIPELINE_BIND_POINT_COMPUTE`.

## Expected prior state

- Steps 004, 007, 016 done.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/vk/ComputePipeline.hpp`, `lib/src/bg2e/gpu/vk/ComputePipeline.cpp`.
- Modify: `vk/Device.{hpp,cpp}` — override `createComputePipeline`.

## Proposed design

- `dynamic_cast` the description's shader/layout to `vk::` types; throw on mismatch.
- Build `VkPipelineShaderStageCreateInfo` (stage `VK_SHADER_STAGE_COMPUTE_BIT`, module, `pName`).
- `VkComputePipelineCreateInfo` with the stage + `vk::PipelineLayout::handle()`.
- `vkCreateComputePipelines` → store `VkPipeline` + bind point `VK_PIPELINE_BIND_POINT_COMPUTE`.
- `handle()`/`bindPoint()`/`cleanup()` (`vkDestroyPipeline`).

## Required changes (no code in this plan)

- New `vk/ComputePipeline.{hpp,cpp}` (auto-globbed). Override in `vk::Device`.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- A valid compute SPIR-V + layout yields a valid `VkPipeline` (exercised in step 020).

## Risks / points to check

- Confirm the compute SPIR-V's local workgroup size is known to the example for dispatch sizing.

## What must NOT be done in this step

- No Metal implementation (018). No dispatch (019).
