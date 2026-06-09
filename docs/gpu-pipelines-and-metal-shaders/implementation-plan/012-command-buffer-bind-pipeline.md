# Step 012 — `cmd->bindPipeline()` + rendering-scope materialisation

## Objective

Add `gpu::CommandBuffer::bindPipeline(gpu::GraphicsPipeline*)` and implement it on both backends,
including the **rendering-scope materialisation** required so subsequent draws record inside an
active render scope. The clear loop (example 04) must keep working unchanged.

## Context (design-critical — see summary §2.3)

- **Vulkan** defers `vkCmdBeginRendering` until `flushPendingRendering()` (called at `end`/`endRendering`).
  A bound pipeline / draw must run *inside* the render scope, so `bindPipeline` must call
  `flushPendingRendering()` first.
- **Metal** currently creates the `MTL::RenderCommandEncoder` *inside* `endRendering()`. To record
  `setRenderPipelineState` + draws, the encoder must exist *before* the first bind/draw and stay
  alive until `endRendering()`.

## Expected prior state

- Steps 010 & 011 done (graphics pipelines exist on both backends). Example 04 still runs.

## Files expected to review / modify

- Modify: `lib/include/bg2e/gpu/CommandBuffer.hpp` — add `virtual void bindPipeline(gpu::GraphicsPipeline*)`
  as a non-pure virtual with a default that throws (compile-safe; keeps existing subclasses building
  until overridden — though both are overridden in this same step).
- Modify: `vk/CommandBuffer.{hpp,cpp}` — override `bindPipeline`; refactor `flushPendingRendering`
  so it can be triggered eagerly.
- Modify: `metal/CommandBuffer.{hpp,cpp}` — override `bindPipeline`; refactor encoder lifetime.
- Review: `vk::GraphicsPipeline::handle()/bindPoint()`, `metal::GraphicsPipeline` state accessor.

## Proposed design

### Vulkan
- `bindPipeline(p)`:
  - `flushPendingRendering()` (emit `vkCmdBeginRendering` now if pending).
  - Set the dynamic viewport + scissor from the render frame size (since the pipeline uses dynamic
    viewport/scissor) — either here or in `draw` (step 013). Recommend doing it here so the state is
    valid for any subsequent draw; document the choice.
  - `vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->handle())`.
  - Keep a `vk::GraphicsPipeline*` reference for `draw`.

### Metal
- Introduce a private `ensureRenderEncoder()`:
  - If `_encoder` is null and `_passDesc` exists, create `_encoder = _cmd->renderCommandEncoder(_passDesc)`
    (retained), and **do not** end it yet.
- `beginRendering`: build `_passDesc` as today (do **not** create the encoder).
- `bindPipeline(p)`: `ensureRenderEncoder()`, then `_encoder->setRenderPipelineState(state)`; store
  the `metal::GraphicsPipeline*` (for topology at draw time).
- `endRendering`: if `_encoder` was never created (clear-only frame, ex. 04), create it now (so the
  clear `LoadAction` still resolves), then `endEncoding()` + release; release `_passDesc`. This keeps
  example 04 behaviour identical.

### Base
- Default `bindPipeline` throws "not implemented" (both backends override it here).

## Required changes (no code in this plan)

- Header: new virtual on `gpu::CommandBuffer`.
- vk & metal: overrides + scope materialisation refactor as above.

## Compilation criteria

- Build green on all platforms.

## Validation criteria

- Example 04 (clear loop) still clears correctly on both backends (no regression from the Metal
  encoder-lifetime refactor).
- `bindPipeline` records without validation errors when called between `beginRendering` and
  `endRendering`.

## Risks / points to check

- **Regression risk on Metal 04:** the refactor must preserve "clear-only" frames. Verify the clear
  still happens when no pipeline is ever bound.
- Vulkan dynamic viewport/scissor must be set before any draw or validation will complain.
- Ensure `bindPipeline` outside an active rendering scope fails clearly (or is a documented no-op).

## What must NOT be done in this step

- No `draw` yet (step 013). No compute-pipeline binding (step 019 handles dispatch separately).
- Do not change `clearColor`/`clearDepth` semantics.
