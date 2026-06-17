# PipelineLayout

**Header:** `<bg2e/gpu/PipelineLayout.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API PipelineLayout : public DeviceResource {
public:
    explicit PipelineLayout(Device* device);
    virtual ~PipelineLayout() = default;
};
```

Abstract pipeline layout. Defines push constant ranges and resource bindings
that a pipeline uses. Created via `Device::createPipelineLayout()`.

Concrete implementations (`vk::PipelineLayout`, `metal::PipelineLayout`)
validate the `PipelineLayoutDescription` at construction time and throw
`std::runtime_error` if the description violates any of the following rules:

1. At most one `PushConstantRange` per `ShaderStage`.
2. (Metal only) UniformBuffer/StorageBuffer bindings must use buffer indices
   that do not collide with reserved slots:
   - Vertex stage: `metal >= 2` (0 = vertex buffer, 1 = push constants)
   - Fragment stage: `metal >= 1` (0 = push constants)
   - Compute stage: `metal >= 1` (0 = push constants)
3. (Vulkan only) At most one push constant range per shader stage (same
   restriction as Metal).

---

## Resource bindings

Each resource binding in the layout is a `ResourceBinding` struct with a
`ShaderBinding` that carries two backend-specific indices:

```cpp
struct ShaderBinding {
    uint32_t vulkan;  // Vulkan descriptor binding index (within a set)
    uint32_t metal;   // Metal [[buffer(N)]], [[texture(N)]], or [[sampler(N)]]
};
```

The `set` field of `ResourceBinding` selects the descriptor set in Vulkan and
is ignored in Metal. The `binding.vulkan` field maps to the GLSL binding
index. The `binding.metal` field maps directly to the Metal argument index.

### Metal `[[buffer(n)]]` index reservation

Metal shares a single `[[buffer(n)]]` index namespace per stage across vertex
attribute buffers, push-constant data, and uniform/storage buffers. bg2e::gpu
reserves the following indices by convention:

| Index | Vertex stage | Fragment / Compute stage |
|-------|-------------|--------------------------|
| `0`   | Vertex buffer (`stage_in`) | Push constants |
| `1`   | Push constants | Available for UBO/SSBO |
| `2+`  | Available for UBO/SSBO | Available for UBO/SSBO |

Textures and samplers use independent Metal namespaces (`[[texture(n)]]`,
`[[sampler(n)]]`) where `n` is specified directly in `ShaderBinding::metal`
and can start at 0.

MSL shaders **must** hard-code the matching `[[buffer(n)]]`,
`[[texture(n)]]`, or `[[sampler(n)]]` indices — there is no reflection at
runtime.

---

## Methods

### `virtual bool isValid() const = 0`

Returns `true` if the layout was successfully created.

### `virtual void cleanup() = 0`

Releases the pipeline layout resources.

---

## Example

### Layout with push constants and multiple resource sets

```cpp
// Push constants for vertex stage
gpu::PipelineLayoutDescription layoutDesc{};
layoutDesc.pushConstants.push_back(
    {0, sizeof(CameraPushConstants), gpu::ShaderStage::Vertex}
);

// set 0, Vulkan binding 0 / Metal buffer(2): camera UBO (vertex)
layoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 1, Vulkan binding 0 / Metal texture(0): equirect texture (fragment)
layoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
// set 1, Vulkan binding 1 / Metal sampler(0): sampler (fragment)
layoutDesc.resourceBindings.push_back({
    1, {.vulkan = 1, .metal = 0},
    gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});

auto layout = device->createPipelineLayout(layoutDesc);
```

### Layout with fragment push constants and multiple UBOs

