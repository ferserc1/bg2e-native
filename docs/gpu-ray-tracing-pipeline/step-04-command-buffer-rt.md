# Step 04: Add traceRays to CommandBuffer

## Goal

Add ray tracing dispatch commands to the common `CommandBuffer` interface.

## Files to Modify

### `lib/include/bg2e/gpu/CommandBuffer.hpp`

1. Add forward declaration (after `class RayTracingScene;`):
```cpp
class RayTracingPipeline;
```

2. Add virtual methods (after `buildRayTracingScene()`):
```cpp
virtual void bindPipeline(gpu::RayTracingPipeline* pipeline)
{
    throw std::runtime_error("bindPipeline(RayTracingPipeline) not implemented");
}

virtual void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
{
    throw std::runtime_error("bindResourceSet(RayTracingPipeline) not implemented");
}

virtual void traceRays(uint32_t width, uint32_t height, uint32_t depth = 1)
{
    throw std::runtime_error("traceRays not implemented");
}
```

## Design Notes

- `bindPipeline(RayTracingPipeline*)` follows the same overload pattern as `bindPipeline(GraphicsPipeline*)` and `bindPipeline(ComputePipeline*)`.
- `bindResourceSet(RayTracingPipeline*, ...)` enables binding descriptor sets to the RT pipeline.
- `traceRays(width, height, depth)` dispatches ray generation across the image dimensions. The `depth` parameter defaults to 1 (2D dispatch is sufficient for most effects).
- The methods throw by default — backend implementations override them.
- No `beginRayTracing()`/`endRayTracing()` scope is needed. On Vulkan, `vkCmdTraceRaysKHR` is recorded directly. On Metal, the RT pipeline internally manages a compute encoder.

## Integration Points

- The Vulkan implementation (Step 07) calls `vkCmdBindPipeline` with `VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR` and `vkCmdTraceRaysKHR`.
- The Metal implementation (Step 08) manages a compute encoder internally and calls `dispatchThreadgroups`.
- Application code (Step 09) uses these methods in the render loop.

## Verification

Engine compiles. CommandBuffer declares the new methods with default throw implementations.
