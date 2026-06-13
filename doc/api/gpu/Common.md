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
    Compute
};
```

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

### `PipelineLayoutDescription`

```cpp
struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants;
};
```

| Field           | Type                         | Description             |
|-----------------|------------------------------|-------------------------|
| `pushConstants` | `std::vector<PushConstantRange>` | Push constant ranges.|

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
    ShaderDeviceAddress             = 1 << 7
};
```

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
