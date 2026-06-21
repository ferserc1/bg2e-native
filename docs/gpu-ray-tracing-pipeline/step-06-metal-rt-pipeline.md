# Step 06: Metal RayTracingPipeline Backend

## Goal

Implement the Metal backend for `RayTracingPipeline` using a compute pipeline created from the `.rgen.metallib` shader.

## Files to Create

### `lib/include/bg2e/gpu/metal/RayTracingPipeline.hpp` (new)

```cpp
#pragma once

#include <bg2e/gpu/RayTracingPipeline.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class PipelineLayout;

class BG2E_API RayTracingPipeline : public gpu::RayTracingPipeline {
public:
#if BG2E_IS_MAC
    RayTracingPipeline(gpu::Device* gpuDevice, MTL::Device* device,
                       const gpu::RayTracingPipelineDescription& description);
#else
    RayTracingPipeline(gpu::Device* gpuDevice, void* /*device*/,
                       const gpu::RayTracingPipelineDescription& description);
#endif
    ~RayTracingPipeline() override;

    bool isValid() const override;
    void cleanup() override;

#if BG2E_IS_MAC
    MTL::ComputePipelineState* computePipelineState() const { return _computePipelineState; }
    NS::UInteger maxTotalThreadsPerThreadgroup() const { return _maxTotalThreadsPerThreadgroup; }
#endif
    metal::PipelineLayout* layout() const { return _layout; }

private:
    metal::PipelineLayout* _layout = nullptr;
#if BG2E_IS_MAC
    MTL::ComputePipelineState* _computePipelineState = nullptr;
    NS::UInteger _maxTotalThreadsPerThreadgroup = 0;
#endif
};

}
}
}
```

### `lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp` (new)

```cpp
#include <bg2e/gpu/metal/RayTracingPipeline.hpp>
#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

RayTracingPipeline::RayTracingPipeline(
    gpu::Device* gpuDevice, MTL::Device* device,
    const gpu::RayTracingPipelineDescription& description)
    : gpu::RayTracingPipeline(gpuDevice)
{
    auto* metalRgenModule = dynamic_cast<metal::ShaderModule*>(description.raygenShader);
    auto* metalLayout = dynamic_cast<metal::PipelineLayout*>(description.layout);

    if (!metalRgenModule)
    {
        throw std::runtime_error("metal::RayTracingPipeline: raygen shader is not a metal::ShaderModule");
    }

    _layout = metalLayout;

    // missShader and closestHitShader are intentionally ignored on Metal.
    // Metal handles miss/hit behavior internally in the compute kernel.

    if (!metalRgenModule->isValid())
    {
        throw std::runtime_error("metal::RayTracingPipeline: raygen shader module is not valid");
    }

    // Create compute pipeline state from the rgen function
    NS::Error* error = nullptr;
    _computePipelineState = device->newComputePipelineState(
        metalRgenModule->function(), &error);

    if (!_computePipelineState)
    {
        std::string errorMsg = "metal::RayTracingPipeline: failed to create compute pipeline state";
        if (error)
        {
            errorMsg += " - " + std::string(error->localizedDescription()->utf8String());
        }
        throw std::runtime_error(errorMsg);
    }

    _maxTotalThreadsPerThreadgroup = _computePipelineState->maxTotalThreadsPerThreadgroup();
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::isValid() const
{
    return _computePipelineState != nullptr;
}

void RayTracingPipeline::cleanup()
{
    if (_computePipelineState)
    {
        _computePipelineState->release();
        _computePipelineState = nullptr;
    }
    _maxTotalThreadsPerThreadgroup = 0;
}

#else

RayTracingPipeline::RayTracingPipeline(
    gpu::Device* gpuDevice, void* /*device*/,
    const gpu::RayTracingPipelineDescription& /*description*/)
    : gpu::RayTracingPipeline(gpuDevice)
{
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::isValid() const
{
    return false;
}

void RayTracingPipeline::cleanup()
{
}

#endif

}
}
}
```

## Files to Modify

### `lib/include/bg2e/gpu/metal/Device.hpp`

Add override:
```cpp
std::shared_ptr<gpu::RayTracingPipeline> createRayTracingPipeline(
    const gpu::RayTracingPipelineDescription& description) override;
```

### `lib/src/bg2e/gpu/metal/Device.cpp`

Add include and implement:
```cpp
#include <bg2e/gpu/metal/RayTracingPipeline.hpp>

std::shared_ptr<gpu::RayTracingPipeline> Device::createRayTracingPipeline(
    const gpu::RayTracingPipelineDescription& description)
{
    return std::make_shared<metal::RayTracingPipeline>(this, _device, description);
}
```

## Key Design Decisions

1. **Miss and closestHit are silently ignored**: The Metal `RayTracingPipeline` constructor receives the full description but only uses `raygenShader`. If `missShader` or `closestHitShader` are null or invalid, no error is produced. This is the correct behavior because Metal handles hit/miss logic internally in the compute kernel using intersector functions.

2. **Identical to ComputePipeline internally**: The Metal RT pipeline wraps `MTLComputePipelineState` exactly like `metal::ComputePipeline`. The difference is semantic — the rgen shader is a ray tracing compute kernel, not a regular compute shader.

3. **Threadgroup size comes from the shader**: The `maxTotalThreadsPerThreadgroup` is queried from the pipeline state, which reflects the `[[threads_per_threadgroup]]` attribute in the Metal shader.

## Integration Points

- Reuses existing `metal::ShaderModule::function()` to get the MTLFunction for the compute pipeline.
- The `metal::PipelineLayout` is stored for use by `CommandBuffer::bindResourceSet()`.
- No new Metal API calls needed — everything goes through the existing Metal C++ wrappers.

## Verification

Engine compiles. Metal RT pipeline can be created from a rgen metallib. The compute pipeline state is valid and ready for dispatch.
