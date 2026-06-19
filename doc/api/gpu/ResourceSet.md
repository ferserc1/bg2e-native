# ResourceSet

**Header:** `<bg2e/gpu/ResourceSet.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API ResourceSet : public DeviceResource {
public:
    explicit ResourceSet(Device* device);
    virtual ~ResourceSet() = default;

    // Assign resources to binding slots
    virtual void setStorageImage (ShaderBinding binding, gpu::Image*   image)   = 0;
    virtual void setSampledImage (ShaderBinding binding, gpu::Image*   image)   = 0;
    virtual void setSampler      (ShaderBinding binding, gpu::Sampler* sampler) = 0;
    virtual void setUniformBuffer(ShaderBinding binding, gpu::Buffer*  buffer)  = 0;
    virtual void setStorageBuffer(ShaderBinding binding, gpu::Buffer*  buffer)  = 0;
    virtual void setRayTracingScene(ShaderBinding binding, gpu::RayTracingScene* scene) = 0;

    void setSampledCubeMap(ShaderBinding binding, gpu::CubeMap* cubeMap);

    // Convenience shared_ptr overloads
    void setUniformBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer);
    void setStorageBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer);

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

## ShaderBinding

Every `set*()` method takes a `ShaderBinding` instead of a plain `uint32_t`.
A `ShaderBinding` carries two indices:

```cpp
struct ShaderBinding {
    uint32_t vulkan;  // Vulkan descriptor binding index (within a set)
    uint32_t metal;   // Metal [[buffer(N)]], [[texture(N)]], or [[sampler(N)]]
};
```

Use the designated initializer syntax to specify both:

```cpp
set->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
set->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
set->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
```

### Metal binding rules

Metal uses three independent index namespaces: `[[buffer(N)]]` for uniform/
storage buffers and push constants, `[[texture(N)]]` for sampled/storage
images, and `[[sampler(N)]]` for samplers. The `metal` field of `ShaderBinding`
targets the appropriate namespace depending on the resource type.

For **UniformBuffer** and **StorageBuffer** bindings, the Metal buffer index
must respect reserved slots:

| Shader stage | Reserved indices | Available buffer indices |
|--------------|------------------|--------------------------|
| Vertex       | 0 (vertex buffer), 1 (push constants) | `metal >= 2` |
| Fragment     | 0 (push constants) | `metal >= 1` |
| Compute      | 0 (push constants) | `metal >= 1` |

These restrictions do **not** apply to SampledImage, StorageImage, or Sampler
bindings, because `[[texture(N)]]` and `[[sampler(N)]]` are separate
namespaces — indices can start at 0.

### Vulkan binding rules

In Vulkan, the `set` field of `ResourceBinding` selects the descriptor set and
`binding.vulkan` selects the binding within that set. The `metal` field is
ignored. Only one push constant range per shader stage is allowed (same
restriction as Metal).

---

## Methods

### `virtual void setStorageImage(ShaderBinding binding, gpu::Image* image) = 0`

Assigns a storage image to the given binding slot. The image must have been
created with `ImageUsage::Storage`.

### `virtual void setSampledImage(ShaderBinding binding, gpu::Image* image) = 0`

Assigns a sampled (read-only) image to the given binding slot. The image
layout must have been transitioned to `ImageLayout::ShaderReadOnly` before
the resource set is used in a draw call.

### `virtual void setSampler(ShaderBinding binding, gpu::Sampler* sampler) = 0`

Assigns a sampler to the given binding slot.

### `virtual void setUniformBuffer(ShaderBinding binding, gpu::Buffer* buffer) = 0`

Assigns a uniform buffer (created with `Buffer::createUniformBuffer`) to the
given binding slot. The buffer must remain valid and its contents may be
updated each frame via `Buffer::updateUniformBuffer` without re-calling
`update()` on the set.

### `void setUniformBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer)`

Convenience overload that calls `setUniformBuffer(binding, buffer.get())`.
Useful when the buffer is owned by a `FrameResourceRing<gpu::Buffer>`:

```cpp
set->setUniformBuffer({.vulkan = 0, .metal = 2}, modelUboRing.sharedAt(i));
```

### `virtual void setStorageBuffer(ShaderBinding binding, gpu::Buffer* buffer) = 0`

Assigns a storage buffer (created with `Buffer::createStorageBuffer`) to the
given binding slot.

### `void setStorageBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer)`

Convenience overload that calls `setStorageBuffer(binding, buffer.get())`.

### `virtual void setRayTracingScene(ShaderBinding binding, gpu::RayTracingScene* scene) = 0`

Assigns a [`RayTracingScene`](RayTracingScene.md) (top-level acceleration
structure) to the given binding slot, declared as
`ResourceType::AccelerationStructure` in the layout. The scene must have been
built at least once (`buildOrUpdate`) before the set is used in a draw call.

```cpp
lightSet->setRayTracingScene({.vulkan = 1, .metal = 2}, rayTracingScene.get());
lightSet->update();
```

- **Vulkan:** written as a `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR`
  descriptor at `binding.vulkan` (GLSL `accelerationStructureEXT`).
- **Metal:** bound as an `instance_acceleration_structure` at the
  `binding.metal` buffer index; the scene's primitive acceleration structures
  are made resident automatically at bind time.

### `void setSampledCubeMap(ShaderBinding binding, gpu::CubeMap* cubeMap)`

Convenience method that extracts the underlying `gpu::Image*` from a `CubeMap`
and calls `setSampledImage()`. Used to bind a cubemap texture for sampling in
shaders.

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
auto textureSet = device->createResourceSet(layout.get(), 1, "Material set");
textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
textureSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
textureSet->update();

// Per frame
cmd->bindResourceSet(pipeline.get(), 1, textureSet.get());
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
    set->setUniformBuffer({.vulkan = 0, .metal = 2}, modelUboRing.sharedAt(i));
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

### Multi-set layout with push constants (complete example)

This example shows a pipeline layout with vertex push constants, a camera UBO,
a model UBO, a cubemap texture + sampler, and a fragment settings UBO:

```cpp
// Pipeline layout description
gpu::PipelineLayoutDescription layoutDesc{};
layoutDesc.pushConstants.push_back(
    {0, sizeof(CameraPushConstants), gpu::ShaderStage::Vertex}
);

