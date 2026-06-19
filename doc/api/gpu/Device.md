# Device

**Header:** `<bg2e/gpu/Device.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Device {
public:
    virtual ~Device() = default;

    virtual void create(Instance* instance, PhysicalDevice* physicalDevice,
                        Surface* surface) = 0;
    virtual void cleanup() = 0;
    virtual void waitIdle() = 0;

    virtual bool isValid() const = 0;

    virtual const Queue& graphicsQueue() const = 0;
    virtual const Queue& presentQueue() const = 0;
    virtual const Queue& transferQueue() const = 0;

    virtual std::shared_ptr<Image>  createImage(const ImageDescription& description);
    virtual std::shared_ptr<Sampler> createSampler(const SamplerDescription& description);
    virtual std::unique_ptr<ResourceSet> createResourceSet(PipelineLayout* layout,
        uint32_t setIndex, const std::string& debugName = {});

    virtual std::unique_ptr<ShaderModule> createShaderModule(
        const ShaderModuleDescription& description);
    virtual std::unique_ptr<PipelineLayout> createPipelineLayout(
        const PipelineLayoutDescription& description);
    virtual std::unique_ptr<GraphicsPipeline> createGraphicsPipeline(
        const GraphicsPipelineDescription& description);
    virtual std::unique_ptr<ComputePipeline> createComputePipeline(
        const ComputePipelineDescription& description);
    virtual std::unique_ptr<Buffer> createBuffer(const std::string& debugName = {});

    virtual std::shared_ptr<RayTracingMesh>  createRayTracingMesh(const RayTracingMeshDescription& description);
    virtual std::shared_ptr<RayTracingScene> createRayTracingScene(const std::string& debugName = {});

    virtual void immediateSubmit(std::function<void(CommandBuffer* cmd)>&& function);
};
```

Represents a logical device created from a physical device. Provides access
to command queues and factory methods for creating shader modules, pipeline
layouts, and pipelines.

---

## Methods

### `virtual void create(Instance* instance, PhysicalDevice* physicalDevice, Surface* surface) = 0`

Creates the logical device. The instance, physical device, and surface must
already be initialized.

| Parameter        | Type              | Description                 |
|------------------|-------------------|-----------------------------|
| `instance`       | `Instance*`       | The GPU instance.           |
| `physicalDevice` | `PhysicalDevice*` | The chosen physical device. |
| `surface`        | `Surface*`        | The rendering surface.      |

### `virtual void cleanup() = 0`

Destroys the logical device and releases all associated resources.

### `virtual void waitIdle() = 0`

Blocks until all pending GPU operations on this device have completed.

### `virtual bool isValid() const = 0`

Returns `true` if the device has been successfully created.

### `virtual const Queue& graphicsQueue() const = 0`

Returns the graphics command queue. Valid only after `create()`.

### `virtual const Queue& presentQueue() const = 0`

Returns the presentation command queue. Valid only after `create()`.

### `virtual const Queue& transferQueue() const = 0`

Returns the transfer command queue. Valid only after `create()`.

### `virtual std::unique_ptr<ShaderModule> createShaderModule(const ShaderModuleDescription& description)`

Creates a shader module from the given description. The default implementation
throws `std::runtime_error`; backends override this with platform-specific
logic.

| Parameter     | Type                        | Description             |
|---------------|-----------------------------|-------------------------|
| `description` | `ShaderModuleDescription`   | Shader path, entry point, and stage. |

### `virtual std::unique_ptr<PipelineLayout> createPipelineLayout(const PipelineLayoutDescription& description)`

Creates a pipeline layout defining push constant ranges and (future) descriptor
set layouts.

| Parameter     | Type                          | Description             |
|---------------|-------------------------------|-------------------------|
| `description` | `PipelineLayoutDescription`   | Push constant ranges.   |

### `virtual std::unique_ptr<GraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDescription& description)`

Creates a graphics pipeline with the specified shaders, topology, and
attachment formats.

| Parameter     | Type                            | Description             |
|---------------|---------------------------------|-------------------------|
| `description` | `GraphicsPipelineDescription`   | Pipeline configuration. |

### `virtual std::unique_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescription& description)`

Creates a compute pipeline with the specified compute shader and layout.

| Parameter     | Type                           | Description             |
|---------------|--------------------------------|-------------------------|
| `description` | `ComputePipelineDescription`   | Pipeline configuration. |

### `virtual std::unique_ptr<Buffer> createBuffer(const std::string& debugName = {})`

Allocates a new GPU buffer. The buffer is empty until one of its
`create*` methods is called:

```cpp
auto buf = device->createBuffer("My vertex buffer");
buf->createVertexBuffer(mesh.vertices);     // upload vertex data
buf->createIndexBuffer(mesh.indices);       // upload index data
```

The returned `Buffer` handles both the device-local allocation and the
internal staging upload transparently.

| Parameter   | Type          | Default | Description                          |
|-------------|---------------|---------|--------------------------------------|
| `debugName` | `std::string` | `""`    | Optional label for GPU debug tools.  |

See [Buffer](Buffer.md) for the full API.

### `virtual std::shared_ptr<RayTracingMesh> createRayTracingMesh(const RayTracingMeshDescription& description)`

Creates a bottom-level ray tracing acceleration structure for one submesh, from
existing GPU vertex/index buffers. Requires a ray-tracing-capable device
(`PhysicalDeviceProperties::rayTracingSupported()`); throws otherwise. See
[RayTracingMesh](RayTracingMesh.md).

| Parameter     | Type                          | Description                          |
|---------------|-------------------------------|--------------------------------------|
| `description` | `RayTracingMeshDescription`   | Shared vertex/index buffers + submesh range. |

### `virtual std::shared_ptr<RayTracingScene> createRayTracingScene(const std::string& debugName = {})`

