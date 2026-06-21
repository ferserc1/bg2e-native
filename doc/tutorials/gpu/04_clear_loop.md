# Tutorial 04: Animated Clear Loop

This tutorial walks through the `04_clear_loop` example: running a continuous render loop that clears the screen with an animated color each frame. This is the first example that actually draws something to the window, and it introduces the core concepts of frame acquisition, command buffers, image transitions, and presentation.

**Source:** `examples/gpu/04_clear_loop/src/main.cpp`

## What you will learn

- How to structure a continuous render loop with SDL event handling
- How to acquire a frame from the swapchain and create a command buffer
- How to transition image layouts for rendering
- How to clear the screen with a color value
- How to present a rendered frame to the display
- The correct order of operations in a GPU render loop

## Prerequisites

- Completed [02_device](02_device.md) -- you should understand device and queue creation
- Completed [03_offscreen_device](03_offscreen_device.md) -- you should understand the initialization pipeline
- bg2e-native built and available on your system

## The render loop structure

The render loop follows a strict sequence of operations each frame. The API documentation defines this canonical order:

```
1.  Poll SDL events
2.  surface->beginFrame()          // acquire next swapchain image
3.  cmd = graphicsQueue.createCommandBuffer()
4.  cmd->begin()
5.  cmd->transition(color, ColorAttachment)
6.  cmd->transition(depth, DepthAttachment)
7.  cmd->beginRendering(frame)
8.  cmd->clearColor(0, clearColor)
9.  cmd->clearDepth(1.0f)
10. cmd->endRendering()
11. cmd->transition(color, Present)
12. surface->present(cmd)           // record present
13. cmd->end()
14. graphicsQueue.submit(cmd)       // submit to GPU
15. surface->endFrame(frame)        // present to screen
```

**Reference:** [GPU API Index -- Render loop structure](../../api/gpu/index.md), [Surface API -- Frame lifecycle](../../api/gpu/Surface.md#frame-lifecycle)

## Step-by-step code explanation

### 1. Headers and includes

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <cmath>
#include <iostream>
```

| Header | Purpose |
|--------|---------|
| `<bg2e.hpp>` | Engine umbrella header |
| `<bg2e/gpu/all.hpp>` | GPU abstraction layer (Factory, Backend, Device, CommandBuffer, etc.) |
| `<bg2e/app/SDLUtils.hpp>` | SDL helper utilities (`app::initSdlVideoDriver()`) |
| `<cmath>` | `std::sin()` for the animated clear color |

### 2. Backend selection and initialization (steps 1-7)

The first seven steps of this example are identical to [02_device](02_device.md). They create the full GPU initialization pipeline:

```cpp
// 1. Select backend per platform
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
{
    std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
    int choice = 0;
    std::cin >> choice;
    backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
}

// 2. Init backend to query window type
gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();

// 3. SDL init + create window based on backend window type
app::initSdlVideoDriver();
SDL_Init(SDL_INIT_VIDEO);

Uint32 windowFlags = 0;
switch (backend->windowType())
{
    case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
    case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
}

SDL_Window* window = SDL_CreateWindow(
    "GPU Clear Loop Example",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    800, 600,
    windowFlags
);

// 4. Create GPU instance
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create(window);

// 5. Create surface
std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

// 6. Select physical device
auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);

// 7. Create logical device
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());
```

The surface is stored as a `std::shared_ptr` because the frame lifecycle API (`beginFrame()`, `present()`, `endFrame()`) requires shared ownership of the surface throughout the render loop.

**Reference:** [Factory API](../../api/gpu/Factory.md), [Instance API](../../api/gpu/Instance.md), [WindowSurface API](../../api/gpu/WindowSurface.md), [Device API](../../api/gpu/Device.md)

### 3. SDL event handling

```cpp
bool running = true;
while (running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            surface->resize({ static_cast<uint32_t>(event.window.data1),
                              static_cast<uint32_t>(event.window.data2) });
        }
    }
```

The outer `while (running)` is the main render loop. Inside it, `SDL_PollEvent` drains all pending events before proceeding to render.

Three events are handled:

| Event | Action |
|-------|--------|
| `SDL_QUIT` | Application asked to close (e.g., OS signal) |
| `SDL_WINDOWEVENT_CLOSE` | Window close button clicked |
| `SDL_WINDOWEVENT_RESIZED` | Window resized -- calls `surface->resize()` to recreate swapchain images |

The `surface->resize()` call is critical. When the window is resized, the swapchain images become invalid. The surface recreates them at the new dimensions, and the next `beginFrame()` will use the updated sizes.

**Reference:** [Surface API -- resize()](../../api/gpu/Surface.md)

### 4. Animated clear color calculation

```cpp
    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
    gpu::Color clearColor {
        0.5f + 0.5f * std::sin(t),
        0.5f + 0.5f * std::sin(t + 2.0f),
        0.5f + 0.5f * std::sin(t + 4.0f),
        1.0f
    };
