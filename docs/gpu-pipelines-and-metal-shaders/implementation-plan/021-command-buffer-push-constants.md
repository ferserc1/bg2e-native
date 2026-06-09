# Step 021 — `cmd->pushConstants()` foundation + triangle color

## Objective

Add `cmd->pushConstants(...)` to the command buffer (the first resource-binding primitive) and use it
to drive the triangle color, implemented on both backends. This establishes the small-constants path
that later resource bindings will extend.

## Context

This is item 2 of the resource-binding roadmap (after "pipeline without complex bindings"). Mapping:
Vulkan `vkCmdPushConstants`; Metal `setVertexBytes` / `setFragmentBytes` / `setBytes`. The Vulkan
pipeline layout must declare a push-constant range (step 007 already supports it); the Metal logical
layout supplies the buffer index (step 008). Size stays within the Vulkan-compatible ceiling (≤128 B).

## Expected prior state

- Steps 007/008 (layouts support push-constant ranges), 010/011 (graphics pipelines), 012/013
  (bind/draw), 015 (triangle renders). The triangle currently has a hardcoded color.

## Files expected to review / modify

- Modify: `lib/include/bg2e/gpu/CommandBuffer.hpp` — add
  `virtual void pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data)`
  (non-pure, throwing default). (Stage may be a single enum now; mask later.)
- Modify: `vk/CommandBuffer.{hpp,cpp}` and `metal/CommandBuffer.{hpp,cpp}` — overrides.
- Modify (example): `examples/gpu/05_simple_triangle/src/main.cpp` + the triangle shaders to read a
  push-constant color.
- Review: how the currently bound pipeline/layout is tracked by the command buffer (needed by both
  backends to resolve the Vulkan `VkPipelineLayout` and the Metal buffer index).

## Proposed design

### Command buffer needs the bound layout
- Track the bound graphics pipeline (already kept from step 012) and reach its `PipelineLayout`:
  - Vulkan: need the `VkPipelineLayout` for `vkCmdPushConstants` → expose it from the bound
    `vk::GraphicsPipeline` (store the layout handle in the pipeline, or keep the bound
    `vk::PipelineLayout*` in the command buffer). Recommend the pipeline holds its layout handle.
  - Metal: need the push-constant **buffer index** from `metal::PipelineLayout` (step 008 convention).

### Vulkan
- `pushConstants(stage, offset, size, data)`:
  - `vkCmdPushConstants(_cmd, layoutHandle, toVkStageFlags(stage), offset, size, data)`.

### Metal
- `pushConstants(stage, offset, size, data)` (`ensureRenderEncoder()` first):
  - Vertex → `_encoder->setVertexBytes(data, size, bufferIndex)`.
  - Fragment → `_encoder->setFragmentBytes(data, size, bufferIndex)`.
  - (Compute path via the compute encoder `setBytes` is analogous; add if needed for compute use.)
  - `bufferIndex` from the bound `metal::PipelineLayout` push-constant convention.

### Example + shaders
- Add a push-constant block (e.g. `vec4 color` or `float t`) to the triangle shaders:
  - GLSL: `layout(push_constant) uniform Push { vec4 color; } pc;` used by the fragment shader.
  - MSL: a `constant Push& pc [[buffer(N)]]` parameter where `N` matches the layout's buffer index.
- Create the layout with a matching `PushConstantRange` (size of the push block, Fragment stage).
- Each frame: `cmd->pushConstants(ShaderStage::Fragment, 0, sizeof(push), &push)` between
  `bindPipeline` and `draw`.

## Required changes (no engine code in this plan; design only)

- Header virtual + vk/metal overrides + bound-layout tracking.
- Example layout now carries a push-constant range; shaders read it; per-frame push call.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- Changing the pushed color value changes the triangle color on **both** backends.
- No validation errors (Vulkan range must match the layout; Metal buffer index must match the MSL
  `[[buffer(N)]]`).

## Risks / points to check

- **Index/range agreement:** Vulkan push-constant range (offset/size/stage) must match the layout;
  Metal `[[buffer(N)]]` must match the layout's reserved index (step 008). Keep one documented source
  of truth.
- **Size ceiling:** keep the push block ≤128 bytes for Vulkan portability.
- **Stage mask vs single stage:** if a constant is used in both vertex and fragment, a single-stage
  enum is insufficient — note the future upgrade to a stage mask (Vulkan) / both `setVertexBytes` +
  `setFragmentBytes` (Metal).
- Metal buffer-index collision with future vertex buffers — the layout convention must avoid index 0
  once vertex buffers exist (documented in step 008).

## What must NOT be done in this step

- No uniform buffers, textures, samplers, storage buffers, or resource sets (later phases).
- Do not generalise to argument buffers; this is direct small-constant binding only.
