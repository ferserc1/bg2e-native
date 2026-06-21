# gpu-ray-tracing-pipeline — Implementation Plan

## Problem Statement

`bg2e::gpu` has ray tracing acceleration structure support (`RayTracingMesh` for BLAS, `RayTracingScene` for TLAS) and hardware ray query support, but lacks a ray tracing **pipeline** abstraction. There is no common C++ API for dispatching ray tracing shaders (ray generation, miss, closest hit) across Vulkan and Metal backends. The Vulkan backend has all required extension function pointers loaded (`createRayTracingPipelines`, `cmdTraceRays`, `getRayTracingShaderGroupHandles`) but no pipeline class consumes them. The Metal backend has no ray tracing pipeline concept.

This plan adds `gpu::RayTracingPipeline` as the next ray tracing support block, enabling ray-traced effects (GI, reflections, soft shadows) through a common API that works on both Vulkan (real `VK_KHR_ray_tracing_pipeline` with SBT) and Metal (compute-based ray tracing via `.rgen.metallib`).

## Proposed Architecture

```
                        ┌─────────────────────────────────┐
                        │     Application (Example 13)     │
                        └──────────────┬──────────────────┘
                                       │
                        ┌──────────────▼──────────────────┐
                        │     gpu::RayTracingPipeline      │
                        │  ┌────────────────────────────┐  │
                        │  │ RayTracingPipelineDescription│ │
                        │  │  raygenShader               │  │
                        │  │  missShader (nullable Metal) │  │
                        │  │  closestHitShader (nullable) │  │
                        │  │  layout                     │  │
                        │  │  maxRecursionDepth          │  │
                        │  └────────────────────────────┘  │
                        └──────────────┬──────────────────┘
                                       │
               ┌───────────────────────┼───────────────────────┐
               │                       │                       │
    ┌──────────▼──────────┐ ┌─────────▼──────────┐ ┌─────────▼──────────┐
    │  Device::createRT   │ │  CommandBuffer::   │ │  ShaderLib::       │
    │  Pipeline()         │ │  traceRays()       │ │  rayGen/miss/CloseHit│
    └──────────┬──────────┘ └─────────┬──────────┘ └────────────────────┘
               │                       │
    ┌──────────▼──────────┐ ┌─────────▼──────────┐
    │   Backend impl      │ │  Backend impl      │
    └─────────┬───────────┘ └─────────┬──────────┘
              │                       │
   ┌──────────┴──────────┐  ┌────────┴──────────┐
   │  Vulkan             │  │  Metal             │
   │  VkPipeline         │  │  MTLComputePipeline│
   │  Internal SBT       │  │  State             │
   │  vkCmdTraceRaysKHR  │  │  dispatchThreadgrp │
   └─────────────────────┘  └───────────────────┘
```

## Files to Create/Modify

