# FrameResourceRing

**Header:** `<bg2e/gpu/FrameResourceRing.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
template <typename T>
class FrameResourceRing {
    static_assert(std::is_base_of_v<DeviceResource, T>,
                  "FrameResourceRing<T>: T must derive from DeviceResource");
public:
    FrameResourceRing() = default;
    ~FrameResourceRing() = default;

    FrameResourceRing(const FrameResourceRing&) = delete;
    FrameResourceRing& operator=(const FrameResourceRing&) = delete;

    void create(gpu::Surface* surface,
                std::function<std::shared_ptr<T>(uint32_t)> factory);

    T* current();
    const T* current() const;
    std::shared_ptr<T> currentShared();
    std::shared_ptr<const T> currentShared() const;

    T* at(uint32_t index);
    const T* at(uint32_t index) const;
    std::shared_ptr<T> sharedAt(uint32_t index);
    std::shared_ptr<const T> sharedAt(uint32_t index) const;

    uint32_t size() const;
    bool empty() const;

    void cleanup();
    void clear();
};
```

A template container that holds one `DeviceResource` per in-flight frame slot.
Useful for resources that are updated every frame, such as uniform buffers
containing model matrices or per-frame scene data.

Instead of creating and destroying a resource every frame, the ring keeps
several persistent copies indexed by the current frame.

---

## Type requirements

`T` must derive from `DeviceResource`. The `static_assert` enforces this at
compile time.

Common instantiations:

```cpp
gpu::FrameResourceRing<gpu::Buffer>      // per-frame uniform buffers
gpu::FrameResourceRing<gpu::ResourceSet> // per-frame descriptor sets
```

---

## Creation

### `void create(gpu::Surface* surface, std::function<std::shared_ptr<T>(uint32_t)> factory)`

Creates the ring with `surface->inFlightFrames()` slots. For each slot index
`i`, the factory is called to produce the resource.

| Parameter | Type | Description |
|-----------|------|-------------|
| `surface` | `gpu::Surface*` | Surface that provides `inFlightFrames()` and `currentFrameIndex()`. |
| `factory` | `std::function<...>` | Callable that creates a resource for a given slot index. |

The ring size matches the number of concurrent frames, not the number of
swapchain images. For window surfaces, `inFlightFrames()` returns 2 (two
copies); for offscreen surfaces, it returns 1 (one copy).

Example:

```cpp
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});
```

---

## Access

### `T* current()`

Returns the resource for the current frame slot. Uses
`surface->currentFrameIndex()` to select the slot.

### `std::shared_ptr<T> currentShared()`

Returns a `shared_ptr` to the resource for the current frame slot.

### `T* at(uint32_t index)`

Returns the resource at the given slot index.

### `std::shared_ptr<T> sharedAt(uint32_t index)`

Returns a `shared_ptr` to the resource at the given slot index. Useful for
passing a resource into a deferred cleanup closure or a resource set.

### `uint32_t size()`

Returns the number of slots in the ring.

### `bool empty()`

Returns `true` if the ring has no slots.

---

## Cleanup

### `void cleanup()`

Calls `cleanup()` on all resources in **reverse** order and releases the
`shared_ptr` references.

### `void clear()`

Releases all stored `shared_ptr` references without calling `cleanup()`.

---

## Relationship with Surface

The ring depends on two `Surface` methods:

- `inFlightFrames()` — determines the ring size at creation time.
- `currentFrameIndex()` — determines which slot is current each frame.

For Vulkan window surfaces, `inFlightFrames()` returns 2 and
`currentFrameIndex()` cycles through `0..1`. For offscreen surfaces,
`inFlightFrames()` returns 1 and `currentFrameIndex()` always returns `0`.

---

## Example usage

```cpp
// Create per-frame uniform buffer ring
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer("Model UBO [" + std::to_string(i) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

// Create per-frame resource set ring
gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(layout.get(), 1,
        "Model set [" + std::to_string(i) + "]");
    set->setUniformBuffer(binding, modelUboRing.sharedAt(i));
    set->update();
    return set;
});

// In the render loop
auto* modelUbo = modelUboRing.current();
modelUbo->updateUniformBuffer(modelData);
auto* modelSet = modelSetRing.current();

cmd->bindResourceSet(pipeline.get(), 1, modelSet);

// At shutdown
modelSetRing.cleanup();
modelUboRing.cleanup();
```
