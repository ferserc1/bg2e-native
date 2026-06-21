# Step 08: Metal CommandBuffer RT Integration

## Goal

Implement `bindPipeline(RayTracingPipeline*)`, `bindResourceSet(RayTracingPipeline*, ...)`, and `traceRays()` in the Metal command buffer, managing compute encoders internally.

## Files to Modify

### `lib/include/bg2e/gpu/metal/CommandBuffer.hpp`

Add forward declaration and overrides:

```cpp
// In CommandBuffer class
class RayTracingPipeline;

void bindPipeline(gpu::RayTracingPipeline* pipeline) override;
void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set) override;
void traceRays(uint32_t width, uint32_t height, uint32_t depth) override;

// Add member variable (inside #if BG2E_IS_MAC block)
metal::RayTracingPipeline* _boundRTPipeline = nullptr;
```

### `lib/src/bg2e/gpu/metal/CommandBuffer.cpp`

Add include:
```cpp
#include <bg2e/gpu/metal/RayTracingPipeline.hpp>
```

Implement methods:

```cpp
void CommandBuffer::bindPipeline(gpu::RayTracingPipeline* pipeline)
{
    auto* metalPipeline = dynamic_cast<metal::RayTracingPipeline*>(pipeline);
    if (!metalPipeline)
    {
        throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): not a metal::RayTracingPipeline");
    }

    // Create compute encoder if not already active
    if (!_computeEncoder)
    {
        if (!_cmd)
        {
            throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): no command buffer");
        }
        _computeEncoder = _cmd->computeCommandEncoder();
        if (!_computeEncoder)
        {
            throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): failed to create compute encoder");
        }
    }

    _computeEncoder->setComputePipelineState(metalPipeline->computePipelineState());
    _boundComputePipeline = nullptr;
    _boundRTPipeline = metalPipeline;
    _boundLayout = metalPipeline->layout();
}

void CommandBuffer::bindResourceSet(gpu::RayTracingPipeline* /*pipeline*/, uint32_t /*setIndex*/, gpu::ResourceSet* set)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(RayTracingPipeline): no active compute scope");
    }

    auto* metalSet = dynamic_cast<metal::ResourceSet*>(set);
    if (!metalSet)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(RayTracingPipeline): not a metal::ResourceSet");
    }

    // Reuse the same resource binding logic as compute pipelines
    for (const auto& entry : metalSet->entries())
    {
        if (entry.texture)
        {
            _computeEncoder->setTexture(entry.texture, entry.index);
        }
        if (entry.sampler)
        {
            _computeEncoder->setSamplerState(entry.sampler, entry.index);
        }
        if (entry.buffer)
        {
            _computeEncoder->setBuffer(entry.buffer, 0, entry.index);
        }
        if (entry.rtScene)
        {
            for (auto* prim : entry.rtScene->referencedPrimitives())
            {
                _computeEncoder->useResource(prim, MTL::ResourceUsageRead);
            }
            _computeEncoder->setAccelerationStructure(entry.rtScene->handle(), entry.index);
        }
    }
}

void CommandBuffer::traceRays(uint32_t width, uint32_t height, uint32_t depth)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::traceRays: no active compute scope; call bindPipeline(RayTracingPipeline) first");
    }

    // Use 8x8 threadgroups to match the Metal shader threadgroup_size
    constexpr NS::UInteger kThreadGroupWidth  = 8;
    constexpr NS::UInteger kThreadGroupHeight = 8;

    MTL::Size threadsPerGroup = MTL::Size::Make(kThreadGroupWidth, kThreadGroupHeight, 1);
    MTL::Size threadGroups = MTL::Size::Make(
        (width + kThreadGroupWidth - 1) / kThreadGroupWidth,
        (height + kThreadGroupHeight - 1) / kThreadGroupHeight,
        depth
    );
    _computeEncoder->dispatchThreadgroups(threadGroups, threadsPerGroup);
}
```

## Key Design Decisions

1. **Auto-manage compute encoder**: `bindPipeline(RayTracingPipeline*)` creates a compute encoder if one doesn't exist. This allows the application to call `bindPipeline(RT)` without explicit `beginCompute()`/`endCompute()` scope.

2. **Resource binding reuses compute path**: The resource binding logic is identical to `bindResourceSet(ComputePipeline*, ...)` — textures, samplers, buffers, and acceleration structures are all bound through the compute encoder.

3. **Threadgroup size matches shader**: The 8x8 threadgroup size must match the Metal shader's `[[threads_per_threadgroup]]` attribute. The dispatch computes `ceil(width/8) x ceil(height/8)` threadgroups.

4. **Encoder lifecycle**: The compute encoder created by `bindPipeline(RT)` persists until `endCompute()` is called or a new encoding scope begins. This allows multiple `traceRays()` calls per frame if needed.

## Integration Points

- Reuses `metal::ResourceSet::entries()` for resource binding.
- Reuses `metal::RayTracingScene::referencedPrimitives()` for AS residency.
- The compute encoder is the same type used by regular compute pipelines.

## Verification

Engine compiles. Metal RT pipeline can be bound and rays dispatched via compute encoder.