```

This produces a smoothly oscillating RGB color:

- `SDL_GetTicks64()` returns milliseconds since SDL initialization
- Dividing by 1000.0 converts to seconds
- Each channel uses `std::sin()` with a phase offset (0, 2, 4) so the colors shift independently
- The formula `0.5 + 0.5 * sin(t)` maps the sine range `[-1, 1]` to `[0, 1]`
- Alpha is always 1.0 (fully opaque)

The result is a color that cycles through all hues over approximately 6.3 seconds (one full sine period at `2 * PI`).

### 5. Frame acquisition

```cpp
    auto frame = surface->beginFrame();
```

`beginFrame()` acquires the next available image from the swapchain. It returns a `std::shared_ptr<SurfaceFrame>` containing:

- **`colorImage()`** -- the color attachment (the image you render into)
- **`depthImage()`** -- the depth attachment (for depth testing)

The swapchain typically has 2-3 images. `beginFrame()` picks the one that is not currently being displayed or rendered to. If all images are in use, the call blocks until one becomes available.

Returns `nullptr` if the surface is out of date (e.g., after a resize before the new swapchain is ready). Always check the return value in production code.

**Reference:** [SurfaceFrame API](../../api/gpu/SurfaceFrame.md), [Surface API -- beginFrame()](../../api/gpu/Surface.md)

### 6. Command buffer creation

```cpp
    auto cmd = graphicsQueue.createCommandBuffer("Frame command buffer");
```

Creates a new command buffer from the graphics queue. The string parameter is a debug label used by validation layers and GPU debugging tools.

Command buffers are recording containers for GPU commands. You record a sequence of operations, then submit the entire buffer to the GPU for execution in one batch. This is more efficient than submitting individual commands.

**Reference:** [Queue API -- createCommandBuffer()](../../api/gpu/Queue.md), [CommandBuffer API](../../api/gpu/CommandBuffer.md)

### 7. Image layout transitions

```cpp
    cmd->begin();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
```

GPU images must be in a specific layout for each operation. A layout transition tells the GPU how to reinterpret the image data:

| Layout | Purpose |
|--------|---------|
| `ColorAttachment` | Image will be used as a render target for color output |
| `DepthAttachment` | Image will be used as a render target for depth testing |
| `Present` | Image is ready to be displayed on screen |

The swapchain images start in an undefined or present layout. Before you can render into them, you must transition them to the attachment layouts. After rendering, you transition back to `Present` so the display controller can read them.

Without these transitions, the GPU may read stale data or produce visual artifacts. The `transition()` method records the necessary pipeline barriers to ensure correct ordering.

**Reference:** [CommandBuffer API -- transition()](../../api/gpu/CommandBuffer.md), [Image API -- currentLayout()](../../api/gpu/Image.md)

### 8. Rendering pass

```cpp
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, clearColor);
    cmd->clearDepth(1.0f);
    cmd->endRendering();
