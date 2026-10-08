# Engine

**Header:** `<bg2e/draw/Engine.hpp>`  
**Namespace:** `bg2e::draw`

The experimental GPU context owner. The public header uses abstract gpu types
and PImpl, with no Vulkan or Metal native API types. Copy construction and copy
assignment are deleted.

```cpp
Engine();
~Engine();
void init(SDL_Window* window, gpu::Backend& backend, const EngineConfig& config);
void cleanup();
gpu::BackendType backendType() const;
gpu::Instance* instance() const;
gpu::PhysicalDevice* physicalDevice() const;
gpu::Device* device() const;
gpu::WindowSurface* surface() const;
gpu::CleanupManager& cleanupManager();
```

## Initialization and current behavior

`init()` records the borrowed window/backend pointers and configuration, then
throws `std::logic_error` with:

```text
draw GPU initialization is not implemented; complete milestone 02
```

It does not create GPU objects or mark the engine initialized. All accessors,
including backendType(), throw `std::logic_error("draw::Engine is not initialized.")`
until initialization is implemented. The pointer-returning accessors do not
return null as an alternative to this exception. A repeated init is not a
successful retry in this milestone: initialization always reaches this boundary.

## Ownership

The shell declares borrowed SDL window, Backend and shared Instance references,
and exclusive PhysicalDevice, Device, WindowSurface and CleanupManager ownership.
GPU object construction is pending. Applications normally select EngineConfig
through MainLoop rather than constructing its private execution wrapper.

`cleanup()` resets the shell, releases its stored wrappers and borrowed
references, restores default configuration and clears initialization state.
It is safe to call on an uninitialized shell and repeatedly. The destructor is
currently defaulted; do not interpret it as an implemented GPU shutdown protocol.
Actual synchronization and ordered GPU cleanup belong to milestone 02.

See [EngineConfig](EngineConfig.md) and [RenderLoop](RenderLoop.md).
