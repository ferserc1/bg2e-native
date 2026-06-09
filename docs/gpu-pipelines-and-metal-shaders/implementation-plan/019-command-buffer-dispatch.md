# Step 019 — `cmd->dispatch()` + compute encoder handling

## Objective

Add `cmd->bindPipeline(gpu::ComputePipeline*)` (compute overload) and
`cmd->dispatch(groupCountX, groupCountY, groupCountZ)`, implemented on both backends. This lets a
compute pipeline be bound and dispatched. Compute runs **outside** the render scope.

## Context

Vulkan binds compute with `VK_PIPELINE_BIND_POINT_COMPUTE` and dispatches with `vkCmdDispatch` (no
render scope; must not be inside `vkCmdBeginRendering`). Metal needs a separate
`MTL::ComputeCommandEncoder` (distinct from the render encoder); it cannot be active while a render
encoder is open.

## Expected prior state

- Steps 017 & 018 done (compute pipelines exist). Steps 012/013 done (graphics bind/draw).

## Files expected to review / modify

- Modify: `lib/include/bg2e/gpu/CommandBuffer.hpp` — add an overload
  `virtual void bindPipeline(gpu::ComputePipeline*)` and
  `virtual void dispatch(uint32_t x, uint32_t y, uint32_t z)` (non-pure, throwing defaults).
- Modify: `vk/CommandBuffer.{hpp,cpp}` and `metal/CommandBuffer.{hpp,cpp}` — overrides.

## Proposed design

### Vulkan
- `bindPipeline(ComputePipeline*)`: ensure **not** inside a rendering scope (compute is outside
  rendering); `vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, handle())`; remember it.
- `dispatch(x,y,z)`: `vkCmdDispatch(_cmd, x, y, z)`.
- Note: barriers for compute↔graphics hazards are out of scope for the minimal validation; document.

### Metal
- Add a private `MTL::ComputeCommandEncoder* _computeEncoder` and `ensureComputeEncoder()`:
  - Must not be called while a render encoder is open (assert/throw or end the render encoder first).
  - `_computeEncoder = _cmd->computeCommandEncoder()` (retained).
- `bindPipeline(ComputePipeline*)`: `ensureComputeEncoder()`; `setComputePipelineState(state)`.
- `dispatch(x,y,z)`: translate to `dispatchThreadgroups(MTL::Size(x,y,z), threadsPerGroup)` where
  `threadsPerGroup` comes from the pipeline (e.g. `threadExecutionWidth` / kernel attribute). Document
  the threadgroup-size choice; for the first validation a fixed small size is acceptable.
- `end()`: end and release the compute encoder if open (mirroring render-encoder teardown).

### Base
- Default overloads throw "not implemented" (both backends override here).

## Required changes (no code in this plan)

- Header overloads; vk & metal overrides + Metal compute-encoder lifetime.

## Compilation criteria

- Build green on all platforms; overloaded `bindPipeline` resolves unambiguously
  (`GraphicsPipeline*` vs `ComputePipeline*`).

## Validation criteria

- A compute pipeline can be bound + dispatched in a command buffer without validation errors
  (exercised in step 020). Graphics path (triangle, example 04) unaffected.

## Risks / points to check

- **Overload ambiguity / nullptr:** distinct parameter types avoid ambiguity; ensure no implicit
  conversions cause confusion.
- **Metal encoder exclusivity:** a render encoder and a compute encoder cannot be open at once —
  dispatch must happen outside `beginRendering`/`endRendering`. Document ordering.
- **Vulkan:** dispatching inside an active dynamic-rendering scope is invalid — ensure compute is
  recorded before/after the render scope.

## What must NOT be done in this step

- No automatic barrier/synchronisation framework. No resource binding for compute (only the dispatch).
- No example yet (step 020).
