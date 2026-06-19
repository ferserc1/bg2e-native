# Step 07: CleanupManager Deferred API

## Files to Modify

- `lib/include/bg2e/gpu/CleanupManager.hpp`
- `lib/src/bg2e/gpu/CleanupManager.cpp`

## Interface Changes

### Header (`CleanupManager.hpp`)

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/Surface.hpp>

#include <deque>
#include <functional>
#include <memory>
#include <vector>

namespace bg2e {
namespace gpu {

class BG2E_API CleanupManager {
public:
    // Constructor now requires a Surface pointer for deferred cleanup timing.
    // The surface is NOT owned — the caller must ensure it outlives the manager.
    explicit CleanupManager(gpu::Surface* surface);
    ~CleanupManager() = default;

    CleanupManager(const CleanupManager&) = delete;
    CleanupManager& operator=(const CleanupManager&) = delete;

    // --- Existing API (unchanged) ---

    void push(const std::shared_ptr<DeviceResource>& resource);
    void push(std::shared_ptr<DeviceResource>&& resource);

    void pushStatic(const std::shared_ptr<DeviceResource>& resource);
    void pushStatic(std::shared_ptr<DeviceResource>&& resource);

    void flush();
    void clear();
    bool empty() const;

    // --- Deferred cleanup API (new) ---

    // Schedule a cleanup closure to run after inFlightFrames() frames have elapsed.
    // The closure will be executed when flushDeferred() is called and
    // surface->frameCounter() >= targetFrame.
    void defer(std::function<void()>&& cleanup);

    // Execute all deferred closures whose targetFrame <= surface->frameCounter().
    // Call this AFTER endFrame() in the render loop (i.e., after the fence).
    void flushDeferred();

    // Execute ALL pending deferred closures immediately, regardless of frame counter.
    // Call this after device->waitIdle() at application shutdown.
    void flushAllDeferred();

private:
    gpu::Surface* _surface;

    std::deque<std::shared_ptr<DeviceResource>> _staticResources;
    std::deque<std::shared_ptr<DeviceResource>> _resources;

    struct DeferredCleanup {
        uint64_t targetFrame;
        std::function<void()> cleanup;
    };
    std::vector<DeferredCleanup> _deferredCleanups;
};

}
}
```

### Source (`CleanupManager.cpp`)

```cpp
#include <bg2e/gpu/CleanupManager.hpp>

namespace bg2e {
namespace gpu {

CleanupManager::CleanupManager(gpu::Surface* surface)
    : _surface(surface)
{
}

void CleanupManager::push(const std::shared_ptr<DeviceResource>& resource)
{
    _resources.push_back(resource);
}

void CleanupManager::push(std::shared_ptr<DeviceResource>&& resource)
{
    _resources.push_back(std::move(resource));
}

void CleanupManager::pushStatic(const std::shared_ptr<DeviceResource>& resource)
{
    _staticResources.push_back(resource);
}

void CleanupManager::pushStatic(std::shared_ptr<DeviceResource>&& resource)
{
    _staticResources.push_back(std::move(resource));
}

void CleanupManager::flush()
{
    // Static resources first, in insertion order
    for (auto& resource : _staticResources)
    {
        if (resource)
        {
            resource->cleanup();
        }
    }
    _staticResources.clear();

    // Normal resources after, in reverse insertion order
    for (auto it = _resources.rbegin(); it != _resources.rend(); ++it)
    {
        if (*it)
        {
            (*it)->cleanup();
        }
    }
    _resources.clear();
}

void CleanupManager::clear()
{
    _staticResources.clear();
    _resources.clear();
}

bool CleanupManager::empty() const
{
    return _staticResources.empty() && _resources.empty();
}

void CleanupManager::defer(std::function<void()>&& cleanup)
{
    _deferredCleanups.push_back({
        _surface->frameCounter() + _surface->inFlightFrames(),
        std::move(cleanup)
    });
}

void CleanupManager::flushDeferred()
{
    auto counter = _surface->frameCounter();
    _deferredCleanups.erase(
        std::remove_if(_deferredCleanups.begin(), _deferredCleanups.end(),
            [counter](const DeferredCleanup& d) {
                if (d.targetFrame <= counter) {
                    d.cleanup();
                    return true;
                }
                return false;
            }),
        _deferredCleanups.end()
    );
}

void CleanupManager::flushAllDeferred()
{
    for (auto& d : _deferredCleanups)
    {
        d.cleanup();
    }
    _deferredCleanups.clear();
}

}
}
```

## Implementation Details

- **Constructor change**: `CleanupManager` now requires a `gpu::Surface*` parameter. The default constructor is removed.
- **`defer()`**: Computes `targetFrame = surface->frameCounter() + surface->inFlightFrames()` and stores the closure
- **`flushDeferred()`**: Uses `std::remove_if` pattern (same as `render::Engine::flushDeferredExec`) to execute and remove expired closures
- **`flushAllDeferred()`**: Executes all pending closures unconditionally — for shutdown
- **Existing API**: `push()`, `pushStatic()`, `flush()`, `clear()`, `empty()` are unchanged

## Integration Points

### Render loop (after fence):
```cpp
auto frame = surface->beginFrame();    // waits fence
// ... render ...
surface->endFrame(frame.get());        // ++_frameCounter
cleanupManager.flushDeferred();        // execute expired closures
```

### Application shutdown (after waitIdle):
```cpp
device->waitIdle();
cleanupManager.flushAllDeferred();     // execute all pending
cleanup.flush();                       // existing resource cleanup
surface->cleanup();
```

## Notes

- The `Surface*` is non-owning — the caller manages its lifetime
- Closures are stored as `std::function<void()>`, which can capture `shared_ptr`s by value to extend resource lifetime
- The `erase-remove` pattern is the same used in `render::Engine::flushDeferredExec()` for consistency
- `flushDeferred()` is O(n) where n is the number of pending deferred closures
