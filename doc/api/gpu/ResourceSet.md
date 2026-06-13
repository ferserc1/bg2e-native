# ResourceSet

**Header:** `<bg2e/gpu/ResourceSet.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API ResourceSet : public DeviceResource {
public:
    // Assign resources to binding slots
    virtual void setStorageImage (uint32_t binding, gpu::Image*   image)   = 0;
    virtual void setSampledImage (uint32_t binding, gpu::Image*   image)   = 0;
    virtual void setSampler      (uint32_t binding, gpu::Sampler* sampler) = 0;
    virtual void setUniformBuffer(uint32_t binding, gpu::Buffer*  buffer)  = 0;
    virtual void setStorageBuffer(uint32_t binding, gpu::Buffer*  buffer)  = 0;

    // Convenience shared_ptr overloads
    void setUniformBuffer(uint32_t binding, const std::shared_ptr<gpu::Buffer>& buffer);
    void setStorageBuffer(uint32_t binding, const std::shared_ptr<gpu::Buffer>& buffer);

    virtual void update() = 0;       // flush assignments to the backend
    virtual uint32_t setIndex() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
```

A `ResourceSet` groups all resources bound to a single descriptor set (GLSL
`layout(set=N, binding=M)`). It is created via `Device::createResourceSet()`
and bound per frame with `CommandBuffer::bindResourceSet()`.

Resources are assigned to named slots via `set*()` methods, then flushed to
the backend once with `update()`. For **uniform/storage buffers** used with a
`FrameResourceRing`, the descriptor set does **not** need to be re-updated
each frame — only the buffer contents change (via `Buffer::updateUniformBuffer`
or `updateStorageBuffer`).

Created via `Device::createResourceSet(layout, setIndex, debugName)`.

---

## Methods

### `virtual void setStorageImage(uint32_t binding, gpu::Image* image) = 0`

Assigns a storage image to the given binding slot. The image must have been
created with `ImageUsage::Storage`.

### `virtual void setSampledImage(uint32_t binding, gpu::Image* image) = 0`

Assigns a sampled (read-only) image to the given binding slot. The image
layout must have been transitioned to `ImageLayout::ShaderReadOnly` before
the resource set is used in a draw call.

### `virtual void setSampler(uint32_t binding, gpu::Sampler* sampler) = 0`

Assigns a sampler to the given binding slot.

### `virtual void setUniformBuffer(uint32_t binding, gpu::Buffer* buffer) = 0`

Assigns a uniform buffer (created with `Buffer::createUniformBuffer`) to the
given binding slot. The buffer must remain valid and its contents may be
updated each frame via `Buffer::updateUniformBuffer` without re-calling
`update()` on the set.

### `void setUniformBuffer(uint32_t binding, const std::shared_ptr<gpu::Buffer>& buffer)`

Convenience overload that calls `setUniformBuffer(binding, buffer.get())`.
Useful when the buffer is owned by a `FrameResourceRing<gpu::Buffer>`:

```cpp
set->setUniformBuffer(0, modelUboRing.sharedAt(i));
```

### `virtual void setStorageBuffer(uint32_t binding, gpu::Buffer* buffer) = 0`

Assigns a storage buffer (created with `Buffer::createStorageBuffer`) to the
given binding slot.

### `void setStorageBuffer(uint32_t binding, const std::shared_ptr<gpu::Buffer>& buffer)`

Convenience overload that calls `setStorageBuffer(binding, buffer.get())`.

### `virtual void update() = 0`

Flushes all pending slot assignments to the backend. Must be called after all
`set*()` calls and before the resource set is used in a draw or dispatch.

On Vulkan this calls `vkUpdateDescriptorSets`. On Metal the assignments are
cached and replayed into the render/compute encoder at bind time.

### `virtual uint32_t setIndex() const = 0`

Returns the descriptor set index (the `N` in `layout(set=N)`) passed at
creation time.

### `virtual bool isValid() const = 0`

Returns `true` if the resource set has been successfully created.

### `virtual void cleanup() = 0`

Releases backend resources. Must be called before `Device::cleanup()`.

---

## Usage pattern

### Persistent resource set (texture + sampler)

```cpp
// Create once; re-used every frame
auto textureSet = device->createResourceSet(layout.get(), 2, "Material set");
textureSet->setSampledImage(0, texture.get());
textureSet->setSampler(1, sampler.get());
textureSet->update();

// Per frame
cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
```

### Per-frame UBO with FrameResourceRing

```cpp
// Create one buffer per swapchain frame
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buf = device->createBuffer("Model UBO[" + std::to_string(i) + "]");
    buf->createUniformBuffer(ModelUBO{});
    return buf;
});

// Create one resource set per swapchain frame, bound once
gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(layout.get(), 1, "Model set[" + std::to_string(i) + "]");
    set->setUniformBuffer(0, modelUboRing.sharedAt(i));
    set->update();   // called once per slot; no re-update needed later
    return set;
});

// Per frame
auto* modelUbo = modelUboRing.current();
modelUbo->updateUniformBuffer(ModelUBO{ glm::rotate(...) });  // only buffer contents change
auto* modelSet = modelSetRing.current();
cmd->bindResourceSet(pipeline.get(), 1, modelSet);

// Cleanup
modelSetRing.cleanup();
modelUboRing.cleanup();
```

---

## vk::ResourceSet

**Header:** `<bg2e/gpu/vk/ResourceSet.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::ResourceSet`

Backed by a `VkDescriptorPool` + `VkDescriptorSet`. The pool is sized from
the `ResourceBinding` entries in the layout:

- `UniformBuffer` bindings → `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- `StorageBuffer` bindings → `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`
- `SampledImage` bindings  → `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`
- `StorageImage` bindings  → `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`
- `Sampler` bindings       → `VK_DESCRIPTOR_TYPE_SAMPLER`

`update()` calls `vkUpdateDescriptorSets` with the pending `VkWriteDescriptorSet`
entries. Pointer resolution (from `pImageInfo` vs `pBufferInfo`) is deferred
to `update()` to avoid dangling pointers from vector reallocation.

---

## metal::ResourceSet

**Header:** `<bg2e/gpu/metal/ResourceSet.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::ResourceSet`

Metal has no descriptor sets. Instead, `metal::ResourceSet` stores a list of
`ResourceEntry` values keyed by the Metal argument index resolved through
`metal::PipelineLayout::metalIndex()` (textures/samplers) or
`metal::PipelineLayout::metalBufferIndex()` (UBO/SSBO). When
`CommandBuffer::bindResourceSet()` is called, the entries are replayed via
`setVertexBuffer` / `setFragmentBuffer` (or `setBuffer` for compute) on the
current encoder.
