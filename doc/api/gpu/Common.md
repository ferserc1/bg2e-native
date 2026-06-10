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
