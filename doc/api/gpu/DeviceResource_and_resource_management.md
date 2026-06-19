# DeviceResource and Explicit GPU Resource Management

`DeviceResource` defines the common lifecycle model for GPU resources created by `bg2e::gpu::Device`.

The goal of this design is to make GPU resource ownership explicit, predictable, backend-agnostic, and safe across Vulkan, Metal, and future backends such as D3D12 or WebGPU.

Unlike ordinary C++ objects, GPU resources are not just host-side objects. They usually own backend handles, memory allocations, descriptor state, pipeline state, command resources, or other objects whose destruction order matters. For this reason, bg2e uses explicit cleanup instead of relying on destructors to release GPU resources implicitly.

## Core principle

Every object created through `gpu::Device` is a `DeviceResource`.

Typical examples include:

```cpp
gpu::Buffer
gpu::Image
gpu::Sampler
gpu::ShaderModule
gpu::PipelineLayout
gpu::GraphicsPipeline
gpu::ComputePipeline
gpu::ResourceSet
```

A `DeviceResource` always stores a reference to the `gpu::Device` that created it. This gives every resource enough context to release its own backend-specific objects during `cleanup()`.

Objects that are not created by `gpu::Device` are not required to inherit from `DeviceResource`. For example:

```cpp
gpu::Instance
gpu::PhysicalDevice
gpu::Surface
gpu::SurfaceFrame
gpu::Backend
gpu::Factory
```

This distinction is intentional. `DeviceResource` does not mean “any GPU-related object”. It means “a GPU resource owned by a logical device”.

## Creation model

Device resources are created through factory methods on `gpu::Device`.

For example:

```cpp
auto image = device->createImage({
    { 1024, 1024 },
    gpu::PixelFormat::R8G8B8A8_UNORM,
    gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
    "Albedo texture"
});

auto sampler = device->createSampler({
    .debugName = "Default sampler"
});

auto pipeline = device->createGraphicsPipeline(pipelineDesc);
```

The application never constructs backend resources directly. The device selects the concrete backend implementation:

```cpp
gpu::vk::Image
gpu::metal::Image
```

while the user-facing type remains:

```cpp
gpu::Image
```

This keeps the public API backend-agnostic.

## Shared ownership

All objects created by `gpu::Device` are returned as `std::shared_ptr`.

This is intentional. GPU resources often need to be referenced from several places at the same time:

```cpp
std::shared_ptr<gpu::Image> texture;
std::shared_ptr<gpu::ResourceSet> materialSet;
std::shared_ptr<gpu::GraphicsPipeline> pipeline;
```

For example, a texture may be owned by a material cache, referenced by a resource set, and also registered in a cleanup manager. Using `std::shared_ptr` makes this ownership explicit and prevents the host-side C++ object from being destroyed while it is still registered for cleanup.

`DeviceResource` also inherits from `std::enable_shared_from_this`. This allows resources and helper systems to safely obtain a shared reference to the resource when needed, as long as the object has already been created through `gpu::Device`.

The rule is simple:

```cpp
auto resource = device->createBuffer("Per-object buffer");
```

After creation, the resource is managed by `std::shared_ptr`.

A `DeviceResource` must never call `shared_from_this()` inside its constructor.

## Explicit cleanup

A `DeviceResource` exposes a virtual `cleanup()` method:

```cpp
class DeviceResource : public std::enable_shared_from_this<DeviceResource> {
public:
    explicit DeviceResource(Device* device);
    virtual ~DeviceResource() = default;

    Device* device() const;

    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;

protected:
    Device* _device = nullptr;
};
```

The destructor does not replace `cleanup()`.

This is a deliberate design choice. In graphics APIs such as Vulkan and Metal, the destruction order of resources is significant. Relying on C++ destructor order can easily produce invalid destruction sequences, especially when resources are owned through nested objects, caches, frame rings, or application-level systems.

Instead, bg2e uses explicit cleanup:

```cpp
device->waitIdle();

pipeline->cleanup();
layout->cleanup();
texture->cleanup();

device->cleanup();
instance->cleanup();
```

Each resource is responsible for releasing its own backend-specific objects.

A good `cleanup()` implementation must be idempotent:

```text
- If the resource is already clean, cleanup() should do nothing.
- Native backend handles should be reset to null/nullptr after destruction.
- Allocations should be released and cleared.
- isValid() should return false after cleanup().
```

This makes cleanup robust even when systems are refactored or when a resource is manually cleaned before being flushed by a manager.

## Why not clean in destructors?

