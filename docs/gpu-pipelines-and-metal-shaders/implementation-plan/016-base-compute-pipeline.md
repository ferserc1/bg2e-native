# Step 016 — Base `gpu::ComputePipeline` + description

## Objective

Introduce the backend-independent `gpu::ComputePipeline` and `ComputePipelineDescription`
(compute shader module + pipeline layout), and a compile-safe `Device::createComputePipeline` hook.
No backend implementation.

## Context

A compute pipeline needs only a compute shader stage and a layout. Vulkan → `VkPipeline` with
`VK_PIPELINE_BIND_POINT_COMPUTE`; Metal → `MTL::ComputePipelineState` from a kernel `MTL::Function`.

## Expected prior state

- Steps 003–008 done (shader module + pipeline layout exist). Triangle (009–015) may be done; this
  triad is independent of the graphics pipeline triad except for sharing shader-module/layout.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/ComputePipeline.hpp` — `gpu::ComputePipeline` (abstract) +
  `ComputePipelineDescription`.
- Modify: `lib/include/bg2e/gpu/Device.hpp` — add `createComputePipeline(...)` non-pure virtual,
  throwing default.
- Modify: `lib/include/bg2e/gpu/all.hpp` — include the new header.

## Proposed design

```cpp
// gpu/ComputePipeline.hpp  (sketch)
namespace bg2e::gpu {
struct ComputePipelineDescription {
    gpu::ShaderModule*   computeShader = nullptr; // not owned, stage == Compute
    gpu::PipelineLayout* layout        = nullptr; // not owned
};

class BG2E_API ComputePipeline {
public:
    virtual ~ComputePipeline() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
}
```

- `Device` gains
  `virtual std::unique_ptr<gpu::ComputePipeline> createComputePipeline(const ComputePipelineDescription&);`
  with throwing default.

## Required changes (no code in this plan)

- New header; default-throwing factory on `gpu::Device`.

## Compilation criteria

- Build green; both backends inherit the throwing default.

## Validation criteria

- Types visible via `all.hpp`; nothing instantiates yet.

## Risks / points to check

- Ensure `computeShader->stage()` is expected to be `Compute`; document (backends may assert).

## What must NOT be done in this step

- No backend implementation (017/018). No dispatch (019).
