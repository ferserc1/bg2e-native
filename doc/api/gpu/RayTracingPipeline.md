# Ray Tracing Pipeline

**Header:** `<bg2e/gpu/RayTracingPipeline.hpp>` (included by `<bg2e/gpu/all.hpp>`)
**Namespace:** `bg2e::gpu`

A **RayTracingPipeline** dispatches ray tracing shaders (ray generation, miss,
closest hit, optional any-hit on Vulkan) against an acceleration structure. It is the core abstraction for
ray-traced effects: global illumination, reflections, soft shadows, and path
tracing.

---

## Architecture

The ray tracing pipeline follows the same resource creation model as
`GraphicsPipeline` and `ComputePipeline`:

1. Load shader modules via `ShaderLib`
2. Create a `PipelineLayout` describing resource bindings
3. Create the `RayTracingPipeline` from the description
4. Bind resources via `ResourceSet`
5. Dispatch with `CommandBuffer::traceRays()`

### Backend differences

| Aspect | Vulkan | Metal |
|--------|--------|-------|
| Pipeline type | Real `VK_KHR_ray_tracing_pipeline` | `MTLComputePipelineState` from `.rgen.metallib` |
| Shader Binding Table | Internal, auto-created | N/A |
| Miss/closestHit shaders | Optional SPIR-V modules; provide stages needed by dispatched rays | Ignored (null accepted) |
| Dispatch | `vkCmdTraceRaysKHR` | Compute `dispatchThreadgroups` |
| Ray tracing model | Hardware RT pipeline | Compute + intersector |

---

## API reference

### `gpu::RayTracingPipelineDescription`

```cpp
struct RayTracingPipelineDescription {
    gpu::ShaderModule*   raygenShader     = nullptr;
    gpu::ShaderModule*   missShader       = nullptr; // nullable on Metal
    gpu::ShaderModule*   closestHitShader = nullptr; // nullable on Metal
    gpu::PipelineLayout* layout           = nullptr;
    uint32_t             maxRecursionDepth = 1;
    std::string          debugName;
    gpu::ShaderModule*   anyHitShader = nullptr; // optional Vulkan stage
};
```

| Field             | Type              | Description                                      |
|-------------------|-------------------|--------------------------------------------------|
| `raygenShader`    | `ShaderModule*`   | Ray generation shader (required on all backends).  |
| `missShader`      | `ShaderModule*`   | Miss shader (null on Metal).                       |
| `closestHitShader`| `ShaderModule*`   | Closest hit shader (null on Metal).                |
| `layout`          | `PipelineLayout*` | Pipeline layout with descriptor bindings.         |
| `maxRecursionDepth`| `uint32_t`       | Maximum ray recursion depth (default: 1).         |
| `anyHitShader`    | `ShaderModule*`   | Optional Vulkan alpha-candidate rejection stage; rejected if supplied to Metal. |
| `debugName`       | `std::string`     | Optional label for GPU debug tools.               |

### `gpu::RayTracingPipeline`

```cpp
class BG2E_API RayTracingPipeline : public DeviceResource {
public:
    explicit RayTracingPipeline(Device* device);
    virtual ~RayTracingPipeline() = default;
};
```

The abstract base class for ray tracing pipelines. Both backends inherit from
this class and implement their own pipeline management:

- **Vulkan:** owns `VkPipeline` handle and a Shader Binding Table (SBT) buffer.
- **Metal:** wraps `MTLComputePipelineState` loaded from the ray generation
  shader's metallib. Miss and closest hit shaders are ignored because Metal
  handles those stages internally in the compute kernel.

### `gpu::Device::createRayTracingPipeline()`

```cpp
std::shared_ptr<RayTracingPipeline> createRayTracingPipeline(
    const RayTracingPipelineDescription& description);
```

Creates a ray tracing pipeline from the description. The pipeline owns the
internal shader binding table (Vulkan) or compute pipeline state (Metal).

Throws `std::runtime_error` if ray tracing is not supported by the device.

### `gpu::CommandBuffer` RT methods

```cpp
void bindPipeline(gpu::RayTracingPipeline* pipeline);
void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set);
void traceRays(uint32_t width, uint32_t height, uint32_t depth = 1);
```

| Method | Description |
|--------|-------------|
| `bindPipeline(RayTracingPipeline*)` | Binds an RT pipeline for subsequent `traceRays()` calls. On Vulkan, binds at `VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR`. On Metal, creates a compute encoder. |
| `bindResourceSet(RT pipeline*, setIndex, ResourceSet*)` | Binds a resource set to the bound RT pipeline. Same binding model as compute/graphics pipelines. |
| `traceRays(width, height, depth)` | Dispatches ray tracing across the specified dimensions. On Vulkan, calls `vkCmdTraceRaysKHR` with the pipeline's internal SBT. On Metal, dispatches the rgen compute kernel as `ceil(width/8) x ceil(height/8) x depth` threadgroups. |

---

## Usage example

```cpp
// Load RT shaders
auto rgen = shaderLib->rayGeneration("path_tracer", device.get());
auto rmiss = shaderLib->miss("path_tracer", device.get());     // may be nullptr on Metal
auto rchit = shaderLib->closestHit("path_tracer", device.get()); // may be nullptr on Metal

// Create pipeline
gpu::RayTracingPipelineDescription desc{};
desc.raygenShader = rgen.get();
desc.missShader = rmiss.get();
desc.closestHitShader = rchit.get();
desc.layout = layout.get();
auto pipeline = device->createRayTracingPipeline(desc);

// Dispatch
cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, resourceSet.get());
cmd->traceRays(width, height);
```

---

## See also

- [`ShaderLibraries`](ShaderLibraries.md) — loading ray generation, miss, and closest hit shaders
- [`CommandBuffer`](CommandBuffer.md) — `traceRays()` reference
- [`Device`](Device.md) — `createRayTracingPipeline()` factory
- [`RayTracingMesh`](RayTracingMesh.md) — bottom-level acceleration structures
- [`RayTracingScene`](RayTracingScene.md) — top-level acceleration structures

## Any-hit and material-resource prerequisites

`ShaderLib::anyHit()` resolves `.rahit.spv` with entry `main`; an absent Metal file may return null. Vulkan uses one triangle hit group containing optional closest-hit/any-hit modules, with all hit material addressing supplied through primitive buffers/resources. Modules must be valid, correctly staged and belong to the pipeline device. Recursion and dispatch dimensions obey native device limits. Missing miss/hit SBT regions are zero; provide every stage required by rays actually dispatched.

Use `ResourceBinding::additionalStages` to expose the same descriptor to raygen, closest-hit and any-hit without duplicate set/binding entries. Bounded image/sampler tables require `Device::resourceArrayCapabilities()`, separately from tracing support. Metal advanced tracing remains independently authored compute/intersection-query code. See [ResourceSet](ResourceSet.md) for arrays and lifetime rules. No high-level AO/GI/reflection pass is supplied by these primitive changes.

The separate high-level [draw effects](../draw/index.md) now consume these primitives. GPU itself owns no scene, material or effect policy; its Metal compute/query boundary remains unchanged.
