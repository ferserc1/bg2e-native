# Buffer

**Header:** `<bg2e/gpu/Buffer.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Buffer : public DeviceResource {
public:
    uint64_t    byteSize() const;
    BufferUsage usage()    const;

    // Device-local buffers (staging upload path)
    virtual void createVertexBuffer(const void* data, uint64_t byteSize);
    virtual void createIndexBuffer(const std::vector<uint32_t>& indices);
    template <typename VertexT>
    void createVertexBuffer(const std::vector<VertexT>& vertices);

    // Host-visible buffers (direct CPU-write path)
    virtual void createUniformBuffer(const void* data, uint64_t byteSize);
    virtual void createStorageBuffer(const void* data, uint64_t byteSize);
    virtual void updateUniformBuffer(const void* data, uint64_t byteSize);
    virtual void updateStorageBuffer(const void* data, uint64_t byteSize);
    template <typename T> void createUniformBuffer(const T& data);
    template <typename T> void createStorageBuffer(const std::vector<T>& data);
    template <typename T> void updateUniformBuffer(const T& data);
    template <typename T> void updateStorageBuffer(const std::vector<T>& data);
};
```

Abstract GPU buffer. `Buffer` exposes two allocation strategies determined by
the create method called:

- **Device-local** (`createVertexBuffer`, `createIndexBuffer`): data is uploaded
  via an internal staging buffer into device-local GPU memory. Optimal for
  static geometry that is read many times.
- **Host-visible** (`createUniformBuffer`, `createStorageBuffer`): the backend
  allocates memory that is directly writable by the CPU (no staging). Used for
  small, frequently updated data such as per-frame transformation matrices.

Created via `Device::createBuffer()`.

---

## Methods

### `virtual void cleanup() = 0`

Releases the device-local buffer allocation. Must be called before
`Device::cleanup()`.

### `virtual bool isValid() const = 0`

Returns `true` if the buffer has been successfully allocated (i.e.
`createVertexBuffer` or `createIndexBuffer` has been called).

### `uint64_t byteSize() const`

Returns the size of the buffer in bytes (set when the buffer is created).

### `BufferUsage usage() const`

Returns the `BufferUsage` flags set when the buffer was created.

### `virtual void createVertexBuffer(const void* data, uint64_t byteSize)`

Uploads raw vertex data from CPU memory to a device-local GPU buffer.
Internally creates a CPU-visible staging buffer, copies the data into it,
then submits a transfer command via `Device::immediateSubmit`.

| Parameter  | Type           | Description                           |
|------------|----------------|---------------------------------------|
| `data`     | `const void*`  | Pointer to vertex data.               |
| `byteSize` | `uint64_t`     | Total size of the data in bytes.      |

### `template <typename VertexT> void createVertexBuffer(const std::vector<VertexT>& vertices)`

Convenience overload that derives `data` and `byteSize` from the vector.
Equivalent to `createVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexT))`.

### `virtual void createIndexBuffer(const std::vector<uint32_t>& indices)`

Uploads 32-bit index data to a device-local GPU buffer. Indices are always
`uint32_t`; there is no 16-bit variant.

| Parameter | Type                         | Description              |
|-----------|------------------------------|--------------------------|
| `indices` | `const std::vector<uint32_t>&` | Index data to upload.  |

---

## Uniform and storage buffer methods

These methods create a **host-visible, CPU-writable** buffer. The backend
picks the appropriate memory strategy automatically (persistently mapped on
Vulkan, shared storage on Metal). They are intended for data that changes
every frame, such as transformation matrices.

### `virtual void createUniformBuffer(const void* data, uint64_t byteSize)`

Allocates a host-visible buffer suitable for use as a uniform buffer (UBO).
The initial contents are copied from `data` if non-null.

| Parameter  | Type          | Description                          |
|------------|---------------|--------------------------------------|
| `data`     | `const void*` | Optional initial data (may be null). |
| `byteSize` | `uint64_t`    | Buffer size in bytes.                |

### `template <typename T> void createUniformBuffer(const T& data)`

Convenience overload. Equivalent to `createUniformBuffer(&data, sizeof(T))`.

### `virtual void createStorageBuffer(const void* data, uint64_t byteSize)`

Allocates a host-visible buffer suitable for use as a storage buffer (SSBO).

| Parameter  | Type          | Description                          |
|------------|---------------|--------------------------------------|
| `data`     | `const void*` | Optional initial data (may be null). |
| `byteSize` | `uint64_t`    | Buffer size in bytes.                |

### `template <typename T> void createStorageBuffer(const std::vector<T>& data)`

Convenience overload. Equivalent to
`createStorageBuffer(data.data(), data.size() * sizeof(T))`.

### `virtual void updateUniformBuffer(const void* data, uint64_t byteSize)`

Overwrites the contents of a buffer previously created with
`createUniformBuffer`. The new data must not exceed the original byte size.
Intended for per-frame updates — typically called once per frame on a buffer
that is part of a `FrameResourceRing<gpu::Buffer>`.

| Parameter  | Type          | Description                          |
|------------|---------------|--------------------------------------|
| `data`     | `const void*` | New data to write.                   |
| `byteSize` | `uint64_t`    | Must be ≤ the original buffer size.  |

### `template <typename T> void updateUniformBuffer(const T& data)`

