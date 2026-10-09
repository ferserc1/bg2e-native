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

## Initialization

`init()` requires a non-null SDL window and a Backend whose type matches
EngineConfig. It borrows the backend's shared Instance wrapper, rejects an
instance already in use, applies the configured application name and debug mode,
and creates the windowed instance. MainLoop substitutes its appId when the
configured application name is empty.

The engine creates a WindowSurface with the requested formats, selects a
PhysicalDevice, and creates a Device. Device creation establishes the surface
render target. Finally, the engine creates the CleanupManager and marks itself
initialized. Accessors throw `std::logic_error` until initialization succeeds.
An initialized engine must be cleaned before another call to `init()`.

Initialization failure cleans completed and partially started stages and
rethrows the original exception. Cleanup allows the engine to be initialized
again.

## Ownership and shutdown

The SDL window and shared Instance wrapper are borrowed. Factory-owned backends
are retained through a lease: Factory rejects backend replacement while the
execution or Engine retains it. A caller-owned Backend must outlive the Engine.
The Engine controls the initialized Instance lifetime and exclusively owns its
PhysicalDevice, Device, WindowSurface and CleanupManager.

The caller must stop frame production and coordinate background GPU producers
before cleanup, keeping them stopped throughout resource destruction.
`cleanup()` waits through Device::waitIdle, drains deferred and registered
resources, destroys the surface before the device, then cleans the instance
and detaches borrowed references. It is safe before initialization and on
repeat calls. Explicit cleanup reports errors; the destructor attempts cleanup
without propagating exceptions.

## MainLoop execution

Milestone 02 step 03 enables experimental MainLoop execution: draw can present
retained scene color through Vulkan or Metal. UI preparation and composition are implemented for Vulkan and Metal in
steps 04/05. Event processing and frame overrides are enabled after the selected
UI backend is initialized.
Scene and presentation coordination belongs to RenderLoop.

See [EngineConfig](EngineConfig.md) and [RenderLoop](RenderLoop.md).
