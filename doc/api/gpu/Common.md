# Common types

**Header:** `<bg2e/gpu/Common.hpp>`
**Namespace:** `bg2e::gpu`

This header defines the fundamental enums, structs, and types used throughout
the GPU abstraction layer.

---

## Enums

### `BackendType`

Selects which graphics backend to use.

```cpp
enum class BackendType {
    Vulkan,
    Metal
};
```

### `ShaderStage`

Pipeline stage a shader module targets.

```cpp
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    RayGeneration,  // ray generation shader (RT pipeline)
    Miss,           // miss shader (RT pipeline)
    ClosestHit      // closest hit shader (RT pipeline)
};
```

The ray tracing stages (`RayGeneration`, `Miss`, `ClosestHit`) are used with
`RayTracingPipeline`. On Metal, only `RayGeneration` is used — `Miss` and
`ClosestHit` are null/ignored because Metal handles those stages internally in
the compute kernel.

### `ImageLayout`

Image layout states for transitions. Used in `CommandBuffer::transition()`.

```cpp
enum class ImageLayout {
    Undefined = 0,
    General,
    ColorAttachment,
    DepthAttachment,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Present
};
```

| Value             | Description                                      |
|-------------------|--------------------------------------------------|
| `Undefined`       | Initial or discarded state.                      |
| `General`         | General-purpose access (compute, transfer).      |
| `ColorAttachment` | Writable color render target.                    |
| `DepthAttachment` | Writable depth/stencil render target.            |
| `ShaderReadOnly`  | Read-only texture sampling.                      |
| `TransferSrc`     | Source for copy operations.                      |
| `TransferDst`     | Destination for copy or clear operations.        |
| `Present`         | Ready for swapchain presentation.                |

### `PixelFormat`

Pixel format for images, surfaces, and attachments.

```cpp
enum class PixelFormat {
    Undefined = 0,

    // Color
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_SFLOAT,
    R32G32B32A32_SFLOAT,

    // Depth / stencil
    D16_UNORM,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT
};
```

#### Helper functions

```cpp
constexpr bool isDepthFormat(PixelFormat f);
constexpr bool hasStencil(PixelFormat f);
```

### `PipelineBarrierFlags`

Bitmask flags for pipeline barriers (used internally for synchronization).

```cpp
enum class PipelineBarrierFlags : uint32_t {
    None                 = 0,
    AllCommands          = 1 << 0,
    VertexInput          = 1 << 1,
    VertexShader         = 1 << 2,
    FragmentShader       = 1 << 3,
    ComputeShader        = 1 << 4,
    Transfer             = 1 << 5,
    ColorAttachmentOutput = 1 << 6,
    EarlyFragment        = 1 << 7,
    LateFragment         = 1 << 8,
    BottomOfPipe         = 1 << 9,
    TopOfPipe            = 1 << 10,
    MemoryRead           = 1 << 11,
    MemoryWrite          = 1 << 12
};
```

Supports bitwise `|`, `&`, `|=`, and the helper `hasFlag()`.

---

## Structs

### `Size2D`

```cpp
struct Size2D {
    uint32_t width  = 0;
    uint32_t height = 0;

    Size2D() = default;
    Size2D(uint32_t w, uint32_t h);

    bool operator==(const Size2D& o) const;
    bool operator!=(const Size2D& o) const;
    bool isZero() const;
};
```

### `Size3D`

```cpp
struct Size3D {
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t depth  = 1;

    Size3D() = default;
    Size3D(uint32_t w, uint32_t h, uint32_t d = 1);
    explicit Size3D(const Size2D& s, uint32_t d = 1);

    Size2D toSize2D() const;
    bool operator==(const Size3D& o) const;
    bool operator!=(const Size3D& o) const;
};
```

### `Color`

```cpp
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f);
};
```

Used in `CommandBuffer::clearColor()` to specify clear values.

### `PushConstantRange`

```cpp
struct PushConstantRange {
    uint32_t    offset = 0;
    uint32_t    size   = 0;
    ShaderStage stage  = ShaderStage::Vertex;
};
```

| Field    | Type          | Description                                      |
|----------|---------------|--------------------------------------------------|
| `offset` | `uint32_t`    | Byte offset into the push constant block.        |
| `size`   | `uint32_t`    | Size in bytes (Vulkan min: 128; Metal: ~4 KB).   |
| `stage`  | `ShaderStage` | Which shader stage receives this range.          |

### `ResourceType`

The type of resource bound at a descriptor slot.

