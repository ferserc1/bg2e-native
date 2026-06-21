# CleanupManager

**Header:** `<bg2e/gpu/CleanupManager.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API CleanupManager {
public:
    explicit CleanupManager(gpu::Surface* surface);
    ~CleanupManager() = default;

    CleanupManager(const CleanupManager&) = delete;
    CleanupManager& operator=(const CleanupManager&) = delete;

    // Ordered cleanup
    void push(const std::shared_ptr<DeviceResource>& resource);
    void push(std::shared_ptr<DeviceResource>&& resource);
    void pushStatic(const std::shared_ptr<DeviceResource>& resource);
    void pushStatic(std::shared_ptr<DeviceResource>&& resource);
    void flush();
    void clear();
    bool empty() const;

    // Deferred cleanup
    void defer(std::function<void()>&& cleanup);
    void flushDeferred();
    void flushAllDeferred();
};
```

Provides ordered cleanup for groups of `DeviceResource` objects and deferred
cleanup closures whose execution is tied to the surface frame counter.

---

## Construction

### `explicit CleanupManager(gpu::Surface* surface)`

Creates a cleanup manager tied to the given surface. The surface pointer is
**not owned** — the caller must ensure the surface outlives the manager.

The surface provides the frame counter and in-flight frame count used by the
deferred cleanup API.

---

## Ordered cleanup

### `void push(const std::shared_ptr<DeviceResource>& resource)`
### `void push(std::shared_ptr<DeviceResource>&& resource)`

Registers a device resource for cleanup. Normal resources are cleaned in
**reverse** insertion order when `flush()` is called.

**Null-safe:** Both overloads silently ignore `nullptr` resources (no-op).
This allows backend-specific factory methods such as `ShaderLib::miss()` or
`ShaderLib::closestHit()` to return `nullptr` on platforms where a shader
stage is not applicable, without requiring callers to add null-checks before
registration.

### `void pushStatic(const std::shared_ptr<DeviceResource>& resource)`
### `void pushStatic(std::shared_ptr<DeviceResource>&& resource)`

Registers a device resource as a static (long-lived) resource. Static resources
are cleaned **first**, in **insertion** order when `flush()` is called.

Use this for engine-level shared resources such as texture caches, geometry
caches, or global material resources that should be released before ordinary
per-object resources.

**Null-safe:** Both overloads silently ignore `nullptr` resources (no-op).

### `void flush()`

Executes all registered cleanups in the following order:

1. Static resources — insertion order.
2. Normal resources — reverse insertion order.
3. Clears all stored `shared_ptr` references.

### `void clear()`

Releases all stored `shared_ptr` references without calling `cleanup()` on them.

### `bool empty() const`

Returns `true` if both the normal and static queues are empty.

---

## Deferred cleanup

### `void defer(std::function<void()>&& cleanup)`

Schedules a cleanup closure for deferred execution. The closure will run after
`inFlightFrames()` frames have elapsed, specifically when `flushDeferred()` is
called and `surface->frameCounter() >= targetFrame`.

`targetFrame` is computed as:

```
targetFrame = surface->frameCounter() + surface->inFlightFrames()
```

The closure is stored as a `std::function<void()>`. It can capture
`shared_ptr<DeviceResource>` values by move to extend resource lifetime until
the closure executes.

Typical usage for safe GPU buffer destruction:

```cpp
auto oldMesh = std::move(gpuMesh);

cleanup.defer([oldMesh = std::move(oldMesh)]() mutable {
    oldMesh.cleanup();
});
```

### `void flushDeferred()`

Executes all deferred closures whose `targetFrame <= surface->frameCounter()`.

Call this **after** `surface->endFrame()` in the render loop, after the fence
has been waited:

```cpp
surface->endFrame(frame.get());
cleanup.flushDeferred();
```

### `void flushAllDeferred()`

Executes **all** pending deferred closures immediately, regardless of frame
counter. Call this after `device->waitIdle()` at application shutdown:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();
cleanup.flush();
surface->cleanup();
device->cleanup();
instance->cleanup();
```

---

## Render loop integration

The deferred cleanup API is designed to be called in a specific order within the
render loop:

```cpp
while (running)
{
    // 1. Handle events (resize calls waitIdle + flushAllDeferred)
    // ...

    // 2. Acquire frame (waits fence internally)
    auto frame = surface->beginFrame();

    // 3. Record and submit commands
    auto cmd = graphicsQueue.createCommandBuffer("Frame");
    cmd->begin();
    // ... rendering ...
    cmd->end();
    graphicsQueue.submit(cmd.get());

    // 4. Present and increment frame counter
    surface->endFrame(frame.get());

    // 5. Flush deferred cleanups AFTER endFrame()
    cleanup.flushDeferred();
}
```

On application exit:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();   // run all remaining deferred closures
cleanup.flush();               // run ordered device resource cleanup
surface->cleanup();
device->cleanup();
instance->cleanup();
```

On window resize:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();   // drain all pending deferred closures
surface->resize(newSize);
```

---

## Thread safety

`CleanupManager` is not thread-safe. All methods must be called from the same
thread — typically the main render loop thread. All access is sequential within
the render loop, so no synchronization is needed.

---

## Design notes

- The `Surface*` is non-owning. The surface is typically held by a
  `std::shared_ptr` in the caller's scope.
- Deferred closures use the `erase-remove` idiom for execution and removal.
- `flushDeferred()` is O(n) where n is the number of pending deferred closures.
- The frame counter is a monotonically increasing `uint64_t` — overflow is not a
  practical concern (584 billion years at 60 fps).
