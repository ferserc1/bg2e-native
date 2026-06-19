# RayTracingMesh

**Header:** `<bg2e/gpu/RayTracingMesh.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API RayTracingMesh : public DeviceResource {
public:
    explicit RayTracingMesh(Device* device);
    ~RayTracingMesh() override = default;

    const RayTracingMeshDescription& description() const;

    virtual bool isBuilt() const = 0;   // true once built on the GPU

    // From DeviceResource
    virtual bool isValid() const = 0;
    virtual void cleanup()       = 0;
};
```

`RayTracingMesh` is the **bottom-level acceleration structure** (BLAS) for a
single submesh. Conceptually:

```
RayTracingMesh  = bottom-level acceleration data for one submesh
RayTracingScene = top-level acceleration data containing instances of RayTracingMesh
```

Each submesh gets its own `RayTracingMesh`. This is intentional: submeshes in a
drawable can carry their own local transform, so a single acceleration
structure for a whole mesh would not represent the drawable model correctly.

A `RayTracingMesh` is built **from existing GPU buffers** — the same vertex and
index buffers already uploaded for rasterization (see
[`MeshGeneric::rayTracingMeshDescription`](Mesh.md)). It does **not** own those
buffers and does not duplicate them; the caller must keep them alive while the
`RayTracingMesh` is in use.

Ray tracing requires hardware support. Query it before creating any acceleration
structure:

```cpp
if (!physicalDevice->properties()->rayTracingSupported()) {
    // ray queries / acceleration structures are not available
}
```

Created via [`Device::createRayTracingMesh()`](Device.md). The acceleration
structure storage and scratch buffers are allocated at creation time, but the
actual build is a GPU command — record it with
[`CommandBuffer::buildRayTracingMesh()`](CommandBuffer.md), typically inside a
`Device::immediateSubmit`.

---

## RayTracingMeshDescription

Describes the geometry of one submesh, referencing GPU buffers that already
exist. Prefer building it with
[`gpu::MeshGeneric<T>::rayTracingMeshDescription(submeshIndex)`](Mesh.md), which
resolves the vertex stride, position offset and submesh index range
automatically.

```cpp
struct RayTracingMeshDescription {
    gpu::Buffer* vertexBuffer = nullptr;   // interleaved vertex buffer (shared with rasterization)
    gpu::Buffer* indexBuffer  = nullptr;   // 32-bit index buffer (shared with rasterization)

    uint32_t vertexCount    = 0;           // number of vertices in the vertex buffer
    uint32_t vertexStride   = 0;           // bytes between consecutive vertices
    uint32_t positionOffset = 0;           // byte offset of the position attribute
    Format   vertexFormat   = Format::R32G32B32_SFLOAT;   // position attribute format

    uint32_t firstIndex = 0;               // submesh start index into indexBuffer
    uint32_t indexCount = 0;               // submesh index count (must be a multiple of 3)

    std::string debugName;
};
```

| Field            | Type          | Description                                              |
|------------------|---------------|----------------------------------------------------------|
| `vertexBuffer`   | `gpu::Buffer*`| Interleaved vertex buffer, shared with the raster mesh.  |
| `indexBuffer`    | `gpu::Buffer*`| 32-bit index buffer, shared with the raster mesh.        |
| `vertexCount`    | `uint32_t`    | Number of vertices in `vertexBuffer`.                    |
| `vertexStride`   | `uint32_t`    | Byte stride between successive vertices.                 |
| `positionOffset` | `uint32_t`    | Byte offset of the position attribute within a vertex.   |
| `vertexFormat`   | `Format`      | Position attribute format (normally `R32G32B32_SFLOAT`). |
| `firstIndex`     | `uint32_t`    | First index of the submesh inside `indexBuffer`.         |
| `indexCount`     | `uint32_t`    | Index count of the submesh (a non-zero multiple of 3).   |
| `debugName`      | `std::string` | Optional label for GPU debug tools.                      |

---

## Methods

### `const RayTracingMeshDescription& description() const`

Returns the description the mesh was created from.

### `virtual bool isBuilt() const = 0`