```

This defines the dynamic rendering scope:

1. **`beginRendering(frame)`** -- starts a rendering pass using the frame's color and depth images as attachments. The clear values set before this call are applied at the start of the pass.

2. **`clearColor(0, clearColor)`** -- sets the clear value for color attachment index 0. The first parameter is the attachment index (matching the framebuffer attachment order); the second is the RGBA clear color.

3. **`clearDepth(1.0f)`** -- sets the depth clear value to 1.0 (far plane). This means the depth buffer starts fully cleared and fragments will be tested against it.

4. **`endRendering()`** -- ends the rendering pass. The GPU resolves any pending operations on the attachments.

In this example, no draw calls are issued between `beginRendering()` and `endRendering()` -- the entire screen is simply cleared with the animated color. In a real application, you would bind pipelines and issue draw calls here.

**Reference:** [CommandBuffer API -- beginRendering()](../../api/gpu/CommandBuffer.md)

### 9. Final transition and presentation

```cpp
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());
```

The final four operations complete the frame:

| Step | Method | Purpose |
|------|--------|---------|
| 1 | `transition(color, Present)` | Make the color image available for display |
| 2 | `surface->present(cmd)` | Record presentation commands into the command buffer |
| 3 | `cmd->end()` | Finalize the command buffer for submission |
| 4 | `graphicsQueue.submit(cmd)` | Submit the recorded commands to the GPU for execution |
| 5 | `surface->endFrame(frame)` | Increment the frame counter and handle synchronization |

The `present()` call records the platform-specific presentation commands (e.g., Vulkan's `vkQueuePresentKHR` setup) into the command buffer. The actual presentation happens when the GPU processes the submitted commands.

`endFrame()` handles synchronization -- on Vulkan it manages fences and semaphores to ensure the GPU has finished rendering before the next `beginFrame()` reuses the image.

**Reference:** [Surface API -- present(), endFrame()](../../api/gpu/Surface.md), [Queue API -- submit()](../../api/gpu/Queue.md)

### 10. Cleanup

```cpp
    // 9. Cleanup
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();
```

Resources are released in reverse creation order, as explained in [02_device](02_device.md). `device->waitIdle()` blocks until the GPU finishes all pending work before destroying any resources.

## Key concepts

### The swapchain

A swapchain is a collection of images that the GPU cycles through for rendering and display. At any moment, one image is being displayed, one is being rendered to, and possibly one more is queued:

```
Image 0: Displayed  |  Rendering  |  Queued
Image 1: Queued     |  Displayed  |  Rendering
Image 2: Rendering  |  Queued     |  Displayed
```

`beginFrame()` picks the next image that is free to render into. This avoids conflicts between the display controller and the GPU. The typical swapchain has 2-3 images (double or triple buffering).

### Image layouts

Image layouts are GPU-specific memory configurations. Different operations require different layouts:

- **Undefined** -- initial state, contents are undefined
- **ColorAttachment** -- optimized for fragment shader writes
- **DepthAttachment** -- optimized for depth testing
- **Present** -- optimized for the display controller to read
- **TransferSrc/TransferDst** -- optimized for copy operations

Transitions are expensive because they may require copying or reformatting data. The GPU abstraction layer handles this automatically through `CommandBuffer::transition()`, but you must call it at the right points in the render loop.

### Command buffers

Command buffers batch GPU operations for efficiency. The workflow is:

1. **Create** -- allocate from a queue's command pool
2. **Begin** -- start recording
3. **Record** -- add transitions, rendering passes, draw calls
4. **End** -- stop recording
5. **Submit** -- send to the GPU for execution

A command buffer can only be submitted once. For the next frame, you create a new one. This is why `createCommandBuffer()` is called inside the loop rather than outside it.

### The rendering scope

Dynamic rendering (introduced in Vulkan 1.3 and used by this engine) replaces the older `VkRenderPass` concept. Instead of defining render passes at pipeline creation time, you specify the attachment images at draw time through `beginRendering(frame)`. This simplifies the API and makes render passes more flexible.

## Complete source

For reference, here is the full source of `examples/gpu/04_clear_loop/src/main.cpp`:

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <cmath>
#include <iostream>

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend to query window type
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. SDL init + create window based on backend window type
    app::initSdlVideoDriver();
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GPU Clear Loop Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 4. Create GPU instance
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create surface (shared_ptr for the frame lifecycle API)
    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // 8. Animated clear loop
    auto& graphicsQueue = device->graphicsQueue();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                surface->resize({ static_cast<uint32_t>(event.window.data1),
                                  static_cast<uint32_t>(event.window.data2) });
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
        gpu::Color clearColor {
            0.5f + 0.5f * std::sin(t),
            0.5f + 0.5f * std::sin(t + 2.0f),
            0.5f + 0.5f * std::sin(t + 4.0f),
            1.0f
        };

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

        cmd->begin();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, clearColor);
        cmd->clearDepth(1.0f);
        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 9. Cleanup
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

## Building and running

This example requires no shaders. Build it with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_clear_loop
```

Then run the binary from the `bin/` directory. You should see a window that cycles through colors continuously. Close the window or press the OS close button to exit.

## Next steps

- **[05_simple_triangle](05_simple_triangle.md)** -- Render geometry by binding a graphics pipeline and issuing draw calls.
- **[GPU API Reference](../../api/gpu/index.md)** -- Full class hierarchy, object creation flow, and backend-specific accessors.
- **[CommandBuffer API](../../api/gpu/CommandBuffer.md)** -- Complete list of recording methods (draw calls, compute dispatch, ray tracing).
