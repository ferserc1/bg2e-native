# Step 007 — Vulkan `vk::PipelineLayout` (`VkPipelineLayout`)

## Objective

Implement `vk::PipelineLayout` creating a `VkPipelineLayout` from `PipelineLayoutDescription`
(push-constant ranges only for now), and override `vk::Device::createPipelineLayout`.

## Context

`gpu::PipelineLayout` + factory hook exist (006). `vk::Device::handle()` → `VkDevice`. The first
triangle uses an empty layout (no push constants); step 021 adds a push-constant range.

## Expected prior state

- Step 006 done. `createPipelineLayout` throws by default.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/vk/PipelineLayout.hpp` — `vk::PipelineLayout : gpu::PipelineLayout`.
- Create: `lib/src/bg2e/gpu/vk/PipelineLayout.cpp`.
- Modify: `vk/Device.{hpp,cpp}` — override `createPipelineLayout`.
- Review: `vk/Info.hpp` for struct-init helpers.

## Proposed design

- `vk::PipelineLayout` holds `VkDevice` + `VkPipelineLayout`.
- Construction: translate each `PushConstantRange` → `VkPushConstantRange`
  (`stageFlags` from `ShaderStage`, `offset`, `size`); zero descriptor set layouts for now;
  `vkCreatePipelineLayout`.
- Map `ShaderStage` → `VkShaderStageFlags` (Vertex→`VK_SHADER_STAGE_VERTEX_BIT`,
  Fragment→`..._FRAGMENT_BIT`, Compute→`..._COMPUTE_BIT`). Put this mapping in a small local helper
  (reusable by the pipeline steps).
- `cleanup()` → `vkDestroyPipelineLayout`; `handle()` → `VkPipelineLayout`.
- Override returns `std::make_unique<vk::PipelineLayout>(...)`.

## Required changes (no code in this plan)

- New `vk/PipelineLayout.{hpp,cpp}` (auto-globbed). Override in `vk::Device`.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- An empty-description layout produces a valid `VkPipelineLayout` (exercised by the example later).

## Risks / points to check

- Empty push-constant list must still produce a valid (no-push-constant) layout.
- Keep the `ShaderStage → VkShaderStageFlags` helper where the pipeline steps can reuse it.

## What must NOT be done in this step

- No Metal implementation (008). No descriptor set layouts. No pipeline creation.
