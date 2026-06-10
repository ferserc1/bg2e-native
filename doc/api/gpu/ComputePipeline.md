# ComputePipeline

**Header:** `<bg2e/gpu/ComputePipeline.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
struct ComputePipelineDescription {
    gpu::ShaderModule*   computeShader = nullptr;
    gpu::PipelineLayout* layout        = nullptr;
};

class BG2E_API ComputePipeline {
public:
    virtual ~ComputePipeline() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
```

Abstract compute pipeline. Encapsulates a compute shader and its pipeline
layout. Created via `Device::createComputePipeline()`.

---

## ComputePipelineDescription

| Field          | Type              | Default   | Description                            |
|----------------|-------------------|-----------|----------------------------------------|
| `computeShader`| `ShaderModule*`   | `nullptr` | Compute shader (non-owning pointer).   |
| `layout`       | `PipelineLayout*` | `nullptr` | Pipeline layout (non-owning pointer).  |

Both pointer fields are **non-owning**. The caller must keep the referenced
objects alive until the pipeline is created.

---

## Methods

### `virtual bool isValid() const = 0`

Returns `true` if the pipeline was successfully created.

### `virtual void cleanup() = 0`

Releases the pipeline resources.

---

## Example

```cpp
// Empty layout (no bindings or push constants)
auto computeLayout = device->createPipelineLayout({});

// Create compute pipeline
gpu::ComputePipelineDescription desc{};
desc.computeShader = cs.get();
desc.layout        = computeLayout.get();

auto computePipeline = device->createComputePipeline(desc);

// Dispatch in a command buffer
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
cmd->dispatch(64, 1, 1);
cmd->endCompute();
```

---

## vk::ComputePipeline

**Header:** `<bg2e/gpu/vk/ComputePipeline.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::ComputePipeline`

```cpp
class ComputePipeline : public gpu::ComputePipeline {
public:
    ComputePipeline(VkDevice device, const gpu::ComputePipelineDescription& description);
    ~ComputePipeline() override;

    bool isValid() const override;
    void cleanup() override;

    VkPipeline handle() const;
    VkPipelineBindPoint bindPoint() const;
};
```

### Vulkan-specific methods

#### `VkPipeline handle() const`

Returns the raw `VkPipeline` handle.

#### `VkPipelineBindPoint bindPoint() const`

Returns `VK_PIPELINE_BIND_POINT_COMPUTE`.

---

## metal::ComputePipeline

**Header:** `<bg2e/gpu/metal/ComputePipeline.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::ComputePipeline`

Metal compute pipeline state object.
