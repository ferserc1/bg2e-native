# PipelineLayout

**Header:** `<bg2e/gpu/PipelineLayout.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API PipelineLayout {
public:
    virtual ~PipelineLayout() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
```

Abstract pipeline layout. Defines push constant ranges and (future) descriptor
set layouts that a pipeline uses. Created via
`Device::createPipelineLayout()`.

---

## Methods

### `virtual bool isValid() const = 0`

Returns `true` if the layout was successfully created.

### `virtual void cleanup() = 0`

Releases the pipeline layout resources.

---

## Example

```cpp
// Layout with push constants for the fragment stage
gpu::PipelineLayoutDescription layoutDesc{};
layoutDesc.pushConstants.push_back({
    0,                    // offset
    sizeof(PushConstants),// size
    gpu::ShaderStage::Fragment
});
auto layout = device->createPipelineLayout(layoutDesc);

// Layout with UBOs and a texture+sampler pair (no push constants)
gpu::PipelineLayoutDescription cubeLayoutDesc{};
cubeLayoutDesc.resourceBindings.push_back({ 0, 0, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex,   1 });
cubeLayoutDesc.resourceBindings.push_back({ 1, 0, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex,   1 });
cubeLayoutDesc.resourceBindings.push_back({ 2, 0, gpu::ResourceType::SampledImage,  gpu::ShaderStage::Fragment, 1 });
cubeLayoutDesc.resourceBindings.push_back({ 2, 1, gpu::ResourceType::Sampler,       gpu::ShaderStage::Fragment, 1 });
cubeLayoutDesc.debugName = "Cube pipeline layout";
auto cubeLayout = device->createPipelineLayout(cubeLayoutDesc);

// Empty layout (no push constants, no bindings)
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
    bool isValid() const override;
    void cleanup() override;

    const gpu::PipelineLayoutDescription& description() const;
    const std::vector<ResourceBinding>& resourceBindings() const;

    // Texture / sampler index (= binding number, independent namespace)
    uint32_t metalIndex(const ResourceBinding& b) const;

    // Buffer namespace index for UBO / SSBO
    static constexpr uint32_t MaxVertexBuffers  = 8;
    static constexpr uint32_t MaxBindingsPerSet = 8;
    uint32_t metalBufferIndex(const ResourceBinding& b) const;

    // Push-constant buffer slot (stays in the vertex-buffer region)
    static constexpr uint32_t PushConstantBufferIndex = 0;
    uint32_t pushConstantBufferIndex(ShaderStage stage) const;
};
```

### Metal `[[buffer(n)]]` index partitioning

Metal shares a single `[[buffer(n)]]` index namespace per stage across vertex
attribute buffers, push-constant data, and uniform/storage buffers. To avoid
collisions the layout partitions the namespace as follows:

| Range | Owner |
|-------|-------|
| `[0]` | Push-constant data (`PushConstantBufferIndex = 0`) |
| `[1 .. MaxVertexBuffers-1]` | Vertex attribute buffers (`stage_in`) |
| `[MaxVertexBuffers + set*MaxBindingsPerSet + binding]` | UBO / SSBO |

With the default constants (`MaxVertexBuffers = 8`, `MaxBindingsPerSet = 8`):

| GLSL `set, binding` | Metal `[[buffer(n)]]` |
|---------------------|-----------------------|
| set 0, binding 0    | `buffer(8)`           |
| set 0, binding 1    | `buffer(9)`           |
| set 1, binding 0    | `buffer(16)`          |

MSL shaders **must** hard-code the matching `[[buffer(n)]]` indices — there is
no reflection at runtime. Textures and samplers use independent Metal
namespaces (`[[texture(n)]]`, `[[sampler(n)]]`) where `n = binding`.

### `uint32_t metalBufferIndex(const ResourceBinding& b) const`

Returns `MaxVertexBuffers + b.set * MaxBindingsPerSet + b.binding`.

### `uint32_t metalIndex(const ResourceBinding& b) const`

Returns `b.binding`. Used for texture and sampler bindings whose Metal index
equals the binding number directly.

### `const std::vector<ResourceBinding>& resourceBindings() const`

Returns the list of all resource bindings declared in the
`PipelineLayoutDescription` supplied at creation time. Used by
`metal::ResourceSet` to validate and resolve binding slots.
