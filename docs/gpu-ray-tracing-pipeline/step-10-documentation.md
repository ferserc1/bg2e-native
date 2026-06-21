# Step 10: Documentation

## Goal

Update existing documentation and create new documentation for the ray tracing pipeline API.

## Files to Create

### `doc/api/gpu/RayTracingPipeline.md` (new)

Full API documentation covering:

```markdown
# Ray Tracing Pipeline

**Header:** `<bg2e/gpu/RayTracingPipeline.hpp>` (included by `<bg2e/gpu/all.hpp>`)
**Namespace:** `bg2e::gpu`

A **RayTracingPipeline** dispatches ray tracing shaders (ray generation, miss,
closest hit) against an acceleration structure. It is the core abstraction for
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
| Miss/closestHit shaders | Required SPIR-V modules | Ignored (null accepted) |
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
};
```

### `gpu::RayTracingPipeline`

```cpp
class BG2E_API RayTracingPipeline : public DeviceResource {
public:
    explicit RayTracingPipeline(Device* device);
    virtual ~RayTracingPipeline() = default;
};
```

### `gpu::Device::createRayTracingPipeline()`

```cpp
std::shared_ptr<RayTracingPipeline> createRayTracingPipeline(
    const RayTracingPipelineDescription& description);
```

### `gpu::CommandBuffer` RT methods

```cpp
void bindPipeline(gpu::RayTracingPipeline* pipeline);
void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set);
void traceRays(uint32_t width, uint32_t height, uint32_t depth = 1);
```

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
```

## Files to Modify

### `doc/api/gpu/Common.md`

Update `ShaderStage` enum documentation:

```markdown
### `ShaderStage`

Pipeline stage a shader module targets.

```cpp
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    RayGeneration,  // ray generation shader (RT pipeline)
    Miss,           // miss shader (RT pipeline)
    ClosestHit      // closest hit shader (RT pipeline)
};
```

The ray tracing stages (`RayGeneration`, `Miss`, `ClosestHit`) are used with
`RayTracingPipeline`. On Metal, only `RayGeneration` is used — `Miss` and
`ClosestHit` are null/ignored because Metal handles those stages internally in
the compute kernel.
```

Add `BufferUsage::ShaderBindingTable`:

```markdown
| `ShaderBindingTable`  | SBT buffer for ray tracing pipelines |
```

### `doc/api/gpu/ShaderLibraries.md`

Add RT methods to the API reference section:

```markdown
#### `rayGeneration / miss / closestHit`

```cpp
std::shared_ptr<ShaderModule> rayGeneration(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");

std::shared_ptr<ShaderModule> miss(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");

std::shared_ptr<ShaderModule> closestHit(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");
```

Load ray tracing shader modules. The file resolution follows the same pattern
as `vertex()` / `fragment()` / `compute()`:

| Method | Stage extension | Vulkan file | Metal file | Vulkan entry | Metal entry |
|--------|----------------|-------------|------------|--------------|-------------|
| `rayGeneration()` | `.rgen` | `.rgen.spv` | `.rgen.metallib` | `main` | `rgenMain` |
| `miss()` | `.rmiss` | `.rmiss.spv` | `.rmiss.metallib` | `main` | `rmissMain` |
| `closestHit()` | `.rchit` | `.rchit.spv` | `.rchit.metallib` | `main` | `rchitMain` |

**Metal note:** On Metal, `miss()` and `closestHit()` return `nullptr` when the
`.rmiss.metallib` or `.rchit.metallib` file does not exist. This is expected
because Metal handles miss/hit behavior internally in the compute kernel. The
`RayTracingPipeline` accepts null pointers for these shaders without error.

`rayGeneration()` always throws if the file is missing (the rgen shader is
required on both backends).
```

### `doc/api/gpu/CommandBuffer.md`

Add RT methods:

```markdown
### `bindPipeline(RayTracingPipeline*)`

```cpp
void bindPipeline(gpu::RayTracingPipeline* pipeline);
```

Binds a ray tracing pipeline for subsequent `traceRays()` calls. On Vulkan, binds
at `VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR`. On Metal, creates/reuses a compute
encoder.

### `bindResourceSet(RayTracingPipeline*, ...)`

```cpp
void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set);
```

Binds a resource set to the bound ray tracing pipeline. Same binding model as
compute/graphics pipelines.

### `traceRays()`

```cpp
void traceRays(uint32_t width, uint32_t height, uint32_t depth = 1);
```

Dispatches ray tracing across the specified dimensions. On Vulkan, calls
`vkCmdTraceRaysKHR` with the pipeline's internal SBT. On Metal, dispatches
the rgen compute kernel as `ceil(width/8) x ceil(height/8) x depth` threadgroups.
```

### `doc/api/gpu/Device.md`

Add `createRayTracingPipeline()`:

```markdown
### `createRayTracingPipeline()`

```cpp
std::shared_ptr<RayTracingPipeline> createRayTracingPipeline(
    const RayTracingPipelineDescription& description);
```

Creates a ray tracing pipeline from the description. The pipeline owns the
internal shader binding table (Vulkan) or compute pipeline state (Metal).

Throws `std::runtime_error` if ray tracing is not supported by the device.
```

### `doc/api/gpu/reference.md`

Add link to new RayTracingPipeline doc:

```markdown
- [`RayTracingPipeline`](RayTracingPipeline.md) — ray tracing pipeline dispatch
```

### `doc/api/gpu/quick_start.md`

Add a ray tracing pipeline recipe:

```markdown
### Ray tracing pipeline (RT)

1. Load shaders:
   ```cpp
   auto rgen = shaderLib->rayGeneration("path_tracer", device.get());
   auto rmiss = shaderLib->miss("path_tracer", device.get());
   auto rchit = shaderLib->closestHit("path_tracer", device.get());
   ```

2. Create pipeline layout with storage image, camera UBO, acceleration structure, etc.

3. Create pipeline:
   ```cpp
   gpu::RayTracingPipelineDescription desc{};
   desc.raygenShader = rgen.get();
   desc.missShader = rmiss.get();
   desc.closestHitShader = rchit.get();
   desc.layout = layout.get();
   auto pipeline = device->createRayTracingPipeline(desc);
   ```

4. Dispatch:
   ```cpp
   cmd->bindPipeline(pipeline.get());
   cmd->bindResourceSet(pipeline.get(), 0, outputSet.get());
   cmd->bindResourceSet(pipeline.get(), 1, sceneSet.get());
   cmd->traceRays(width, height);
   ```
```

### `doc/api/gpu/index.md`

Add ray tracing pipeline section to the architecture overview.

## Verification

Documentation is consistent with implementation. All new APIs are documented with usage examples.