```cpp
gpu::PipelineLayoutDescription desc{};
desc.pushConstants.push_back(
    {0, sizeof(CubeReflectionPushConstants), gpu::ShaderStage::Fragment}
);

// set 0: camera UBO (vertex, Metal buffer 2)
desc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 1: model UBO (vertex, Metal buffer 3)
desc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 2: cubemap texture (fragment, Metal texture 0)
desc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
// set 2: sampler (fragment, Metal sampler 0)
desc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 0},
    gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});
// set 3: render settings UBO (fragment, Metal buffer 1)
desc.resourceBindings.push_back({
    3, {.vulkan = 0, .metal = 1},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1
});

desc.debugName = "Cube reflection pipeline layout";
auto layout = device->createPipelineLayout(desc);
```

### Empty layout (no push constants, no bindings)

```cpp
auto emptyLayout = device->createPipelineLayout({});
```

---

## vk::PipelineLayout

**Header:** `<bg2e/gpu/vk/PipelineLayout.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::PipelineLayout`

```cpp
class PipelineLayout : public gpu::PipelineLayout {
public:
    PipelineLayout(VkDevice device, const gpu::PipelineLayoutDescription& description);
    ~PipelineLayout() override;

    bool isValid() const override;
    void cleanup() override;

    VkPipelineLayout handle() const;
};
```

### Vulkan-specific methods

#### `VkPipelineLayout handle() const`

Returns the raw `VkPipelineLayout` handle.

The `ShaderBinding::vulkan` field is used as the descriptor binding index.
The `metal` field is ignored. Push constants use `VkPushConstantRange` with
the offset from `PushConstantRange::offset`.

---

## metal::PipelineLayout

**Header:** `<bg2e/gpu/metal/PipelineLayout.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::PipelineLayout`

Metal pipeline layout. Stores push constant and resource binding metadata
used during command recording and resource set binding.

```cpp
class PipelineLayout : public gpu::PipelineLayout {
public:
    PipelineLayout(gpu::Device* gpuDevice, const gpu::PipelineLayoutDescription& description);
    ~PipelineLayout() override;

    bool isValid() const override;
    void cleanup() override;

    const gpu::PipelineLayoutDescription& description() const;
    const std::vector<ResourceBinding>& resourceBindings() const;

    // Metal texture/sampler argument index: returns b.binding.metal
    uint32_t metalIndex(const ResourceBinding& b) const;

    // Metal buffer argument index: returns b.binding.metal
    uint32_t metalBufferIndex(const ResourceBinding& b) const;

    // Push-constant buffer slot per stage:
    //   Vertex -> buffer(1), Fragment -> buffer(0), Compute -> buffer(0)
    uint32_t pushConstantBufferIndex(ShaderStage stage) const;
};
```

### Metal binding index resolution

Metal argument indices are stored explicitly in `ShaderBinding::metal`. There
is no computed formula — the caller is responsible for choosing indices that
respect the reservation rules:

| Resource type          | Metal namespace    | Vertex index  | Fragment/Compute index |
|------------------------|--------------------|---------------|------------------------|
| Push constants         | `[[buffer]]`       | 1             | 0                      |
| Vertex buffer          | `[[buffer]]`       | 0             | —                      |
| Uniform/Storage buffer | `[[buffer]]`       | 2, 3, ...     | 1, 2, ...              |
| Sampled/Storage image  | `[[texture]]`      | 0, 1, ...     | 0, 1, ...              |
| Sampler                | `[[sampler]]`      | 0, 1, ...     | 0, 1, ...              |

### `uint32_t metalIndex(const ResourceBinding& b) const`

Returns `b.binding.metal`. Used for texture and sampler bindings.

### `uint32_t metalBufferIndex(const ResourceBinding& b) const`

Returns `b.binding.metal`. Used for UBO/SSBO bindings.

### `uint32_t pushConstantBufferIndex(ShaderStage stage) const`

Returns the Metal buffer index reserved for push constants:

- `ShaderStage::Vertex` → `1`
- `ShaderStage::Fragment` → `0`
- `ShaderStage::Compute` → `0`

### `const std::vector<ResourceBinding>& resourceBindings() const`

Returns the list of all resource bindings declared in the
`PipelineLayoutDescription` supplied at creation time. Used by
`metal::ResourceSet` to validate and resolve binding slots.
