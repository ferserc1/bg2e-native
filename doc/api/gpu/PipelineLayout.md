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

Metal pipeline layout. Stores push constant metadata for use during command
recording.
