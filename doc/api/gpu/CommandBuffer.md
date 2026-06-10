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

    // Draw calls
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                      uint32_t firstVertex = 0, uint32_t firstInstance = 0);

    // Compute dispatch
    virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                          uint32_t groupCountZ);

    // Push constants
    virtual void pushConstants(ShaderStage stage, uint32_t offset,
                               uint32_t size, const void* data);

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

## Draw calls

### `virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)`

Issues a non-indexed draw call. A graphics pipeline must be bound first.

| Parameter       | Type       | Default | Description              |
|-----------------|------------|---------|--------------------------|
| `vertexCount`   | `uint32_t` | --      | Number of vertices.      |
| `instanceCount` | `uint32_t` | 1       | Number of instances.     |
| `firstVertex`   | `uint32_t` | 0       | Index of first vertex.   |
| `firstInstance` | `uint32_t` | 0       | Index of first instance. |

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
| `offset`  | `uint32_t`    | Byte offset into push data.    |
| `size`    | `uint32_t`    | Size in bytes.                 |
| `data`    | `const void*` | Pointer to the data.           |

---

## Example

```cpp
auto cmd = graphicsQueue.createCommandBuffer();
cmd->begin();

// Transition images
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);

// Set clear values and begin rendering
cmd->clearColor(0, { 0.1f, 0.1f, 0.1f, 1.0f });
cmd->clearDepth(1.0f);
cmd->beginRendering(frame.get());

// Bind pipeline, push constants, and draw
cmd->bindPipeline(pipeline.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
cmd->draw(3);

cmd->endRendering();
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
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