```cpp
enum class ResourceType {
    UniformBuffer,         // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER / Metal [[buffer(n)]]
    StorageBuffer,         // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER / Metal [[buffer(n)]]
    SampledImage,          // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE  / Metal [[texture(n)]]
    StorageImage,          // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE  / Metal [[texture(n)]] (read-write)
    Sampler,               // VK_DESCRIPTOR_TYPE_SAMPLER        / Metal [[sampler(n)]]
    AccelerationStructure  // VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR / Metal instance_acceleration_structure [[buffer(n)]]
};
```

`AccelerationStructure` binds a [`RayTracingScene`](RayTracingScene.md) for ray
queries (see `ResourceSet::setRayTracingScene`). On Metal it occupies the
`[[buffer(N)]]` namespace, so it follows the same reserved-slot rules as
UniformBuffer/StorageBuffer (`metal >= 1` in fragment/compute stages).

### `ShaderBinding`

Backend-specific binding indices. Vulkan uses descriptor set + binding; Metal
uses direct argument indices (`[[buffer(N)]]`, `[[texture(N)]]`, `[[sampler(N)]]`).

```cpp
struct ShaderBinding {
    uint32_t vulkan = 0;  // Vulkan descriptor binding index (within a set)
    uint32_t metal  = 0;  // Metal argument index
};
```

| Field    | Type       | Description                                                        |
|----------|------------|--------------------------------------------------------------------|
| `vulkan` | `uint32_t` | Vulkan descriptor binding index (combined with `ResourceBinding::set`). |
| `metal`  | `uint32_t` | Metal `[[buffer(N)]]`, `[[texture(N)]]`, or `[[sampler(N)]]` index. |

For UniformBuffer and StorageBuffer bindings, the Metal index must respect
reserved slots in the `[[buffer(N)]]` namespace:

- **Vertex stage:** `metal >= 2` (0 = vertex buffer, 1 = push constants)
- **Fragment stage:** `metal >= 1` (0 = push constants)
- **Compute stage:** `metal >= 1` (0 = push constants)

These restrictions do **not** apply to SampledImage, StorageImage, or Sampler
bindings, because Metal `[[texture(N)]]` and `[[sampler(N)]]` are independent
namespaces — indices can start at 0.

### `ResourceBinding`

Describes a single slot in a pipeline layout. A collection of these forms the
layout's descriptor-set schema, consumed by `ResourceSet` at creation time.

```cpp
struct ResourceBinding {
    uint32_t      set     = 0;
    ShaderBinding binding = {};
    ResourceType  type    = ResourceType::SampledImage;
    ShaderStage   stage   = ShaderStage::Fragment;
    uint32_t      count   = 1;
};
```

| Field     | Type            | Description                                                  |
|-----------|-----------------|--------------------------------------------------------------|
| `set`     | `uint32_t`      | Descriptor set index (matches GLSL `layout(set=N)`). Ignored in Metal. |
| `binding` | `ShaderBinding` | Backend-specific binding indices (Vulkan + Metal).           |
| `type`    | `ResourceType`  | Kind of resource bound at this slot.                         |
| `stage`   | `ShaderStage`   | Shader stage that reads this binding.                        |
| `count`   | `uint32_t`      | Array size; always 1 in the current implementation.          |

### `PipelineLayoutDescription`

```cpp
struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants;
    std::vector<ResourceBinding>   resourceBindings;
    std::string                    debugName;
};
```

| Field              | Type                           | Description                                     |
|--------------------|--------------------------------|-------------------------------------------------|
| `pushConstants`    | `std::vector<PushConstantRange>` | Push constant ranges (may be empty).          |
| `resourceBindings` | `std::vector<ResourceBinding>` | All descriptor bindings across all sets.        |
| `debugName`        | `std::string`                  | Optional label shown in GPU debug tools.        |

**Example — layout with three sets:**

```cpp
gpu::PipelineLayoutDescription desc{};
// set 0, binding 0 (Vulkan) / buffer(2) (Metal): vertex UBO
desc.resourceBindings.push_back(
    { 0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1 });
// set 1, binding 0 (Vulkan) / buffer(3) (Metal): vertex UBO
desc.resourceBindings.push_back(
    { 1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1 });
// set 2, binding 0 (Vulkan) / texture(0) (Metal): fragment sampled image
desc.resourceBindings.push_back(
    { 2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage,  gpu::ShaderStage::Fragment, 1 });
// set 2, binding 1 (Vulkan) / sampler(0) (Metal): fragment sampler
desc.resourceBindings.push_back(
    { 2, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler,       gpu::ShaderStage::Fragment, 1 });
desc.debugName = "Cube pipeline layout";
auto layout = device->createPipelineLayout(desc);
```

