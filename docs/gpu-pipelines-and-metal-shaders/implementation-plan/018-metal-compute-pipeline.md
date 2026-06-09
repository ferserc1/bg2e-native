# Step 018 — Metal `metal::ComputePipeline` (`MTL::ComputePipelineState`)

## Objective

Implement `metal::ComputePipeline`: build an `MTL::ComputePipelineState` from a kernel `MTL::Function`,
and override `metal::Device::createComputePipeline`. Guarded by `BG2E_IS_MAC`.

## Context

`gpu::ComputePipeline` + factory hook exist (016). Reuse `metal::ShaderModule` (005). Metal builds a
compute pipeline directly from a kernel function (no layout object needed; the logical
`metal::PipelineLayout` is kept for binding metadata).

## Expected prior state

- Steps 005, 008, 016 done.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/metal/ComputePipeline.hpp` (`#if BG2E_IS_MAC` + `#else` stub),
  `lib/src/bg2e/gpu/metal/ComputePipeline.cpp`.
- Modify: `metal/Device.{hpp,cpp}` — override `createComputePipeline`.

## Proposed design

- `dynamic_cast` the description's shader to `metal::ShaderModule`; throw on mismatch.
- `device->newComputePipelineState(fn->function(), &error)` → `MTL::ComputePipelineState*`.
- Store the state (and optionally `maxTotalThreadsPerThreadgroup` / `threadExecutionWidth` for the
  example to size dispatches). `cleanup()` releases the state; non-Mac stub `isValid()` false.

## Required changes (no code in this plan)

- New `metal/ComputePipeline.{hpp,cpp}` (auto-globbed). Override in `metal::Device`.

## Compilation criteria

- macOS real impl; Linux/Windows stub. Build green everywhere.

## Validation criteria

- On macOS, a valid kernel function yields a valid `MTL::ComputePipelineState` (exercised in step 020).

## Risks / points to check

- metal-cpp ownership: release state in cleanup; capture `NS::Error` on failure.
- Threadgroup sizing differs from Vulkan; expose pipeline limits for the example.

## What must NOT be done in this step

- No Vulkan changes. No dispatch (019).
