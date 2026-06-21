# Tutorial 12: Deferred Cleanup

This tutorial walks through the `12_deferred_cleanup` example: demonstrating deferred resource cleanup by switching between cube and sphere geometry every 5 seconds. When switching, old GPU buffers are scheduled for deferred destruction via `CleanupManager::defer()` rather than immediate deletion.

**Source:** `examples/gpu/12_deferred_cleanup/src/main.cpp`

## What you will learn

- Why GPU resources cannot always be destroyed immediately
- How `CleanupManager::defer()` schedules closures for deferred execution
- How `flushDeferred()` runs expired closures after `endFrame()`
- How `flushAllDeferred()` drains all pending closures at shutdown and on resize
- How to safely replace GPU resources at runtime without validation errors

## Prerequisites

- Completed [07_uniform_buffers](07_uniform_buffers.md) -- you should understand UBOs, `FrameResourceRing`, and basic cleanup patterns
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding deferred cleanup

When a GPU resource (e.g., a vertex buffer) is replaced at runtime, the old resource cannot be destroyed immediately. The GPU may still be referencing it in a previously submitted command buffer. Destroying it too early produces validation errors or undefined behavior.

`CleanupManager::defer()` solves this by scheduling a closure for execution after `inFlightFrames()` frames have elapsed. By that time, the GPU is guaranteed to have finished using the resource:

```
Frame N:   Old mesh in use by GPU (command buffer submitted)
Frame N+1: New mesh created, old mesh captured in closure
Frame N+2: closure.targetFrame reached -> old mesh cleaned up
```

The `targetFrame` is computed as:

```
targetFrame = surface->frameCounter() + surface->inFlightFrames()
```

For window surfaces, `inFlightFrames()` returns 2, so the closure runs 2 frames after it was scheduled.

**Reference:** [GPU API -- CleanupManager](../../api/gpu/CleanupManager.md), [GPU API -- DeviceResource and resource management](../../api/gpu/DeviceResource_and_resource_management.md)

## Step-by-step code explanation

### 1. Device initialization

The first steps follow the standard GPU initialization pattern from previous tutorials:

```cpp
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
{
    std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
    int choice = 0;
    std::cin >> choice;
    backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
}

gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
```

SDL window creation, instance, surface, physical device, and logical device are created using the same pattern as [07_uniform_buffers](07_uniform_buffers.md). The `CleanupManager` is constructed with a `Surface*` for deferred timing:

```cpp
gpu::CleanupManager cleanup(surface.get());
```