GPU resource destruction is not equivalent to ordinary memory destruction.

A destructor runs when C++ ownership ends. That moment may not match the correct GPU lifetime. The GPU may still be using the resource, or other backend objects may still depend on it.

For this reason, bg2e separates two lifetimes:

```text
C++ object lifetime
GPU backend resource lifetime
```

`std::shared_ptr` controls the C++ object lifetime.

`cleanup()` controls the GPU backend resource lifetime.

This separation allows the engine to keep C++ objects alive for bookkeeping while explicitly releasing backend resources at a safe and predictable point.

## CleanupManager

`gpu::CleanupManager` provides ordered cleanup for groups of `DeviceResource` objects and deferred cleanup closures whose execution is tied to the surface frame counter.

It stores `std::shared_ptr<DeviceResource>`, not raw pointers and not lambdas. This guarantees that every registered object remains alive until the manager is flushed.

The constructor requires a `Surface*` pointer for deferred cleanup timing. The surface is **not owned** — the caller must ensure it outlives the manager.

Typical usage:

```cpp
gpu::CleanupManager cleanup(surface.get());

auto layout = device->createPipelineLayout(layoutDesc);
cleanup.push(layout);

auto pipeline = device->createGraphicsPipeline(pipelineDesc);
cleanup.push(pipeline);

auto texture = device->createImage(textureDesc);
cleanup.push(texture);

// ...

device->waitIdle();
cleanup.flush();
```

The cleanup manager does not call `device->waitIdle()`. Synchronization remains the responsibility of the owner of the rendering loop or higher-level system.

## Cleanup order

`CleanupManager` has two queues:

```cpp
cleanup.push(resource);
cleanup.pushStatic(resource);
```

Normal resources are cleaned in reverse insertion order.

Static resources are cleaned first, in insertion order.

The flush order is:

```text
1. Static resources, insertion order.
2. Normal resources, reverse insertion order.
3. Clear all stored shared_ptr references.
```

This is useful for global or shared backend resources such as:

```text
- texture caches
- geometry caches
- editor gizmo meshes
- shared fallback images
- global material resources
```

These resources may need to be released before ordinary per-object resources are destroyed.

## Example cleanup sequence

```cpp
gpu::CleanupManager cleanup;

auto shader = device->createShaderModule(shaderDesc);
cleanup.push(shader);

auto layout = device->createPipelineLayout(layoutDesc);
cleanup.push(layout);

auto pipeline = device->createGraphicsPipeline(pipelineDesc);
cleanup.push(pipeline);

auto texture = device->createImage(textureDesc);
cleanup.push(texture);

auto sampler = device->createSampler(samplerDesc);
cleanup.push(sampler);

// Render loop...

device->waitIdle();
cleanup.flush();

surface->cleanup();
device->cleanup();
instance->cleanup();
```

The normal queue is flushed in reverse order. In this example, the sampler is cleaned before the texture, the texture before the pipeline, and so on.

## Static cleanup handlers

Static resources are not “static” in the C++ language sense. In this context, “static” means engine-level or long-lived shared resources that should be cleaned before the regular deletion queue.

For example:

```cpp
cleanup.pushStatic(globalTextureCache);
cleanup.pushStatic(editorGizmoCache);
```

When `flush()` is called, these resources are cleaned first.

This allows global caches to release backend resources before the rest of the device-level resources are destroyed.

## Deferred cleanup

In addition to ordered cleanup, `CleanupManager` supports **deferred cleanup** — scheduling GPU resource destruction closures that execute only after a specified number of frames have elapsed. This is essential for safely destroying GPU resources that may still be in use by the GPU.

### The problem

When a GPU resource (e.g., a vertex buffer) is replaced at runtime, the old resource cannot be destroyed immediately — the GPU may still be referencing it in a previously submitted command buffer. Destroying it too early produces validation errors or undefined behavior.

### The solution

`CleanupManager::defer()` schedules a closure for execution after `inFlightFrames()` frames have elapsed. By that time, the GPU is guaranteed to have finished using the resource:

```cpp
// Replace geometry at runtime
auto oldMesh = std::move(gpuMesh);

// Schedule deferred destruction — closure captures oldMesh by move
cleanup.defer([oldMesh = std::move(oldMesh)]() mutable {
    oldMesh.cleanup();
});

// Create new geometry
gpuMesh.setMeshData(newMeshData);
gpuMesh.build(device.get());
```

The closure captures the old `MeshPU` by move. Since `MeshPU` owns the GPU buffers as `shared_ptr<DeviceResource>`, the buffers stay alive until the closure executes.

