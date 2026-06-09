# Step 013 — `cmd->draw()`

## Objective

Add `gpu::CommandBuffer::draw(vertexCount, instanceCount=1, firstVertex=0, firstInstance=0)` and
implement it on both backends, so a bound graphics pipeline can render `vertex_id`-generated geometry.

## Context

After step 012, a graphics pipeline can be bound inside an active render scope (Vulkan rendering
emitted; Metal encoder alive). `draw` records the actual draw command. The triangle uses
`draw(3)` with no vertex/index buffers.

## Expected prior state

- Step 012 done. `bindPipeline` works on both backends; example 04 unaffected.

## Files expected to review / modify

- Modify: `lib/include/bg2e/gpu/CommandBuffer.hpp` — add `virtual void draw(uint32_t vertexCount,
  uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0)` (non-pure,
  throwing default).
- Modify: `vk/CommandBuffer.{hpp,cpp}` — override `draw`.
- Modify: `metal/CommandBuffer.{hpp,cpp}` — override `draw`.

## Proposed design

### Vulkan
- `draw(...)`:
  - `flushPendingRendering()` (defensive; normally already flushed by `bindPipeline`).
  - Ensure dynamic viewport/scissor are set (if not done in `bindPipeline`).
  - `vkCmdDraw(_cmd, vertexCount, instanceCount, firstVertex, firstInstance)`.

### Metal
- `draw(...)`:
  - `ensureRenderEncoder()` (defensive).
  - Translate the bound pipeline's `PrimitiveTopology` → `MTL::PrimitiveType`
    (TriangleList→`Triangle`, TriangleStrip→`TriangleStrip`, LineList→`Line`, PointList→`Point`).
  - `_encoder->drawPrimitives(primType, firstVertex, vertexCount, instanceCount, firstInstance)`.
  - Requires a pipeline bound first (the encoder needs `setRenderPipelineState`); document that
    `draw` without a prior `bindPipeline` is undefined/error.

### Base
- Default `draw` throws "not implemented" (both backends override here).

## Required changes (no code in this plan)

- Header: new virtual. vk & metal overrides.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- With a valid bound pipeline and a `vertex_id` vertex shader, `draw(3)` records a triangle draw
  (visually validated by the example in step 015).
- Example 04 unaffected (it never calls `draw`).

## Risks / points to check

- Metal needs the topology from the currently bound `metal::GraphicsPipeline`; ensure step 012 stored it.
- Vulkan: missing dynamic viewport/scissor → validation error / nothing drawn. Confirm they are set.
- `instanceCount = 0` would draw nothing; keep default `1`.

## What must NOT be done in this step

- No indexed draw, no vertex buffers. No example code yet (steps 014–015). No dispatch (step 019).
