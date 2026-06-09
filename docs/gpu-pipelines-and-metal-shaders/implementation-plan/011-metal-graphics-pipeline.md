# Step 011 — Metal `metal::GraphicsPipeline` (`MTL::RenderPipelineState`)

## Objective

Implement `metal::GraphicsPipeline`: build an `MTL::RenderPipelineState` from the description
(vertex+fragment `MTL::Function`, color/depth formats), and override
`metal::Device::createGraphicsPipeline`. Guarded by `BG2E_IS_MAC`.

## Context

Metal builds a render pipeline from an `MTL::RenderPipelineDescriptor` carrying the vertex/fragment
functions and the attachment pixel formats. These formats **must match** the render pass /
`SurfaceFrame` textures used at draw time. There is no vertex input layout (vertex_id only).

## Expected prior state

- Steps 005 (`metal::ShaderModule`), 008 (`metal::PipelineLayout`), 009 (base) done.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/metal/GraphicsPipeline.hpp` (`#if BG2E_IS_MAC` real + `#else` stub).
- Create: `lib/src/bg2e/gpu/metal/GraphicsPipeline.cpp`.
- Modify: `metal/Device.{hpp,cpp}` — override `createGraphicsPipeline`.
- Review: `metal/Format.cpp` (`PixelFormat → MTL::PixelFormat` via `toMetalPixelFormat`),
  `metal::ShaderModule::function()`, `metal/common.hpp`.

## Proposed design

- `dynamic_cast` description's `ShaderModule*` to `metal::ShaderModule`; throw on mismatch.
- Build `MTL::RenderPipelineDescriptor`:
  - `setVertexFunction(vsModule->function())`, `setFragmentFunction(fsModule->function())`.
  - `colorAttachments()->object(0)->setPixelFormat(toMetalPixelFormat(colorFormat))`.
  - If `depthFormat != Undefined`: `setDepthAttachmentPixelFormat(toMetalPixelFormat(depthFormat))`.
  - No `MTL::VertexDescriptor` (vertex_id only).
- `device->newRenderPipelineState(descriptor, &error)` → `MTL::RenderPipelineState*`.
- Store the pipeline state + topology (Metal sets primitive type at `drawPrimitives` time, so keep
  the `PrimitiveTopology` to translate in step 013).
- `cleanup()` releases the state + descriptor; non-Mac stub `isValid()` false.
- Optionally retain a reference to the `metal::PipelineLayout` (for push-constant index lookup later).

## Required changes (no code in this plan)

- New `metal/GraphicsPipeline.{hpp,cpp}` (auto-globbed). Override in `metal::Device`.

## Compilation criteria

- macOS real impl; Linux/Windows stub. Build green everywhere.

## Validation criteria

- On macOS, a description with valid functions + surface formats yields a valid
  `MTL::RenderPipelineState` (exercised by the example).

## Risks / points to check

- Attachment pixel formats must equal those of the `SurfaceFrame` color/depth textures, else
  `newRenderPipelineState` fails — read formats from the surface in the example.
- metal-cpp ownership: release descriptor and capture `NS::Error` on failure.
- If a depth attachment exists at draw time but the pipeline has `Undefined` depth format (or vice
  versa), Metal errors — keep formats consistent with the frame.

## What must NOT be done in this step

- No Vulkan changes. No `MTL::DepthStencilState` object yet (depth test config deferred; depth format
  on the pipeline is enough to be pass-compatible). No vertex descriptors.