Creates a top-level ray tracing acceleration structure (scene) that holds
instances of `RayTracingMesh` objects. Requires a ray-tracing-capable device;
throws otherwise. See [RayTracingScene](RayTracingScene.md).

| Parameter   | Type          | Default | Description                          |
|-------------|---------------|---------|--------------------------------------|
| `debugName` | `std::string` | `""`    | Optional label for GPU debug tools.  |

### `virtual std::shared_ptr<Image> createImage(const ImageDescription& description)`

Allocates a GPU image (texture). See [Image](Image.md).

### `virtual std::shared_ptr<Sampler> createSampler(const SamplerDescription& description)`

Creates a texture sampler. See the sampler description in [Image](Image.md).

### `virtual std::unique_ptr<ResourceSet> createResourceSet(PipelineLayout* layout, uint32_t setIndex, const std::string& debugName = {})`

Allocates a descriptor set (resource set) bound to `setIndex` of the given
pipeline layout.

| Parameter   | Type             | Description                              |
|-------------|------------------|------------------------------------------|
| `layout`    | `PipelineLayout*`| Layout that declares the set bindings.  |
| `setIndex`  | `uint32_t`       | Descriptor set slot index.              |
| `debugName` | `std::string`    | Optional debug label.                   |

### `virtual void immediateSubmit(std::function<void(CommandBuffer* cmd)>&& function)`

Records and submits a one-shot command buffer synchronously. The lambda
receives a `CommandBuffer*`, records commands (e.g. layout transitions,
buffer/image uploads), and the method blocks until the GPU finishes.

```cpp
device->immediateSubmit([&texture](gpu::CommandBuffer* cmd) {
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
```

Used internally by `Buffer::createVertexBuffer`, `Buffer::createIndexBuffer`,
and `Image::uploadRGBA8` to perform staging uploads.

---

## vk::Device

**Header:** `<bg2e/gpu/vk/Device.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Device`

```cpp
class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice,
                gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    std::shared_ptr<gpu::Image>       createImage(const gpu::ImageDescription&) override;
    std::shared_ptr<gpu::Sampler>     createSampler(const gpu::SamplerDescription&) override;
    std::unique_ptr<gpu::ResourceSet> createResourceSet(gpu::PipelineLayout*, uint32_t, const std::string&) override;
    std::unique_ptr<gpu::ShaderModule>    createShaderModule(const gpu::ShaderModuleDescription&) override;
    std::unique_ptr<gpu::PipelineLayout>  createPipelineLayout(const gpu::PipelineLayoutDescription&) override;
    std::unique_ptr<gpu::GraphicsPipeline> createGraphicsPipeline(const gpu::GraphicsPipelineDescription&) override;
    std::unique_ptr<gpu::ComputePipeline>  createComputePipeline(const gpu::ComputePipelineDescription&) override;
    std::unique_ptr<gpu::Buffer>           createBuffer(const std::string& debugName = {}) override;
    std::shared_ptr<gpu::RayTracingMesh>   createRayTracingMesh(const gpu::RayTracingMeshDescription&) override;
    std::shared_ptr<gpu::RayTracingScene>  createRayTracingScene(const std::string& debugName = {}) override;

    VkDevice handle() const;

    bool     rayTracingEnabled() const;
    uint32_t accelerationStructureScratchAlignment() const;
};
```

Vulkan logical device wrapper. Creates `VkDevice` with the required queue
families and exposes factory methods for Vulkan resources.

### Vulkan-specific methods

#### `VkDevice handle() const`

Returns the raw `VkDevice` handle.

#### `bool rayTracingEnabled() const`

Returns `true` when the device was created with the ray tracing extensions
enabled (`VK_KHR_acceleration_structure`, `VK_KHR_ray_query`,
`VK_KHR_buffer_device_address`, …). When `true`, vertex/index buffers are
created with the acceleration-structure build-input usage so they can back a
`RayTracingMesh`.

#### `uint32_t accelerationStructureScratchAlignment() const`

Returns the minimum scratch buffer device-address alignment
(`minAccelerationStructureScratchOffsetAlignment`) used when sizing acceleration
structure build scratch buffers.

---

## metal::Device

**Header:** `<bg2e/gpu/metal/Device.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Device`

```cpp
class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice,
                gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    std::shared_ptr<gpu::Image>       createImage(const gpu::ImageDescription&) override;
    std::shared_ptr<gpu::Sampler>     createSampler(const gpu::SamplerDescription&) override;
    std::unique_ptr<gpu::ResourceSet> createResourceSet(gpu::PipelineLayout*, uint32_t, const std::string&) override;
    std::unique_ptr<gpu::ShaderModule>    createShaderModule(const gpu::ShaderModuleDescription&) override;
    std::unique_ptr<gpu::PipelineLayout>  createPipelineLayout(const gpu::PipelineLayoutDescription&) override;
    std::unique_ptr<gpu::GraphicsPipeline> createGraphicsPipeline(const gpu::GraphicsPipelineDescription&) override;
    std::unique_ptr<gpu::ComputePipeline>  createComputePipeline(const gpu::ComputePipelineDescription&) override;
    std::unique_ptr<gpu::Buffer>           createBuffer(const std::string& debugName = {}) override;
    std::shared_ptr<gpu::RayTracingMesh>   createRayTracingMesh(const gpu::RayTracingMeshDescription&) override;
    std::shared_ptr<gpu::RayTracingScene>  createRayTracingScene(const std::string& debugName = {}) override;

    DeviceHandle handle() const;
};
```

Metal logical device. Wraps the Metal device handle and exposes
graphics/present/transfer queues.

### Metal-specific methods

#### `DeviceHandle handle() const`

Returns the raw `MTL::Device*` handle.