**Reference:** [GPU API -- CleanupManager: Construction](../../api/gpu/CleanupManager.md#construction)

### 2. Pipeline layout (set 0, 1, 2)

The pipeline layout defines three descriptor sets:

```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});
```

| Set | Binding | Type | Stage | Purpose |
|-----|---------|------|-------|---------|
| 0 | 0 | UniformBuffer | Vertex | Camera projection-view matrix |
| 1 | 0 | UniformBuffer | Vertex | Per-object model matrix |
| 2 | 0 | SampledImage | Fragment | Texture |
| 2 | 1 | Sampler | Fragment | Texture sampler |

### 3. Persistent resources (texture, camera UBO)

The texture, sampler, material resource set, camera UBO, and camera resource set are created once and registered for ordered cleanup. These are never replaced during the application lifetime:

```cpp
auto texture = device->createImage({
    .size = {2, 2},
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
    .debugName = "Procedural 2x2 texture"
});
texture->uploadRGBA8(texels.data(), { 2, 2 });
device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
{
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
cleanup.push(texture);
```

These resources use `cleanup.push()` (normal ordered cleanup), not deferred cleanup, because they are never replaced at runtime.

### 4. Per-frame resources with FrameResourceRing

The model UBO and its resource set are managed through `FrameResourceRing` to avoid data races between CPU writes and GPU reads:

```cpp
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i)
{
    auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i)
{
    auto set = device->createResourceSet(graphicsLayout.get(), 1,
        "Model resource set ring[" + std::to_string(i) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
    set->update();
    return set;
});
```

Each ring slot holds a `shared_ptr` to its resource. The resource set in each slot captures `modelUboRing.sharedAt(i)` to keep the corresponding UBO alive. The ring size matches `surface->inFlightFrames()` (typically 2).

**Reference:** [GPU API -- FrameResourceRing](../../api/gpu/FrameResourceRing.md)

### 5. Geometry switching with deferred cleanup

This is the core of the example. A `MeshPU` holds the current GPU mesh, and a lambda creates new geometry:

```cpp
gpu::MeshPU gpuMesh;
std::unique_ptr<geo::MeshPU> meshData;

auto createGeometry = [&](bool sphere) {
    if (sphere) {
        meshData.reset(bg2e::geo::createSpherePU(0.8f, 24, 24));
    } else {
        meshData.reset(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));
    }
    gpuMesh.setMeshData(*meshData);
    gpuMesh.build(device.get());
};

createGeometry(false);
```

Every 5 seconds, the geometry switches. The old mesh is moved into a deferred closure:

```cpp
if (firstFrame || currentTime - lastSwitchTime >= 5000)
{
    if (!firstFrame)
    {
        auto oldMesh = std::move(gpuMesh);

        cleanup.defer([oldMesh = std::move(oldMesh), useSphere]() mutable {
            bg2e_log_debug << "Removing mesh " << (useSphere ? "sphere" : "cube") << bg2e_log_end;
            oldMesh.cleanup();
        });

        useSphere = !useSphere;
    }

    createGeometry(useSphere);
    lastSwitchTime = currentTime;
    firstFrame = false;
}
```

The sequence is:

1. **Move** the current `gpuMesh` into `oldMesh` (the `gpuMesh` is now empty)
2. **Schedule** a deferred closure that captures `oldMesh` by move -- this extends the GPU resource lifetime until the closure executes
3. **Create** the new geometry into `gpuMesh`

The closure captures `useSphere` to log which mesh is being destroyed (for debugging).

**Reference:** [GPU API -- CleanupManager::defer()](../../api/gpu/CleanupManager.md#defer)

### 6. Render loop and flushDeferred() timing

The render loop follows the standard pattern, with one critical addition -- `flushDeferred()` is called **after** `endFrame()`:

```cpp
auto frame = surface->beginFrame();
auto cmd = graphicsQueue.createCommandBuffer("Frame command buffer");

auto* modelUbo = modelUboRing.current();
modelUbo->updateUniformBuffer(modelData);
auto* modelSet = modelSetRing.current();

cmd->begin();
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
cmd->clearDepth(1.0f);

cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
cmd->bindResourceSet(pipeline.get(), 1, modelSet);
cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
gpuMesh.draw(cmd.get());

cmd->endRendering();
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());

// Flush deferred cleanups AFTER endFrame() (fence has been waited)
cleanup.flushDeferred();
```

The key ordering is:

1. `surface->endFrame()` -- presents the frame, waits the fence for this frame slot
2. `cleanup.flushDeferred()` -- runs any closures whose `targetFrame <= frameCounter()`

This guarantees the GPU is done with the previous frame's resources before the deferred closures execute.

**Reference:** [GPU API -- CleanupManager: Render loop integration](../../api/gpu/CleanupManager.md#render-loop-integration)

### 7. Window resize handling

On resize, all pending deferred closures must be drained before recreating the swapchain:

```cpp
if (event.type == SDL_WINDOWEVENT &&
    event.window.event == SDL_WINDOWEVENT_RESIZED)
{
    device->waitIdle();
    cleanup.flushAllDeferred();
    surface->resize({
        static_cast<uint32_t>(event.window.data1),
        static_cast<uint32_t>(event.window.data2)
    });
}
```

`device->waitIdle()` ensures the GPU is not executing any commands. `flushAllDeferred()` runs **all** pending closures immediately, regardless of frame counter. This prevents stale resources from being referenced after the swapchain is recreated.

### 8. Application shutdown

On exit, the cleanup sequence drains deferred closures first, then runs ordered cleanup:

```cpp
device->waitIdle();
cleanup.flushAllDeferred();

modelSetRing.cleanup();
modelUboRing.cleanup();
gpuMesh.cleanup();
cleanup.flush();

surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

The order matters:

1. `waitIdle()` -- stop the GPU
2. `flushAllDeferred()` -- execute all remaining deferred closures
3. Ring resources and current mesh -- manually cleaned before the general flush
4. `cleanup.flush()` -- ordered device resource cleanup (reverse insertion order)
5. Surface, device, instance -- teardown in reverse creation order

## Shader code explanation

The shaders are identical to those used in [07_uniform_buffers](07_uniform_buffers.md). The geometry switching does not affect the shaders -- only the vertex data changes.

### Vertex Shader (`cube.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionView;
} camera;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

The vertex shader transforms positions using the camera's projection-view matrix and the per-object model matrix, then passes texture coordinates to the fragment stage.

### Fragment Shader (`cube.frag.glsl`)

```glsl
#version 450

layout(set = 2, binding = 0) uniform texture2D uTex;
layout(set = 2, binding = 1) uniform sampler   uSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

The fragment shader samples the procedural 2x2 texture using combined image samplers. The separate `texture2D` and `sampler` objects are combined at runtime via `sampler2D(uTex, uSampler)`.

## Key concepts

### Why deferred cleanup is necessary

```
GPU command buffer timeline:
  Frame 0: submit draw calls using mesh A
  Frame 1: submit draw calls using mesh B (mesh A still in-flight)
  Frame 2: GPU finishes frame 0 -> mesh A is safe to destroy
```

If mesh A is destroyed at the start of frame 1, the GPU command buffer for frame 0 may still reference its vertex/index buffers. This produces validation errors or undefined behavior.

`defer()` captures the old resource in a closure and schedules it for destruction after `inFlightFrames()` frames (2 for window surfaces). By the time the closure runs, the GPU is guaranteed to have finished all frames that used the old resource.

### Three cleanup methods compared

| Method | When to use | What it does |
|--------|-------------|--------------|
| `cleanup.push()` | Static resources (never replaced) | Ordered destruction on `flush()` |
| `cleanup.defer()` | Resources replaced at runtime | Deferred destruction after N frames |
| `cleanup.flushAllDeferred()` | Shutdown or resize | Immediately runs all pending deferred closures |

### The move-capture pattern

```cpp
auto oldMesh = std::move(gpuMesh);

cleanup.defer([oldMesh = std::move(oldMesh)]() mutable {
    oldMesh.cleanup();
});
```

This pattern uses two moves:

1. `std::move(gpuMesh)` -- transfers the `MeshPU` value out of `gpuMesh` (which is now empty)
2. `oldMesh = std::move(oldMesh)` in the lambda capture -- moves the `MeshPU` into the closure, extending its lifetime

The closure captures by move to ensure the GPU resources stay alive until the closure executes. If captured by reference, the resources would be destroyed when the enclosing scope exits, which may be too early.

### flushDeferred() vs flushAllDeferred()

| Method | Timing | Use case |
|--------|--------|----------|
| `flushDeferred()` | After `endFrame()` each frame | Normal operation -- only runs expired closures |
| `flushAllDeferred()` | Shutdown or resize | Drains all pending closures immediately |

`flushDeferred()` is selective: it only runs closures whose `targetFrame <= frameCounter()`. `flushAllDeferred()` is unconditional: it runs everything.

## Next steps

- **[GPU API -- CleanupManager](../../api/gpu/CleanupManager.md)** -- Full reference for deferred and ordered cleanup
- **[GPU API -- DeviceResource and resource management](../../api/gpu/DeviceResource_and_resource_management.md)** -- Full reference for the resource lifecycle model
- **[GPU API -- FrameResourceRing](../../api/gpu/FrameResourceRing.md)** -- Full reference for per-frame resource rings
- **[Tutorial 07: Uniform Buffers](07_uniform_buffers.md)** -- The foundational example for UBOs and pipeline layouts