Returns `true` once the acceleration structure has been built on the GPU (i.e.
after `CommandBuffer::buildRayTracingMesh()` has been recorded and submitted).
`RayTracingScene::buildOrUpdate()` requires every referenced `RayTracingMesh`
to be built.

### `virtual bool isValid() const = 0`

Returns `true` once the backend acceleration structure object has been created.

### `virtual void cleanup() = 0`

Releases the acceleration structure and its internal buffers. Must be called
before `Device::cleanup()`.

---

## Usage

```cpp
// Reuse the vertex/index buffers already uploaded by gpu::MeshPN::build().
std::vector<std::shared_ptr<gpu::RayTracingMesh>> rtMeshes;
for (uint32_t s = 0; s < mesh.submeshCount(); ++s) {
    rtMeshes.push_back(
        device->createRayTracingMesh(mesh.rayTracingMeshDescription(s)));
}

// Build the bottom-level acceleration structures on the GPU (one submit).
device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
    for (auto& rtMesh : rtMeshes) {
        cmd->buildRayTracingMesh(rtMesh.get());
    }
});
```

See `examples/gpu/11_ray_query_shadows` for a complete ray-query shadows
example.

---

## vk::RayTracingMesh

**Header:** `<bg2e/gpu/vk/RayTracingMesh.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::RayTracingMesh`

```cpp
class RayTracingMesh : public gpu::RayTracingMesh {
public:
    RayTracingMesh(vk::Device* device, const RayTracingMeshDescription& description);

    void cleanup() override;
    bool isValid() const override;
    bool isBuilt() const override;

    void build(VkCommandBuffer cmd);   // records vkCmdBuildAccelerationStructuresKHR

    VkAccelerationStructureKHR handle() const;
    VkDeviceAddress            deviceAddress() const;
};
```

Vulkan bottom-level acceleration structure
(`VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR`). At construction it queries
`vkGetAccelerationStructureBuildSizesKHR`, allocates the acceleration structure
storage buffer (`VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR`) and a
scratch buffer, and creates the structure with `vkCreateAccelerationStructureKHR`.
`build()` records `vkCmdBuildAccelerationStructuresKHR`. The submesh range is
applied through `VkAccelerationStructureBuildRangeInfoKHR::primitiveOffset`.

The vertex/index device addresses come from `vk::Buffer::deviceAddress()`; those
buffers must have been created on a ray-tracing-enabled device (vertex/index
buffers automatically receive
`VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR` in that
case — see [Buffer](Buffer.md)).

### Vulkan-specific methods

#### `VkAccelerationStructureKHR handle() const`

Returns the raw acceleration structure handle.

#### `VkDeviceAddress deviceAddress() const`

Returns the device address of the BLAS, used by `vk::RayTracingScene` to fill
the instance `accelerationStructureReference`.

---

## metal::RayTracingMesh

**Header:** `<bg2e/gpu/metal/RayTracingMesh.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::RayTracingMesh`

```cpp
class RayTracingMesh : public gpu::RayTracingMesh {
public:
    RayTracingMesh(metal::Device* device, const RayTracingMeshDescription& description);

    void cleanup() override;
    bool isValid() const override;
    bool isBuilt() const override;

    void build(MTL::CommandBuffer* cmd);   // records the build on an acceleration structure encoder

    MTL::AccelerationStructure* handle() const;   // macOS only
};
```

Metal primitive acceleration structure. The build descriptor is a
`MTL::PrimitiveAccelerationStructureDescriptor` with a single
`MTL::AccelerationStructureTriangleGeometryDescriptor` configured from the
existing `MTL::Buffer` objects (`vertexBuffer`, `vertexBufferOffset`,
`vertexStride`, `indexBuffer`, `indexBufferOffset`, `indexType`, triangle
count). Sizes come from `MTL::Device::accelerationStructureSizes`; the
structure and its scratch buffer are created lazily on the first `build()` and
reused afterward. `build()` records the build through a
`MTL::AccelerationStructureCommandEncoder`.

### Metal-specific methods

#### `MTL::AccelerationStructure* handle() const`

Returns the raw primitive acceleration structure (macOS only). Referenced by
`metal::RayTracingScene` both in the instance descriptor and for `useResource`
residency when the scene is used in a shader.
