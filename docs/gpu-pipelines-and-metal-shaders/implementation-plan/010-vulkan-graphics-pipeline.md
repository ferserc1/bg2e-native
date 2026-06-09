# Step 010 — Vulkan `vk::GraphicsPipeline` (`VkPipeline` graphics)

## Objective

Implement `vk::GraphicsPipeline`: build a graphics `VkPipeline` for dynamic rendering from the
description (vertex+fragment `VkShaderModule`, `VkPipelineLayout`, color/depth formats, topology),
and override `vk::Device::createGraphicsPipeline`.

## Context

The engine uses **dynamic rendering** (no `VkRenderPass`). Pipeline creation must use
`VkPipelineRenderingCreateInfo` with the color/depth formats from the description. No vertex input
(empty `VkPipelineVertexInputStateCreateInfo`) since vertices come from `gl_VertexIndex`.

## Expected prior state

- Steps 004 (`vk::ShaderModule`), 007 (`vk::PipelineLayout`), 009 (base) done.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/vk/GraphicsPipeline.hpp` — `vk::GraphicsPipeline : gpu::GraphicsPipeline`.
- Create: `lib/src/bg2e/gpu/vk/GraphicsPipeline.cpp`.
- Modify: `vk/Device.{hpp,cpp}` — override `createGraphicsPipeline`.
- Review: `vk/Format.cpp` (`PixelFormat → VkFormat`), `vk/Info.hpp`, `vk/extensions.hpp`,
  `vk::ShaderModule::handle()/entryPoint()`, `vk::PipelineLayout::handle()`.

## Proposed design

- `dynamic_cast` the description's `ShaderModule*`/`PipelineLayout*` to `vk::` types; throw on mismatch
  (same defensive pattern as `vk::CommandBuffer::transition`).
- Build `VkPipelineShaderStageCreateInfo[2]` from the vertex/fragment modules (stage + module +
  `pName = entryPoint`).
- Fixed-function state (documented defaults):
  - Vertex input: empty.
  - Input assembly: from `topology`.
  - Viewport/scissor: dynamic (`VK_DYNAMIC_STATE_VIEWPORT`, `..._SCISSOR`) → set at draw time
    (step 013 sets them from the frame size).
  - Rasterization: fill, cull none, CCW.
  - Multisample: 1 sample.
  - Color blend: 1 attachment, blend disabled, RGBA write mask.
  - Depth-stencil: enabled only if `depthFormat != Undefined` (test+write, `LESS_OR_EQUAL`).
- `VkPipelineRenderingCreateInfo`: `colorAttachmentFormats = { toVkFormat(colorFormat) }`,
  `depthAttachmentFormat = depthFormat==Undefined ? VK_FORMAT_UNDEFINED : toVkFormat(depthFormat)`.
- `vkCreateGraphicsPipelines`; store `VkPipeline` + bind point `VK_PIPELINE_BIND_POINT_GRAPHICS`.
- `handle()` → `VkPipeline`; `bindPoint()` → `VkPipelineBindPoint`; `cleanup()` → `vkDestroyPipeline`.

## Required changes (no code in this plan)

- New `vk/GraphicsPipeline.{hpp,cpp}` (auto-globbed). Override in `vk::Device`.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- A description with valid vertex/fragment SPIR-V + a surface color format produces a valid
  `VkPipeline` (exercised by the example).

## Risks / points to check

- Dynamic viewport/scissor must actually be set before `vkCmdDraw` (handled in step 013); otherwise
  validation errors. Note this dependency explicitly.
- Color format must match the surface's color image format, else dynamic-rendering validation fails.
- Reuse the `ShaderStage → VkShaderStageFlags` helper from step 007.

## What must NOT be done in this step

- No Metal implementation (011). No vertex buffers. No binding (steps 012–013).
