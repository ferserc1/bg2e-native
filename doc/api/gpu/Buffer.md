# Buffer

**Header:** `<bg2e/gpu/Buffer.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Buffer {
public:
    virtual ~Buffer() = default;

    virtual void cleanup() = 0;
    virtual bool isValid() const = 0;

    uint64_t    byteSize() const;
    BufferUsage usage()    const;

    virtual void createVertexBuffer(const void* data, uint64_t byteSize);
    virtual void createIndexBuffer(const std::vector<uint32_t>& indices);

    template <typename VertexT>
    void createVertexBuffer(const std::vector<VertexT>& vertices);
};
```

Abstract GPU buffer. Manages a device-local allocation and an internal staging
buffer — the caller never creates a staging buffer manually. Created via
`Device::createBuffer()`.

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

In practice, `gpu::MeshGeneric<T>` encapsulates these buffers and exposes
`build()`, `draw()`, and `cleanup()` at a higher level.

---

## vk::Buffer

**Header:** `<bg2e/gpu/vk/Buffer.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Buffer`

```cpp
class Buffer : public gpu::Buffer {
public:
    Buffer(vk::Device* device, const std::string& debugName = {});
    ~Buffer() override;

    void cleanup()  override;
    bool isValid()  const override;

    void createVertexBuffer(const void* data, uint64_t byteSize) override;
    void createIndexBuffer(const std::vector<uint32_t>& indices)  override;

    VkBuffer handle() const;
};
```

Vulkan buffer backed by a VMA allocation. Both `createVertexBuffer` and
`createIndexBuffer` create a `VMA_MEMORY_USAGE_CPU_TO_GPU` staging buffer,
copy the data into it, then perform a `vkCmdCopyBuffer` into a
`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` buffer via `Device::immediateSubmit`.

### `VkBuffer handle() const`

Returns the raw `VkBuffer` handle of the device-local buffer.

---

## metal::Buffer

**Header:** `<bg2e/gpu/metal/Buffer.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Buffer`

```cpp
class Buffer : public gpu::Buffer {
public:
    Buffer(metal::Device* device, const std::string& debugName = {});
    ~Buffer() override;

    void cleanup()  override;
    bool isValid()  const override;

    void createVertexBuffer(const void* data, uint64_t byteSize) override;
    void createIndexBuffer(const std::vector<uint32_t>& indices)  override;

    MTL::Buffer* handle() const;  // macOS only
};
```

Metal buffer backed by a `MTL::ResourceStorageModePrivate` (device-local)
allocation. Data is uploaded via a `MTL::ResourceStorageModeShared` staging
buffer and a blit encoder submitted through `Device::immediateSubmit`.

### `MTL::Buffer* handle() const`

Returns the raw `MTL::Buffer*` handle (macOS only).
