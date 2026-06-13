# GraphicsPipeline

**Header:** `<bg2e/gpu/GraphicsPipeline.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
enum class PrimitiveTopology {
    TriangleList,
    TriangleStrip,
    LineList,
    PointList
};

struct GraphicsPipelineDescription {
    gpu::ShaderModule*                 vertexShader              = nullptr;
    gpu::ShaderModule*                 fragmentShader            = nullptr;
    gpu::PipelineLayout*               layout                    = nullptr;
    PrimitiveTopology                  topology                  = PrimitiveTopology::TriangleList;
    PixelFormat                        colorFormat               = PixelFormat::Undefined;
    PixelFormat                        depthFormat               = PixelFormat::Undefined;
    std::vector<VertexBufferDescription> vertexBufferDescriptions;
    std::string                        debugName;

    void addVertexBufferDescription(const VertexBufferDescription& desc);
};

class BG2E_API GraphicsPipeline {
public:
    virtual ~GraphicsPipeline() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
```

Abstract graphics pipeline. Encapsulates the vertex shader, fragment shader,
pipeline layout, primitive topology, and attachment formats. Created via
`Device::createGraphicsPipeline()`.

---

## PrimitiveTopology

| Value           | Description                              |
|-----------------|------------------------------------------|
| `TriangleList`  | Each 3 vertices form a triangle.         |
| `TriangleStrip` | Each new vertex forms a triangle with the previous two. |
| `LineList`      | Each 2 vertices form a line segment.     |
| `PointList`     | Each vertex is a point.                  |

---

## GraphicsPipelineDescription

| Field                      | Type                                 | Default         | Description                                          |
|----------------------------|--------------------------------------|-----------------|------------------------------------------------------|
| `vertexShader`             | `ShaderModule*`                      | `nullptr`       | Vertex shader (non-owning pointer).                  |
| `fragmentShader`           | `ShaderModule*`                      | `nullptr`       | Fragment shader (non-owning pointer).                |
| `layout`                   | `PipelineLayout*`                    | `nullptr`       | Pipeline layout (non-owning pointer).                |
| `topology`                 | `PrimitiveTopology`                  | `TriangleList`  | Primitive topology.                                  |
| `colorFormat`              | `PixelFormat`                        | `Undefined`     | Color attachment format.                             |
| `depthFormat`              | `PixelFormat`                        | `Undefined`     | Depth attachment format (`Undefined` = no depth).    |
| `vertexBufferDescriptions` | `std::vector<VertexBufferDescription>`| empty          | Vertex buffer layouts; see `addVertexBufferDescription`. |
| `debugName`                | `std::string`                        | `""`            | Optional label shown in GPU debug tools.             |

All pointer fields are **non-owning**. The caller must keep the referenced
objects alive until the pipeline is created.

### `void addVertexBufferDescription(const VertexBufferDescription& desc)`

Appends a `VertexBufferDescription` to `vertexBufferDescriptions`. Equivalent
to `vertexBufferDescriptions.push_back(desc)`.

Typically called once per mesh type before creating the pipeline:

```cpp
desc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(desc);
```

When `vertexBufferDescriptions` is empty the pipeline is created with no
vertex input state (vertex data must be generated procedurally in the vertex
shader). When one or more descriptions are provided, the backend configures
`VkPipelineVertexInputStateCreateInfo` (Vulkan) or `MTL::VertexDescriptor`
(Metal) accordingly.

---

## Methods

### `virtual bool isValid() const = 0`

Returns `true` if the pipeline was successfully created.

### `virtual void cleanup() = 0`

Releases the pipeline resources.

---

## Example

```cpp
auto colorFormat = surface->colorFormat();
auto depthFormat = surface->depthFormat();

gpu::GraphicsPipelineDescription desc{};
desc.vertexShader   = vs.get();
desc.fragmentShader = fs.get();
desc.layout         = layout.get();
desc.topology       = gpu::PrimitiveTopology::TriangleList;
desc.colorFormat    = colorFormat;
desc.depthFormat    = depthFormat;

auto pipeline = device->createGraphicsPipeline(desc);
```

---

## vk::GraphicsPipeline

**Header:** `<bg2e/gpu/vk/GraphicsPipeline.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::GraphicsPipeline`

```cpp
class GraphicsPipeline : public gpu::GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, const gpu::GraphicsPipelineDescription& description);
    ~GraphicsPipeline() override;

    bool isValid() const override;
    void cleanup() override;

    VkPipeline handle() const;
    VkPipelineBindPoint bindPoint() const;
    VkPipelineLayout layoutHandle() const;
};
```

### Vulkan-specific methods

#### `VkPipeline handle() const`

Returns the raw `VkPipeline` handle.

#### `VkPipelineBindPoint bindPoint() const`

Returns `VK_PIPELINE_BIND_POINT_GRAPHICS`.

#### `VkPipelineLayout layoutHandle() const`

Returns the `VkPipelineLayout` handle used by this pipeline.

---

## metal::GraphicsPipeline

**Header:** `<bg2e/gpu/metal/GraphicsPipeline.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::GraphicsPipeline`

```cpp
class GraphicsPipeline : public gpu::GraphicsPipeline {
public:
    bool isValid() const override;
    void cleanup() override;

    MTL::RenderPipelineState* renderPipelineState() const; // macOS only
    MTL::DepthStencilState*   depthStencilState()   const; // macOS only; null when depthFormat == Undefined
    gpu::PrimitiveTopology topology() const;
    metal::PipelineLayout* layout() const;
};
```

### Depth testing

When `GraphicsPipelineDescription::depthFormat` is not `Undefined`, the
constructor creates a `MTL::DepthStencilState` with `CompareFunctionLess` and
depth writes enabled. `CommandBuffer::bindPipeline(GraphicsPipeline*)` calls
`setDepthStencilState()` on the current render encoder automatically, so
depth testing is active for all draw calls that follow.

When `depthFormat` is `Undefined`, `depthStencilState()` returns `nullptr`
and no depth state is set.

### `MTL::DepthStencilState* depthStencilState() const`

Returns the Metal depth-stencil state object, or `nullptr` if no depth
attachment was specified in `GraphicsPipelineDescription`. Available on
macOS only.