| # | File | Action | Description |
|---|------|--------|-------------|
| 1 | `lib/include/bg2e/gpu/Common.hpp` | Modify | Add `RayGeneration`, `Miss`, `ClosestHit` to `ShaderStage`; add `ShaderBindingTable` to `BufferUsage`; update validation functions |
| 2 | `lib/include/bg2e/gpu/ShaderLib.hpp` | Modify | Add `rayGeneration()`, `miss()`, `closestHit()` methods |
| 3 | `lib/src/bg2e/gpu/ShaderLib.cpp` | Modify | Implement RT shader loading with Metal null-safety |
| 4 | `lib/include/bg2e/gpu/RayTracingPipeline.hpp` | Create | Abstract `RayTracingPipeline` class + description struct |
| 5 | `lib/include/bg2e/gpu/Device.hpp` | Modify | Add `createRayTracingPipeline()` factory method |
| 6 | `lib/include/bg2e/gpu/CommandBuffer.hpp` | Modify | Add `bindPipeline(RT)`, `bindResourceSet(RT)`, `traceRays()` |
| 7 | `lib/include/bg2e/gpu/all.hpp` | Modify | Include `RayTracingPipeline.hpp` |
| 8 | `lib/include/bg2e/gpu/vk/RayTracingPipeline.hpp` | Create | Vulkan backend header with SBT regions |
| 9 | `lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp` | Create | Vulkan RT pipeline + SBT creation |
| 10 | `lib/include/bg2e/gpu/metal/RayTracingPipeline.hpp` | Create | Metal backend header (compute pipeline wrapper) |
| 11 | `lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp` | Create | Metal RT pipeline from rgen compute kernel |
| 12 | `lib/include/bg2e/gpu/vk/Device.hpp` | Modify | Add `createRayTracingPipeline()` override |
| 13 | `lib/src/bg2e/gpu/vk/Device.cpp` | Modify | Implement `createRayTracingPipeline()` |
| 14 | `lib/src/bg2e/gpu/vk/PipelineLayout.cpp` | Modify | Add RT stage flags to `shaderStageToVkFlags()` |
| 15 | `lib/include/bg2e/gpu/vk/CommandBuffer.hpp` | Modify | Add RT pipeline overrides + `_boundRTPipeline` member |
| 16 | `lib/src/bg2e/gpu/vk/CommandBuffer.cpp` | Modify | Implement `bindPipeline(RT)`, `bindResourceSet(RT)`, `traceRays()` |
| 17 | `lib/include/bg2e/gpu/metal/Device.hpp` | Modify | Add `createRayTracingPipeline()` override |
| 18 | `lib/src/bg2e/gpu/metal/Device.cpp` | Modify | Implement `createRayTracingPipeline()` |
| 19 | `lib/include/bg2e/gpu/metal/CommandBuffer.hpp` | Modify | Add RT pipeline overrides + `_boundRTPipeline` member |
| 20 | `lib/src/bg2e/gpu/metal/CommandBuffer.cpp` | Modify | Implement `bindPipeline(RT)`, `bindResourceSet(RT)`, `traceRays()` |
| 21 | `examples/gpu/13_ray_tracing_pipeline/CMakeLists.txt` | Create | Build configuration with shader compilation |
| 22 | `examples/gpu/13_ray_tracing_pipeline/src/main.cpp` | Create | Cornell box validation example |
| 23 | `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.glsl` | Create | Vulkan ray generation shader |
| 24 | `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rmiss.glsl` | Create | Vulkan miss shader |
| 25 | `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rchit.glsl` | Create | Vulkan closest hit shader |
| 26 | `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.metal` | Create | Metal compute-based RT kernel |
| 27 | `doc/api/gpu/RayTracingPipeline.md` | Create | New API documentation |
| 28 | `doc/api/gpu/Common.md` | Modify | Update ShaderStage, BufferUsage docs |
| 29 | `doc/api/gpu/ShaderLibraries.md` | Modify | Add RT shader methods |
| 30 | `doc/api/gpu/CommandBuffer.md` | Modify | Add RT dispatch docs |
| 31 | `doc/api/gpu/Device.md` | Modify | Add createRayTracingPipeline |
| 32 | `doc/api/gpu/reference.md` | Modify | Add RT pipeline link |
| 33 | `doc/api/gpu/quick_start.md` | Modify | Add RT recipe |
| 34 | `doc/api/gpu/index.md` | Modify | Add RT section |

## Step Links

- [Step 01: Extend ShaderStage and BufferUsage enums](step-01-extend-enums.md)
- [Step 02: Extend ShaderLib with RT shader loading](step-02-extend-shaderlib.md)
- [Step 03: Create RayTracingPipeline abstract class](step-03-rt-pipeline-abstract.md)
- [Step 04: Add traceRays to CommandBuffer](step-04-command-buffer-rt.md)
- [Step 05: Vulkan RayTracingPipeline backend](step-05-vulkan-rt-pipeline.md)
- [Step 06: Metal RayTracingPipeline backend](step-06-metal-rt-pipeline.md)
- [Step 07: Vulkan CommandBuffer RT integration](step-07-vulkan-cmd-rt.md)
- [Step 08: Metal CommandBuffer RT integration](step-08-metal-cmd-rt.md)
- [Step 09: Validation example (Cornell box)](step-09-validation-example.md)
- [Step 10: Documentation](step-10-documentation.md)

## Thread Safety Notes

- `RayTracingPipeline` objects are created once and reused across frames. No per-frame mutation.
- The internal SBT buffer in the Vulkan backend is immutable after creation.
- The Metal `MTLComputePipelineState` is immutable after creation.
- `CommandBuffer::traceRays()` is called from a single thread per command buffer, consistent with the existing command buffer threading model.
- No new synchronization primitives are needed.

## Key Design Decisions

1. **Metal ignores miss/closestHit shaders**: On Metal, `ShaderLib::miss()` and `ShaderLib::closestHit()` return `nullptr` when the file doesn't exist. `RayTracingPipelineDescription` accepts null pointers for these. Metal implementation discards them silently.

2. **SBT is internal to Vulkan backend**: The SBT buffer is created and managed inside `vk::RayTracingPipeline`. Application code never interacts with SBT directly. The single-raygen/miss/closestHit constraint makes automatic SBT creation trivial.

3. **Metal RT = compute dispatch**: Metal's `traceRays()` dispatches the rgen compute kernel over `(width, height, 1)` with a threadgroup size defined in the Metal shader (typically 8x8 or 16x16).

4. **PipelineLayout reuses existing binding model**: Ray tracing pipelines use the same `PipelineLayout` / `ResourceSet` / `ShaderBinding` system. The `ResourceType::AccelerationStructure` binding already works for TLAS. Storage images for output use `ResourceType::StorageImage`.

5. **Progressive accumulation**: The example uses a frame counter uniform and `mix()` in the shader for progressive refinement. The output image persists across frames and accumulates samples.
