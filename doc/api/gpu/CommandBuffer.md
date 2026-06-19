# CommandBuffer

**Header:** `<bg2e/gpu/CommandBuffer.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    // Recording lifecycle
    virtual void begin() = 0;
    virtual void end() = 0;

    // Image layout transitions
    virtual void transition(gpu::Image* image, ImageLayout newLayout) = 0;

    // Dynamic rendering (graphics)
    virtual void beginRendering(gpu::SurfaceFrame* frame) = 0;
    virtual void endRendering() = 0;

    // Compute passes
    virtual void beginCompute() = 0;
    virtual void endCompute() = 0;

    // Clear operations
    virtual void clearColor(uint32_t attachmentIndex, const gpu::Color& color) = 0;
    virtual void clearDepth(float depth) = 0;

    // Pipeline binding
    virtual void bindPipeline(gpu::GraphicsPipeline* pipeline);
    virtual void bindPipeline(gpu::ComputePipeline* pipeline);

    // Resource set binding
    virtual void bindResourceSet(gpu::GraphicsPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set);
    virtual void bindResourceSet(gpu::ComputePipeline* pipeline,  uint32_t setIndex, gpu::ResourceSet* set);

    // Vertex / index buffer binding
    virtual void bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer,
                                  uint64_t offset = 0);
    virtual void bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset = 0);

    // Draw calls
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                      uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                             uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                             uint32_t firstInstance = 0);

    // Compute dispatch
    virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                          uint32_t groupCountZ);

    // Push constants
    virtual void pushConstants(ShaderStage stage, uint32_t offset,
                               uint32_t size, const void* data);

    // Ray tracing acceleration structure builds
    virtual void buildRayTracingMesh(gpu::RayTracingMesh* mesh);
    virtual void buildRayTracingScene(gpu::RayTracingScene* scene);

    virtual bool isValid() const = 0;
};
```

Abstract command buffer. Records GPU commands for later execution on a queue.
Created via `Queue::createCommandBuffer()`.

Methods marked as non-pure-virtual have default implementations that throw
`std::runtime_error`. Backends override them when the feature is supported.

---

## Recording lifecycle

### `virtual void begin() = 0`

Begins recording commands. Must be called before any other recording method.

### `virtual void end() = 0`

Ends recording. The command buffer is now ready for submission via
`Queue::submit()`.

---

## Image transitions

### `virtual void transition(gpu::Image* image, ImageLayout newLayout) = 0`

Records an image layout transition. The image's `currentLayout()` is updated
after the transition is recorded.

| Parameter   | Type          | Description                    |
|-------------|---------------|--------------------------------|
| `image`     | `gpu::Image*` | The image to transition.       |
| `newLayout` | `ImageLayout` | The target layout.             |

---

## Dynamic rendering

### `virtual void beginRendering(gpu::SurfaceFrame* frame) = 0`

Begins a dynamic rendering pass using the color and depth images from the
given frame. Clears are applied if `clearColor()` or `clearDepth()` were
called before this method.

| Parameter | Type              | Description                    |
|-----------|-------------------|--------------------------------|
| `frame`   | `SurfaceFrame*`   | The frame to render into.      |

### `virtual void endRendering() = 0`

Ends the current dynamic rendering pass.

---

## Compute passes

### `virtual void beginCompute() = 0`

Begins a compute pass.

### `virtual void endCompute() = 0`

Ends the current compute pass.

---

## Clear operations

### `virtual void clearColor(uint32_t attachmentIndex, const gpu::Color& color) = 0`

Sets the clear value for a color attachment. Must be called before
`beginRendering()`.

| Parameter         | Type           | Description                      |
|-------------------|----------------|----------------------------------|
| `attachmentIndex` | `uint32_t`     | Color attachment index (usually 0).|
| `color`           | `gpu::Color`   | Clear color (RGBA floats).       |

### `virtual void clearDepth(float depth) = 0`

Sets the clear value for the depth attachment. Must be called before
`beginRendering()`.

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| `depth`   | `float` | Depth clear value (0.0--1.0). |

---

## Pipeline binding

### `virtual void bindPipeline(gpu::GraphicsPipeline* pipeline)`

Binds a graphics pipeline for subsequent draw calls.