### Frame counter and timing

`CleanupManager` requires a `Surface*` at construction. The surface provides:

- `frameCounter()` — monotonically increasing `uint64_t`, incremented by `endFrame()`.
- `inFlightFrames()` — number of concurrent frames in flight (2 for window surfaces, 1 for offscreen).

When `defer()` is called, it computes:

```
targetFrame = surface->frameCounter() + surface->inFlightFrames()
```

The closure executes when `flushDeferred()` is called and `surface->frameCounter() >= targetFrame`.

### Render loop integration

Deferred closures are flushed **after** `endFrame()`, once the fence has been waited:

```cpp
auto frame = surface->beginFrame();
// ... record and submit commands ...
surface->endFrame(frame.get());

// Safe to run expired deferred closures
cleanup.flushDeferred();
```

On application exit, all pending closures are flushed unconditionally:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();   // execute all pending deferred closures
cleanup.flush();               // ordered device resource cleanup
surface->cleanup();
device->cleanup();
instance->cleanup();
```

On window resize, drain all pending closures before recreating the swapchain:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();
surface->resize(newSize);
```

### API summary

| Method | Description |
|--------|-------------|
| `defer(closure)` | Schedule a closure for deferred execution after `inFlightFrames()` frames |
| `flushDeferred()` | Execute all closures whose `targetFrame <= frameCounter()` |
| `flushAllDeferred()` | Execute all pending closures immediately (for shutdown) |

See [CleanupManager](CleanupManager.md) for the full API reference.

## Resource validity

Every `DeviceResource` must expose:

```cpp
bool isValid() const;
```

This function reports whether the backend resource is currently initialized.

After creation and successful initialization:

```cpp
resource->isValid(); // true
```

After cleanup:

```cpp
resource->cleanup();
resource->isValid(); // false
```

This is useful for debugging, validation, assertions, and defensive cleanup code.

## Backend-specific cleanup

The public cleanup API is common:

```cpp
resource->cleanup();
```

The implementation is backend-specific.

For example:

```cpp
gpu::vk::Buffer::cleanup()
```

may destroy:

```text
VkBuffer
VmaAllocation
device address metadata
```

while:

```cpp
gpu::metal::Buffer::cleanup()
```

may release:

```text
MTL::Buffer*
```

The caller does not need to know which backend is active.

## Device ownership

`DeviceResource` stores a raw `Device*`, not a `std::shared_ptr<Device>`.

This avoids ownership cycles:

```text
Device creates Resource
Resource refers back to Device
```

The engine guarantees that the `Device` outlives all resources created from it.

The correct shutdown order is therefore:

```text
1. Stop submitting work.
2. Wait for the device to become idle.
3. Cleanup device resources.
4. Cleanup surface.
5. Cleanup device.
6. Cleanup instance.
```

## Relationship with frame resource rings

`DeviceResource` and `CleanupManager` also provide the foundation for per-frame resource rings.

A frame resource ring stores one resource per frame slot:

```cpp
gpu::FrameResourceRing<gpu::Buffer>
gpu::FrameResourceRing<gpu::ResourceSet>
```

This is useful for resources that are updated every frame, such as uniform buffers containing model matrices or per-frame scene data.

Instead of creating and destroying a buffer every frame, the engine can keep several persistent copies:

```text
frame slot 0 -> buffer 0
frame slot 1 -> buffer 1
frame slot 2 -> buffer 2
```

Each buffer is a `DeviceResource`, so the entire ring can be cleaned explicitly and safely when the owning object is destroyed.

## Design benefits

The `DeviceResource` model provides several benefits:

```text
- Every device-created resource has a consistent lifecycle.
- Every resource knows the device that created it.
- Cleanup is explicit and ordered.
- Host object lifetime and GPU resource lifetime are separated.
- CleanupManager can keep resources alive until cleanup.
- Backend details remain hidden behind the common API.
- Future systems such as frame resource rings, caches, and editor resources can use the same model.
```

This design is especially important for a multi-backend engine. Vulkan, Metal, D3D12, and WebGPU have different resource management models, but bg2e exposes a single lifecycle pattern to the rest of the engine.

## Summary

`DeviceResource` is the base class for all resources created by `gpu::Device`.

Resources are created as `std::shared_ptr`, cleaned explicitly with `cleanup()`, and can be registered in `CleanupManager` for ordered destruction.

This gives bg2e a deterministic and backend-agnostic resource management model while still preserving the low-level control required by modern graphics APIs.