Convenience overload. Equivalent to `updateUniformBuffer(&data, sizeof(T))`.

### `virtual void updateStorageBuffer(const void* data, uint64_t byteSize)`

Overwrites the contents of a buffer previously created with
`createStorageBuffer`.

### `template <typename T> void updateStorageBuffer(const std::vector<T>& data)`

Convenience overload. Equivalent to
`updateStorageBuffer(data.data(), data.size() * sizeof(T))`.

---

## Usage pattern

```cpp
// Allocate
auto vertexBuf = device->createBuffer("My vertex buffer");
auto indexBuf  = device->createBuffer("My index buffer");

// Upload (staging is internal)
vertexBuf->createVertexBuffer(myVertices);
indexBuf->createIndexBuffer(myIndices);

// Bind in a command buffer
cmd->bindVertexBuffer(0, vertexBuf.get());
cmd->bindIndexBuffer(indexBuf.get());
cmd->drawIndexed(static_cast<uint32_t>(myIndices.size()));

// Cleanup (before device->cleanup())
device->waitIdle();
vertexBuf->cleanup();
indexBuf->cleanup();
```

In practice, `gpu::MeshGeneric<T>` encapsulates vertex and index buffers and
exposes `build()`, `draw()`, and `cleanup()` at a higher level.

### Uniform buffer with per-frame ring

```cpp
struct ModelUBO { glm::mat4 model; };

// Create one buffer per swapchain frame
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buf = device->createBuffer("Model UBO[" + std::to_string(i) + "]");
    buf->createUniformBuffer(ModelUBO{});
    return buf;
});

// Per-frame update (no staging, no GPU sync required)
auto* ubo = modelUboRing.current();
ubo->updateUniformBuffer(ModelUBO{ glm::rotate(glm::mat4(1.f), t, glm::vec3(0,1,0)) });

// Cleanup
modelUboRing.cleanup();
```

---

## vk::Buffer

**Header:** `<bg2e/gpu/vk/Buffer.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Buffer`

```cpp
class Buffer : public gpu::Buffer {
public:
    void cleanup()  override;
    bool isValid()  const override;

    void createVertexBuffer(const void* data, uint64_t byteSize) override;
    void createIndexBuffer(const std::vector<uint32_t>& indices)  override;
    void createUniformBuffer(const void* data, uint64_t byteSize) override;
    void createStorageBuffer(const void* data, uint64_t byteSize) override;
    void updateUniformBuffer(const void* data, uint64_t byteSize) override;
    void updateStorageBuffer(const void* data, uint64_t byteSize) override;

    VkBuffer        handle() const;
    VkDeviceAddress deviceAddress() const;
    uint64_t        byteSize() const;
};
```

Vulkan buffer backed by a VMA allocation.

- **Vertex / index buffers** use a `VMA_MEMORY_USAGE_CPU_TO_GPU` staging
  buffer, copy the data into it, then `vkCmdCopyBuffer` to a device-local
  buffer via `Device::immediateSubmit`. They are always created with
  `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`; on a ray-tracing-enabled device
  they additionally receive
  `VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR`, so the
  same buffers can be reused as [`RayTracingMesh`](RayTracingMesh.md) geometry.
- **Uniform / storage buffers** use
  `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |`
  `VMA_ALLOCATION_CREATE_MAPPED_BIT` so that `updateUniformBuffer` /
  `updateStorageBuffer` can `memcpy` directly into the persistently-mapped
  pointer without staging or synchronization overhead.

### `VkBuffer handle() const`

Returns the raw `VkBuffer` handle of the device-local buffer.

### `VkDeviceAddress deviceAddress() const`

Returns the buffer's GPU device address (via `vkGetBufferDeviceAddress`), or `0`
if the buffer is not allocated. Used as acceleration structure build input for
[`RayTracingMesh`](RayTracingMesh.md).

---

## metal::Buffer

**Header:** `<bg2e/gpu/metal/Buffer.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Buffer`

```cpp
class Buffer : public gpu::Buffer {
public:
    void cleanup()  override;
    bool isValid()  const override;

    void createVertexBuffer(const void* data, uint64_t byteSize) override;
    void createIndexBuffer(const std::vector<uint32_t>& indices)  override;
    void createUniformBuffer(const void* data, uint64_t byteSize) override;
    void createStorageBuffer(const void* data, uint64_t byteSize) override;
    void updateUniformBuffer(const void* data, uint64_t byteSize) override;
    void updateStorageBuffer(const void* data, uint64_t byteSize) override;

    MTL::Buffer* handle() const;  // macOS only
};
```

- **Vertex / index buffers** use `MTL::ResourceStorageModePrivate`
  (device-local). Data is uploaded via a `MTL::ResourceStorageModeShared`
  staging buffer and a blit encoder submitted through `Device::immediateSubmit`.
- **Uniform / storage buffers** use `MTL::ResourceStorageModeShared` so that
  `updateUniformBuffer` / `updateStorageBuffer` can `memcpy` into
  `MTL::Buffer::contents()` directly.

### `MTL::Buffer* handle() const`

Returns the raw `MTL::Buffer*` handle (macOS only). Used by
`metal::ResourceSet` and `metal::CommandBuffer` to bind the buffer at the
correct `[[buffer(n)]]` index.