| Parameter  | Type                    | Description           |
|------------|-------------------------|-----------------------|
| `pipeline` | `gpu::GraphicsPipeline*`| The pipeline to bind. |

### `virtual void bindPipeline(gpu::ComputePipeline* pipeline)`

Binds a compute pipeline for subsequent dispatch calls.

| Parameter  | Type                   | Description           |
|------------|------------------------|-----------------------|
| `pipeline` | `gpu::ComputePipeline*`| The pipeline to bind. |

---

## Resource set binding

### `virtual void bindResourceSet(gpu::GraphicsPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)`

Binds a resource set to the given descriptor set slot for graphics commands.
Must be called after `bindPipeline()` and before the draw call that uses the
resources. The `pipeline` argument is required to resolve the pipeline layout.

| Parameter  | Type                     | Description                              |
|------------|--------------------------|------------------------------------------|
| `pipeline` | `gpu::GraphicsPipeline*` | The currently bound graphics pipeline.   |
| `setIndex` | `uint32_t`               | Descriptor set index (matches `set=N` in GLSL). |
| `set`      | `gpu::ResourceSet*`      | The resource set to bind.                |

### `virtual void bindResourceSet(gpu::ComputePipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)`

Binds a resource set for compute commands. Must be called after
`bindPipeline(ComputePipeline*)` and before `dispatch()`.

| Parameter  | Type                    | Description                              |
|------------|-------------------------|------------------------------------------|
| `pipeline` | `gpu::ComputePipeline*` | The currently bound compute pipeline.    |
| `setIndex` | `uint32_t`              | Descriptor set index.                    |
| `set`      | `gpu::ResourceSet*`     | The resource set to bind.                |

---

## Vertex and index buffer binding

### `virtual void bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset = 0)`

Binds a vertex buffer to the given binding slot. Must be called inside a
rendering pass (after `beginRendering()`), and requires a graphics pipeline to
have been created with a matching `VertexBufferDescription` at the same
`binding` index.

| Parameter | Type          | Default | Description                              |
|-----------|---------------|---------|------------------------------------------|
| `binding` | `uint32_t`    | --      | Vertex buffer binding slot.              |
| `buffer`  | `gpu::Buffer*`| --      | Buffer created with `BufferUsage::Vertex`.|
| `offset`  | `uint64_t`    | 0       | Byte offset into the buffer.             |

### `virtual void bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset = 0)`

Binds an index buffer for use with `drawIndexed()`. Indices are always
`uint32_t` (32-bit unsigned).

| Parameter | Type          | Default | Description                             |
|-----------|---------------|---------|------------------------------------------|
| `buffer`  | `gpu::Buffer*`| --      | Buffer created with `BufferUsage::Index`.|
| `offset`  | `uint64_t`    | 0       | Byte offset into the buffer.             |

---

## Draw calls

### `virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)`

Issues a non-indexed draw call. A graphics pipeline must be bound first.
Vertex data is generated procedurally in the shader (no vertex buffer needed).

| Parameter       | Type       | Default | Description              |
|-----------------|------------|---------|--------------------------|
| `vertexCount`   | `uint32_t` | --      | Number of vertices.      |
| `instanceCount` | `uint32_t` | 1       | Number of instances.     |
| `firstVertex`   | `uint32_t` | 0       | Index of first vertex.   |
| `firstInstance` | `uint32_t` | 0       | Index of first instance. |

### `virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)`

Issues an indexed draw call. A graphics pipeline, a vertex buffer, and an
index buffer must be bound before calling this. Indices are always `uint32_t`.

| Parameter      | Type       | Default | Description                                        |
|----------------|------------|---------|----------------------------------------------------|
| `indexCount`   | `uint32_t` | --      | Number of indices to read.                         |
| `instanceCount`| `uint32_t` | 1       | Number of instances.                               |
| `firstIndex`   | `uint32_t` | 0       | Offset (in indices) into the index buffer.         |
| `vertexOffset` | `int32_t`  | 0       | Value added to each index before fetching a vertex.|
| `firstInstance`| `uint32_t` | 0       | Index of first instance.                           |

In practice, `gpu::MeshGeneric<T>::draw()` and `drawSubmesh()` call this for
you.

---

## Compute dispatch

### `virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)`

