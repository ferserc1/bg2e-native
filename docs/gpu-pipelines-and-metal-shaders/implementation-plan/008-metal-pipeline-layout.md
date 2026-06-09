# Step 008 — Metal `metal::PipelineLayout` (logical metadata)

## Objective

Implement `metal::PipelineLayout` as a **logical** object: Metal has no pipeline-layout object, so it
stores the `PipelineLayoutDescription` (push-constant ranges → buffer-index mapping) for later use by
`pushConstants` / resource binding. Override `metal::Device::createPipelineLayout`.

## Context

In Metal, push/small constants are set via `setVertexBytes/setFragmentBytes/setBytes` at a chosen
**buffer index**. The layout must record which buffer index a push-constant range maps to, so the
command-buffer `pushConstants` (step 021) can pick the right index. No native MTL object is created.

## Expected prior state

- Step 006 done; 007 independent. `createPipelineLayout` throws by default.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/metal/PipelineLayout.hpp` — `metal::PipelineLayout : gpu::PipelineLayout`
  (`#if BG2E_IS_MAC` real, `#else` stub).
- Create: `lib/src/bg2e/gpu/metal/PipelineLayout.cpp` (real + `#else` stubs).
- Modify: `metal/Device.{hpp,cpp}` — override `createPipelineLayout`.

## Proposed design

- `metal::PipelineLayout` stores a copy of `PipelineLayoutDescription` plus a derived mapping
  `pushConstantBufferIndex` (e.g. reserve buffer index `0` for the vertex-id pipeline's push block,
  or a documented high index to avoid clashing with future vertex buffers).
- `isValid()` → true once constructed; `cleanup()` → no-op (no native resource).
- Provide accessors the command buffer will need later, e.g. `pushConstantBufferIndex(stage)`.
- Non-Mac stub: compiles, `isValid()` false.

Decision point — **reserved buffer index for push constants:** document a convention (e.g. index 0
for now, since the triangle has no vertex buffers). Revisit when vertex/uniform buffers are added so
indices don't collide; this is exactly the kind of metadata the logical layout exists to hold.

## Required changes (no code in this plan)

- New `metal/PipelineLayout.{hpp,cpp}` (auto-globbed). Override in `metal::Device`.

## Compilation criteria

- macOS real impl; Linux/Windows stubs. Build green everywhere.

## Validation criteria

- `metalDevice->createPipelineLayout({})` returns a valid logical layout exposing the push-constant
  index convention.

## Risks / points to check

- Keep the buffer-index convention documented in one place; both `metal::GraphicsPipeline`
  (vertex-buffer indices) and `pushConstants` must agree.

## What must NOT be done in this step

- No `MTL` object creation. No Vulkan changes. No argument buffers.