// set 0, Vulkan binding 0 / Metal buffer(2): camera UBO (vertex stage)
layoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 1, Vulkan binding 0 / Metal buffer(3): model UBO (vertex stage)
layoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 2, Vulkan binding 0 / Metal texture(0): cubemap texture (fragment stage)
layoutDesc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
// set 2, Vulkan binding 1 / Metal sampler(0): sampler (fragment stage)
layoutDesc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 0},
    gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});
// set 3, Vulkan binding 0 / Metal buffer(1): render settings UBO (fragment stage)
layoutDesc.resourceBindings.push_back({
    3, {.vulkan = 0, .metal = 1},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1
});

auto layout = device->createPipelineLayout(layoutDesc);

// Create and populate resource sets
auto cameraSet = device->createResourceSet(layout.get(), 0, "Camera set");
cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
cameraSet->update();

auto modelSet = device->createResourceSet(layout.get(), 1, "Model set");
modelSet->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUbo);
modelSet->update();

auto cubemapSet = device->createResourceSet(layout.get(), 2, "Cubemap set");
cubemapSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, cubeMap.get());
cubemapSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
cubemapSet->update();

auto settingsSet = device->createResourceSet(layout.get(), 3, "Settings set");
settingsSet->setUniformBuffer({.vulkan = 0, .metal = 1}, settingsUbo);
settingsSet->update();

// Bind during rendering
cmd->bindPipeline(pipeline.get());
cmd->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(CameraPushConstants), &pushData);
cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
cmd->bindResourceSet(pipeline.get(), 1, modelSet.get());
cmd->bindResourceSet(pipeline.get(), 2, cubemapSet.get());
cmd->bindResourceSet(pipeline.get(), 3, settingsSet.get());
mesh.draw(cmd.get());
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
- `AccelerationStructure` bindings → `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR`

`setRayTracingScene()` writes the descriptor through a
`VkWriteDescriptorSetAccelerationStructureKHR` chained into the
`VkWriteDescriptorSet` via `pNext`. As with image/buffer writes, the chained
struct is resolved in `update()` to keep its pointer stable.

`update()` calls `vkUpdateDescriptorSets` with the pending `VkWriteDescriptorSet`
entries. Pointer resolution (from `pImageInfo` vs `pBufferInfo`) is deferred
to `update()` to avoid dangling pointers from vector reallocation.

The `ShaderBinding::vulkan` field is used as the descriptor binding index. The
`metal` field is ignored.

---

## metal::ResourceSet

**Header:** `<bg2e/gpu/metal/ResourceSet.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::ResourceSet`

Metal has no descriptor sets. Instead, `metal::ResourceSet` stores a list of
`ResourceEntry` values keyed by the Metal argument index resolved through
`ShaderBinding::metal`. When `CommandBuffer::bindResourceSet()` is called,
the entries are replayed via `setVertexBuffer` / `setFragmentBuffer` (or
`setBuffer` for compute) on the current encoder.

For UniformBuffer and StorageBuffer, the `metal` field maps to `[[buffer(N)]]`.
For SampledImage and StorageImage, it maps to `[[texture(N)]]`. For Sampler,
it maps to `[[sampler(N)]]`. For AccelerationStructure, it maps to an
`instance_acceleration_structure` argument in the `[[buffer(N)]]` namespace;
when bound, the scene's primitive acceleration structures are marked resident
via `useResource`.

The `set` field of `ResourceBinding` is ignored in Metal — all resources are
bound by their Metal argument index directly.
