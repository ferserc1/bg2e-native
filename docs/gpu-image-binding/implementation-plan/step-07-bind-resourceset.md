# Step 07 — `CommandBuffer::bindResourceSet(...)`

## Title
Add `bindResourceSet` to the common command buffer and implement it on both
backends.

## Objective
Let a recorded command buffer bind a `ResourceSet` to a pipeline at a given set
index — `vkCmdBindDescriptorSets` on Vulkan; per-entry encoder binds on Metal.

## Context
The decision (see overview) is to expose **only** `bindResourceSet`, not
per-resource `bindImage` / `bindSampler`. The bind point and layout handle are
obtained from the pipeline passed in, mirroring the existing two `bindPipeline`
overloads.

## Expected previous state
- Steps 01–06 complete: `ResourceSet` is implemented on both backends;
  `PipelineLayout` exposes its handle / binding table.

## Files to review / modify
- Review: `lib/include/bg2e/gpu/CommandBuffer.hpp` (default-throw virtual
  pattern), `lib/src/bg2e/gpu/vk/CommandBuffer.cpp` (`bindPipeline`,
  `_boundLayoutHandle`), `lib/src/bg2e/gpu/metal/CommandBuffer.cpp`
  (`ensureRenderEncoder`, `_computeEncoder`, `_boundLayout`).
- Modify: `CommandBuffer.hpp` (base), `vk/CommandBuffer.hpp/.cpp`,
  `metal/CommandBuffer.hpp/.cpp`.

## Proposed design
Base class (`gpu::CommandBuffer`), default-throwing like `dispatch` / `draw`:

```cpp
virtual void bindResourceSet(gpu::GraphicsPipeline* pipeline,
                             uint32_t setIndex, gpu::ResourceSet* set)
{ throw std::runtime_error("bindResourceSet(GraphicsPipeline) not implemented"); }

virtual void bindResourceSet(gpu::ComputePipeline* pipeline,
                             uint32_t setIndex, gpu::ResourceSet* set)
{ throw std::runtime_error("bindResourceSet(ComputePipeline) not implemented"); }
```

(Forward-declare `ResourceSet` in `CommandBuffer.hpp`.)

### Vulkan
Both overloads `dynamic_cast` the pipeline to the vk type, read its layout handle
(`vkPipe->layoutHandle()`), `dynamic_cast` the set to `vk::ResourceSet`, and call:

```cpp
VkDescriptorSet ds = vkSet->handle();
vkCmdBindDescriptorSets(_cmd, bindPoint, layoutHandle, setIndex, 1, &ds, 0, nullptr);
```

with `bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS` or `..._COMPUTE`. The graphics
overload must run inside the active render scope (call `flushPendingRendering()`
first, consistent with `draw`); the compute overload requires an active compute
scope (`_computeActive`).

### Metal
Both overloads `dynamic_cast` the set to `metal::ResourceSet` and iterate its
entries, dispatching by `(stage, type)`:

- Compute scope (`_computeEncoder`):
  - `StorageImage` / `SampledImage` -> `_computeEncoder->setTexture(tex, index)`.
  - `Sampler` -> `_computeEncoder->setSamplerState(state, index)`.
- Render scope (`ensureRenderEncoder()` then `_encoder`):
  - `Vertex` stage texture -> `setVertexTexture`; sampler -> `setVertexSamplerState`.
  - `Fragment` stage texture -> `setFragmentTexture`; sampler ->
    `setFragmentSamplerState`.

`setIndex` is unused on Metal (no descriptor sets) beyond optional validation; the
entries already carry their resolved Metal indices from the `PipelineLayout`.

## Required changes
1. Add the two default-throwing virtuals + `ResourceSet` forward declaration to
   the base `CommandBuffer.hpp`.
2. Implement both overloads in `vk::CommandBuffer` and `metal::CommandBuffer`
   (and add the `#else` no-op stubs in the Metal file's non-mac branch).

## Compilation criteria
- Project compiles on all platforms. Existing examples never call
  `bindResourceSet`, so they are unaffected.

## Validation criteria
- Deferred to Steps 09 and 12 (first real binds).

## Risks / things to check
- Vulkan: ensure the graphics overload is called **after** `bindPipeline` and
  inside `beginRendering` so the descriptor set is bound for the subsequent
  `draw`. The compute overload must be called inside `beginCompute`/`endCompute`
  after `bindPipeline(computePipeline)`.
- Metal: `setSamplerState` / `setFragmentSamplerState` require an active encoder;
  the helper must `ensureRenderEncoder()` for the graphics overload and assert
  `_computeEncoder` for the compute overload.
- The two overloads must select the correct Vulkan bind point; a wrong bind point
  is a silent validation error.

## What NOT to do in this step
- Do not add `bindImage` / `bindSampler` (explicitly rejected in the design).
- Do not implement multi-set binding in a single call; one set per call is enough.