Dispatches a compute workload. A compute pipeline must be bound first.

| Parameter      | Type       | Description                    |
|----------------|------------|--------------------------------|
| `groupCountX`  | `uint32_t` | Number of workgroups in X.     |
| `groupCountY`  | `uint32_t` | Number of workgroups in Y.     |
| `groupCountZ`  | `uint32_t` | Number of workgroups in Z.     |

---

## Push constants

### `virtual void pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data)`

Pushes constant data to the bound pipeline. The pipeline layout must define
a push constant range covering the specified stage, offset, and size.

| Parameter | Type          | Description                    |
|-----------|---------------|--------------------------------|
| `stage`   | `ShaderStage` | Target shader stage.           |
| `offset`  | `uint32_t`    | Byte offset into push data (Vulkan only; ignored by Metal). |
| `size`    | `uint32_t`    | Size in bytes.                 |
| `data`    | `const void*` | Pointer to the data.           |

In Metal, the `offset` is ignored. The push constant buffer index is
determined by the stage: `buffer(1)` for vertex shaders, `buffer(0)` for
fragment and compute shaders. In Vulkan, at most one push constant range per
shader stage is allowed.

---

## Ray tracing acceleration structure builds

Both Vulkan and Metal build acceleration structures through GPU commands, so the
builds are recorded into a command buffer. They must be called **outside** an
active rendering scope (before `beginRendering()`).

### `virtual void buildRayTracingMesh(gpu::RayTracingMesh* mesh)`

Records the build of a bottom-level acceleration structure
([`RayTracingMesh`](RayTracingMesh.md)). Typically recorded once per mesh inside
a `Device::immediateSubmit`.

| Parameter | Type                   | Description                          |
|-----------|------------------------|--------------------------------------|
| `mesh`    | `gpu::RayTracingMesh*` | The bottom-level structure to build. |

### `virtual void buildRayTracingScene(gpu::RayTracingScene* scene)`

Records the build/update of a top-level acceleration structure
([`RayTracingScene`](RayTracingScene.md)) from its current instance list. This
is what `RayTracingScene::buildOrUpdate(cmd)` calls internally.

| Parameter | Type                    | Description                          |
|-----------|-------------------------|--------------------------------------|
| `scene`   | `gpu::RayTracingScene*` | The top-level structure to build.    |

```cpp
// Build BLASes once.
device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
    for (auto& rtMesh : rtMeshes) cmd->buildRayTracingMesh(rtMesh.get());
});

// Build/update the TLAS each frame, before rendering.
cmd->begin();
cmd->buildRayTracingScene(scene.get());   // or scene->buildOrUpdate(cmd)
cmd->beginRendering(frame.get());
// ...
```

---

## Example

```cpp
auto cmd = graphicsQueue.createCommandBuffer();
cmd->begin();

// Transition images
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);

// Set clear values and begin rendering
cmd->clearDepth(1.0f);
cmd->beginRendering(frame.get());

// Bind pipeline, resources, and draw using vertex/index buffers
cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, textureSet.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
mesh.draw(cmd.get());   // calls bindVertexBuffer + bindIndexBuffer + drawIndexed

cmd->endRendering();
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
```

For manual (non-mesh) use:

```cpp
cmd->bindVertexBuffer(0, vertexBuffer.get());
cmd->bindIndexBuffer(indexBuffer.get());
cmd->drawIndexed(indexCount);
```

---

## vk::CommandBuffer

**Header:** `<bg2e/gpu/vk/CommandBuffer.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::CommandBuffer`

```cpp
class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer(vk::Device* device, VkCommandBuffer cmd, VkCommandPool pool);

    // All virtual methods overridden
    VkCommandBuffer handle() const;
};
```

### Vulkan-specific methods

#### `VkCommandBuffer handle() const`

Returns the raw `VkCommandBuffer` handle.

---

## metal::CommandBuffer

**Header:** `<bg2e/gpu/metal/CommandBuffer.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::CommandBuffer`

```cpp
class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer(metal::Device* device, MTL::CommandBuffer* cmd);

    // All virtual methods overridden
    MTL::CommandBuffer* handle() const;
};
```

### Metal-specific methods

#### `MTL::CommandBuffer* handle() const`

Returns the raw `MTL::CommandBuffer*` handle.
