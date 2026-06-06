# Step 6 — Example & validation (OPTIONAL — outside `bg2e::gpu`)

**Status:** *optional and explicitly outside the required scope.* The mandatory plan (steps 01–05)
only touches `bg2e::gpu`. This step adds a runnable example so the loop can be validated end-to-end;
it does **not** modify any existing engine code. Skip it if you only want the `gpu` implementation.

After steps 01–05 the `bg2e::gpu` API matches the design sketch exactly, so the example is the
sketch with the initialization section filled in from `examples/gpu/02_device/src/main.cpp`.

## Files (all new — additive only)

- **New** `examples/gpu/04_clear_loop/CMakeLists.txt`
- **New** `examples/gpu/04_clear_loop/src/main.cpp`

> Creating a new example directory requires its own `CMakeLists.txt` (a *new* file, not a
> modification of an existing CMake file). The source is auto-globbed by the engine's CMake setup.
> If the project policy forbids adding the CMake file, validation can instead be done by temporarily
> dropping the loop body into `02_device` locally — but the dedicated example is the clean option.

## `CMakeLists.txt`

Mirror `examples/gpu/02_device/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_clear_loop)
bundle_app_sdl(
    TARGET_NAME ${APP_TARGET_NAME}
)
```

## `src/main.cpp`

Take the existing initialization block from `02_device` (backend selection, SDL init, window
creation, `instance->create(window)`, `createWindowSurface`, `createPhysicalDevice` + `choose`,
`createDevice` + `create`) verbatim, then replace the print/cleanup tail with the animated clear
loop from the design sketch:

```cpp
auto& graphicsQueue = device->graphicsQueue();

bool running = true;
while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        // (optional) handle SDL_WINDOWEVENT_RESIZED -> surface->resize({w,h});
    }

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
    gpu::Color clearColor {
        0.5f + 0.5f * std::sin(t),
        0.5f + 0.5f * std::sin(t + 2.0f),
        0.5f + 0.5f * std::sin(t + 4.0f),
        1.0f
    };

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer();

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

device->waitIdle();
surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

> `surface` here should be declared as `std::shared_ptr<gpu::Surface>` (the sketch's type). Because
> `createWindowSurface` returns `std::unique_ptr<gpu::WindowSurface>`, convert it:
> `std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);` (unique→shared
> is an implicit move). The device/physicalDevice creation still takes `surface.get()`.

## Validation checklist

- **Vulkan (Linux/macOS/Windows):** window shows a smoothly cycling color; validation layers
  (debug build) report no errors across acquire → barrier → dynamic rendering → present → submit.
- **Metal (macOS):** same visible result; the drawable is presented every frame and the color
  oscillates. No `MTL` validation errors (enable Metal API validation in the scheme if desired).
- **Resize:** if the optional resize handler is wired, dragging the window edge recreates the
  swapchain / drawable size + depth buffer without crashing.
- **Clean shutdown:** closing the window exits the loop; `waitIdle`/`cleanup` run with no leaks
  (Vulkan: no destroy-order validation errors; Metal: no live `MTL` object warnings).

## Build check

Purely additive new files; no existing target changes. The rest of the engine is untouched.
