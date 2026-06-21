# Step 07: Vulkan CommandBuffer RT Integration

## Goal

Implement `bindPipeline(RayTracingPipeline*)`, `bindResourceSet(RayTracingPipeline*, ...)`, and `traceRays()` in the Vulkan command buffer.

## Files to Modify

### `lib/include/bg2e/gpu/vk/CommandBuffer.hpp`

Add forward declaration and overrides:

```cpp
// After existing forward declarations
class RayTracingPipeline;

// In CommandBuffer class, after existing methods
void bindPipeline(gpu::RayTracingPipeline* pipeline) override;
void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set) override;
void traceRays(uint32_t width, uint32_t height, uint32_t depth) override;

// Add member variable
vk::RayTracingPipeline* _boundRTPipeline = nullptr;
```

### `lib/src/bg2e/gpu/vk/CommandBuffer.cpp`

Add include:
```cpp
#include <bg2e/gpu/vk/RayTracingPipeline.hpp>
```

Implement methods:

```cpp
void CommandBuffer::bindPipeline(gpu::RayTracingPipeline* pipeline)
{
    auto* vkPipe = dynamic_cast<vk::RayTracingPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindPipeline(RayTracingPipeline): not a vk::RayTracingPipeline");
    }

    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkPipe->handle());
    _boundLayoutHandle = vkPipe->layoutHandle();
    _boundRTPipeline = vkPipe;
}

void CommandBuffer::bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
{
    auto* vkPipe = dynamic_cast<vk::RayTracingPipeline*>(pipeline);
    if (!vkPipe)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(RayTracingPipeline): not a vk::RayTracingPipeline");
    }

    auto* vkSet = dynamic_cast<vk::ResourceSet*>(set);
    if (!vkSet)
    {
        throw std::runtime_error("vk::CommandBuffer::bindResourceSet(RayTracingPipeline): not a vk::ResourceSet");
    }

    VkDescriptorSet ds = vkSet->handle();
    vkCmdBindDescriptorSets(
        _cmd,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        vkPipe->layoutHandle(),
        setIndex,
        1, &ds,
        0, nullptr
    );
}

void CommandBuffer::traceRays(uint32_t width, uint32_t height, uint32_t depth)
{
    if (_boundRTPipeline == nullptr)
    {
        throw std::runtime_error("vk::CommandBuffer::traceRays: no ray tracing pipeline bound");
    }

    cmdTraceRays(
        _cmd,
        &_boundRTPipeline->raygenSBT(),
        &_boundRTPipeline->missSBT(),
        &_boundRTPipeline->hitSBT(),
        &_boundRTPipeline->callableSBT(),
        width, height, depth
    );
}
```

Also add cleanup of `_boundRTPipeline` in the `end()` method and other reset points.

## Design Notes

- `bindPipeline(RayTracingPipeline*)` uses `VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR`.
- `bindResourceSet` is identical to the compute/graphics versions but uses the RT bind point.
- `traceRays` calls `cmdTraceRays` with the SBT regions from the pipeline.
- The pipeline's SBT regions contain valid device addresses set during construction.
- `_boundRTPipeline` is reset in `end()` to avoid stale state.

## Integration Points

- Uses `vk::extensions::cmdTraceRays` (already loaded in `extensions.hpp`).
- Uses `_boundLayoutHandle` for push constants (same mechanism as graphics/compute).
- The SBT regions come from `vk::RayTracingPipeline::raygenSBT()` etc.

## Verification

Engine compiles. Vulkan RT pipeline can be bound, resources bound, and rays traced.
