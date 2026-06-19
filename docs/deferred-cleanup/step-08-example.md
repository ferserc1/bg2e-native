# Step 08: Validation Example — 12_deferred_cleanup

## Files to Create/Modify

| File | Action |
|------|--------|
| `examples/gpu/12_deferred_cleanup/CMakeLists.txt` | Create |
| `examples/gpu/12_deferred_cleanup/shaders/cube.vert.glsl` | Create (copy from 07) |
| `examples/gpu/12_deferred_cleanup/shaders/cube.frag.glsl` | Create (copy from 07) |
| `examples/gpu/12_deferred_cleanup/shaders/cube.vert.metal` | Create (copy from 07) |
| `examples/gpu/12_deferred_cleanup/shaders/cube.frag.metal` | Create (copy from 07) |
| `examples/gpu/12_deferred_cleanup/src/main.cpp` | Create |
| `examples/CMakeLists.txt` | Modify (add subdirectory) |

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_deferred_cleanup)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")

# Metal shaders (macOS only, no-op elsewhere)
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

compile_shaders_shaderlib(${APP_TARGET_NAME} ${VULKAN_SDK} "${APP_SHADERS_SRC}" "${APP_SHADERS_DST}")
bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
```

## examples/CMakeLists.txt Change

Add after the last `gpu/` entry:

```cmake
add_subdirectory(gpu/12_deferred_cleanup)
```

## Shaders

All four shader files are identical copies from `examples/gpu/07_uniform_buffers/shaders/`:
- `cube.vert.glsl` — position + texcoord, camera UBO (set 0), model UBO (set 1)
- `cube.frag.glsl` — texture (set 2 binding 0) + sampler (set 2 binding 1)
- `cube.vert.metal` — Metal equivalent
- `cube.frag.metal` — Metal equivalent

## main.cpp Design

### Overview

This example demonstrates deferred resource cleanup by switching between cube and sphere geometry every 5 seconds. When switching, the old GPU buffers are scheduled for deferred destruction rather than immediate deletion.

### Global State (in `main()`)

```cpp
// CPU-side geometry (agnostic)
std::unique_ptr<bg2e::geo::MeshPU> meshData;

// GPU mesh (vertex + index buffers)
gpu::MeshPU gpuMesh;

// Toggle state
bool useSphere = false;
uint32_t lastSwitchTime = 0;
bool firstFrame = true;
```

### Geometry Creation Function

```cpp
auto createGeometry = [&](gpu::Device* device, bool sphere) {
    if (sphere) {
        meshData.reset(bg2e::geo::createSpherePU(0.8f, 24, 24));
    } else {
        meshData.reset(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));
    }
    gpuMesh.setMeshData(*meshData);
    gpuMesh.build(device);
};
```

### Main Loop Structure

```cpp
bool running = true;
while (running)
{
    // SDL event handling (same as 07)
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            device->waitIdle();
            cleanupManager.flushAllDeferred();
            surface->resize({
                static_cast<uint32_t>(event.window.data1),
                static_cast<uint32_t>(event.window.data2)
            });
        }
    }

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
    uint32_t currentTime = SDL_GetTicks();

    // Switch geometry every 5 seconds
    if (firstFrame || currentTime - lastSwitchTime >= 5000)
    {
        if (!firstFrame)
        {
            // Schedule deferred destruction of current GPU mesh buffers.
            // Capture the shared_ptrs so they stay alive until the closure runs.
            auto vertexBuf = gpuMesh.vertexBuffer() ?
                std::shared_ptr<gpu::Buffer>(gpuMesh.vertexBuffer()->shared_from_this()) : nullptr;
            auto indexBuf = gpuMesh.indexBuffer() ?
                std::shared_ptr<gpu::Buffer>(gpuMesh.indexBuffer()->shared_from_this()) : nullptr;

            // Move the mesh data out — gpuMesh will be rebuilt below
            auto oldMesh = std::move(gpuMesh);

            cleanupManager.defer([oldMesh = std::move(oldMesh)]() mutable {
                oldMesh.cleanup();
            });

            useSphere = !useSphere;
        }

        createGeometry(device.get(), useSphere);
        lastSwitchTime = currentTime;
        firstFrame = false;
    }

    // Update model UBO (rotation)
    ModelUBO modelData{};
    modelData.model = glm::rotate(glm::mat4(1.0f), t,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

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
    cleanupManager.flushDeferred();
}
```

### Cleanup Sequence

```cpp
device->waitIdle();
cleanupManager.flushAllDeferred();   // Run any remaining deferred closures

modelSetRing.cleanup();
modelUboRing.cleanup();
gpuMesh.cleanup();                   // gpu::MeshPU is not a DeviceResource
cleanup.flush();                     // DeviceResource objects

surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

## Potential Issues to Monitor

1. **`immediateSubmit()` during geometry creation**: The `gpuMesh.build(device)` call internally creates buffers. This should work fine because `immediateSubmit()` is designed for this use case. However, if a resize event happens simultaneously, `waitIdle()` is called first.

2. **Shared pointer lifetime**: The deferred closure must capture the `gpu::MeshPU` by value (move) so the underlying buffers stay alive until the closure executes. The `MeshPU::cleanup()` method calls `cleanup()` on both vertex and index buffers.

3. **First frame**: The first geometry is created before entering the loop, so there is nothing to defer on the first switch.

4. **Resize events**: When a resize occurs, we call `waitIdle()` + `flushAllDeferred()` to drain all pending closures before recreating the swapchain. This mirrors the pattern in `render::Engine::cleanup()`.

## Expected Behavior

- Window opens showing a rotating cube
- After 5 seconds, the cube is replaced by a sphere (old buffers deferred)
- After 10 seconds, the sphere is replaced by a cube again
- The cycle continues indefinitely
- No Vulkan validation errors (buffers are not destroyed while in use)
- Closing the window exits cleanly