### `ShaderModuleDescription`

See [ShaderModule](ShaderModule.md#shadermoduledescription).

---

## Vertex input types

The following types describe how vertex data is laid out in GPU buffers. They
are consumed by `GraphicsPipelineDescription::addVertexBufferDescription()` and
produced by `gpu::MeshGeneric<T>::vertexBufferDescription()`.

### `Format`

Vertex attribute format.

```cpp
enum class Format {
    Undefined = 0,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT
};
```

| Value                 | Components | Size     | Typical use               |
|-----------------------|------------|----------|---------------------------|
| `Undefined`           | –          | –        | Unset / default.          |
| `R32_SFLOAT`          | 1 × float  | 4 bytes  | Scalar attributes.        |
| `R32G32_SFLOAT`       | 2 × float  | 8 bytes  | UV coordinates, 2D pos.   |
| `R32G32B32_SFLOAT`    | 3 × float  | 12 bytes | 3D position, normal.      |
| `R32G32B32A32_SFLOAT` | 4 × float  | 16 bytes | Color, tangent+handedness.|

### `BufferUsage`

Bitmask describing how a `Buffer` will be used. Supports bitwise `|`, `&`,
and `|=` operators.

```cpp
enum class BufferUsage : uint32_t {
    None                            = 0,
    Vertex                          = 1 << 0,
    Index                           = 1 << 1,
    Uniform                         = 1 << 2,
    Storage                         = 1 << 3,
    TransferSrc                     = 1 << 4,
    TransferDst                     = 1 << 5,
    AccelerationStructureBuildInput = 1 << 6,
    ShaderDeviceAddress             = 1 << 7,
    ShaderBindingTable              = 1 << 8
};
```

`AccelerationStructureBuildInput`, `ShaderDeviceAddress`, and `ShaderBindingTable` mark buffers for
ray tracing usage. The first two feed acceleration structure builds; the third
is used for ray tracing pipeline shader binding tables. On a ray-tracing-enabled
device, vertex and index buffers automatically receive the acceleration-structure
build-input usage so they can be reused directly as [`RayTracingMesh`](RayTracingMesh.md)
geometry — no duplicate buffers are needed. See [Buffer](Buffer.md).

### `VertexSemantic`

Semantic meaning of a vertex attribute, used for documentation and
cross-API mapping.

```cpp
enum class VertexSemantic {
    Position,
    Normal,
    Tangent,
    TexCoord0,
    TexCoord1,
    Color
};
```

### `VertexInputRate`

Controls whether an attribute advances per vertex or per instance.

```cpp
enum class VertexInputRate {
    Vertex,
    Instance
};
```

### `VertexAttributeDescription`

Describes a single vertex attribute within a vertex buffer binding.

```cpp
struct VertexAttributeDescription {
    uint32_t       location = 0;
    uint32_t       binding  = 0;
    VertexSemantic semantic;
    Format         format;
    uint32_t       offset   = 0;
};
```

| Field      | Type             | Description                                         |
|------------|------------------|-----------------------------------------------------|
| `location` | `uint32_t`       | Shader input location (`layout(location = N) in`). |
| `binding`  | `uint32_t`       | Vertex buffer binding slot.                         |
| `semantic` | `VertexSemantic` | Semantic hint (position, normal, UV, …).           |
| `format`   | `Format`         | Data type and component count.                      |
| `offset`   | `uint32_t`       | Byte offset from the start of a vertex struct.     |

### `VertexBufferDescription`

Describes a complete vertex buffer binding: stride, input rate, and all
attributes it contains.

```cpp
struct VertexBufferDescription {
    uint32_t                                binding   = 0;
    uint32_t                                stride    = 0;
    VertexInputRate                         inputRate = VertexInputRate::Vertex;
    std::vector<VertexAttributeDescription> attributes;
};
```

| Field        | Type                                      | Description                                  |
|--------------|-------------------------------------------|----------------------------------------------|
| `binding`    | `uint32_t`                                | Buffer binding slot index.                   |
| `stride`     | `uint32_t`                                | Byte stride between successive vertex entries.|
| `inputRate`  | `VertexInputRate`                         | Per-vertex or per-instance stepping.         |
| `attributes` | `std::vector<VertexAttributeDescription>` | All attributes carried by this buffer.       |

Prefer constructing this via `gpu::MeshGeneric<T>::vertexBufferDescription()`
rather than filling it by hand.
