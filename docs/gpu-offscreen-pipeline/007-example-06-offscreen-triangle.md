# Step 007 — Example `examples/gpu/06_offscreen_triangle`

**Type:** Example
**Depends on:** 005 (Vulkan read-back), 006 (Metal read-back)
**Enables:** end-to-end validation of the offscreen pipeline

## Goal

A headless CLI example that reuses the `05_simple_triangle` pipeline and shaders but renders
into an **offscreen** color image, reads the pixels back with `gpu::Image::readPixelsRGBA8`,
and writes the result to `out.jpg` in the **current working directory** with
`bg2e::db::saveImage`. No SDL window is created.

This is the validation artifact for the whole plan: the *same* command-buffer recording as
example 05 (compute dispatch + triangle draw + push constants), executed against an
`OffscreenSurface`, then saved to disk.

## Files

```
examples/gpu/06_offscreen_triangle/
├── CMakeLists.txt
├── src/main.cpp
└── shaders/
    ├── glsl/                # copied verbatim from 05_simple_triangle/shaders/glsl
    │   ├── triangle.vert.glsl
    │   ├── triangle.frag.glsl
    │   └── noop.comp.glsl
    └── metal/
        └── triangle.metal   # copied verbatim from 05_simple_triangle/shaders/metal
```

> The shaders are identical to example 05; copy them so the example is self-contained
> (auto-glob compiles whatever is under the example's `shaders/`).

## CMakeLists.txt

Mirror `05_simple_triangle/CMakeLists.txt` but use **`bundle_app`** (no SDL — headless) which
also accepts `SHADERS_SRC`:

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_offscreen_triangle)
set(APP_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl")

set(METAL_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders/metal")
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app(
    TARGET_NAME ${APP_TARGET_NAME}
    SHADERS_SRC ${APP_SHADERS}
)

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${METAL_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}/metal")
endif()
```

> This is the **only** new CMake file. Auto-glob adds the example to the build; no edits to
> existing CMake.

## src/main.cpp (structure)

Based on `05_simple_triangle/src/main.cpp`, with these differences:

1. **No SDL / no window.** Remove all `SDL_*` calls and `app::SDLUtils`. Create the instance
   headless like example 03: `instance->create();`
2. **Offscreen surface** instead of window surface:
   ```cpp
   const gpu::Size2D size{ 800, 600 };
   auto surface = backend->createOffscreenSurface(instance, size); // RGBA8 + D32 by default
   ```
3. **Pipeline setup identical to 05** (shader modules, layouts, graphics + compute pipelines).
   `targetName = "gpu_offscreen_triangle"`. Color/depth formats come from
   `surface->colorFormat()` / `surface->depthFormat()`.
4. **Single render iteration** (no loop):
   ```cpp
   auto& graphicsQueue = device->graphicsQueue();

   PushConstants push{};
   push.color[0] = 1.0f; push.color[1] = 0.4f; push.color[2] = 0.1f; push.color[3] = 1.0f;
   gpu::Color clearColor{ 0.1f, 0.1f, 0.15f, 1.0f };

   auto frame = surface->beginFrame();
   auto cmd   = graphicsQueue.createCommandBuffer();

   cmd->begin();

   cmd->beginCompute();
   cmd->bindPipeline(computePipeline.get());
   cmd->dispatch(1, 1, 1);
   cmd->endCompute();

   cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
   cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
   cmd->beginRendering(frame.get());
   cmd->clearColor(0, clearColor);
   cmd->clearDepth(1.0f);
   cmd->bindPipeline(pipeline.get());
   cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
   cmd->draw(3);
   cmd->endRendering();
   // NOTE: no transition to Present — offscreen. Leave color image in ColorAttachment.

   surface->present(cmd.get());   // no-op for offscreen
   cmd->end();
   graphicsQueue.submit(cmd.get());
   surface->endFrame(frame.get());

   device->waitIdle();            // offscreen submit has no fence — wait before read-back
   ```
5. **Read back + save:**
   ```cpp
   const uint32_t w = surface->width();
   const uint32_t h = surface->height();
   std::vector<uint8_t> pixels;
   frame->colorImage()->readPixelsRGBA8(pixels, gpu::ImageLayout::ColorAttachment);

   auto outPath = std::filesystem::current_path() / "out.jpg";
   bg2e::db::saveImage(outPath, pixels.data(), w, h, 4);
   std::cout << "Wrote " << outPath << " (" << w << "x" << h << ")\n";
   ```
6. **Cleanup** as in 05 (pipelines, layouts, shader modules, surface, device, instance) — but
   no `SDL_DestroyWindow` / `SDL_Quit`.

## Includes

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>     // PlatformTools (shaderPath, currentPlatform)
#include <bg2e/db/image.hpp>     // saveImage
#include <filesystem>
#include <vector>
#include <iostream>
```

## Notes / pitfalls

- The color image is left in `ImageLayout::ColorAttachment` after `endRendering`; that's the
  `currentLayout` passed to `readPixelsRGBA8`, which transitions it to `TRANSFER_SRC` and back
  internally.
- `waitIdle()` is required because the offscreen `Queue::submit` path uses no fence
  (`lib/src/bg2e/gpu/vk/Queue.cpp`). Without it the render may not have completed before the
  read-back's `immediateSubmit`. (The read-back copy itself is synchronous.)
- The compute dispatch is kept purely to exercise the compute path offscreen; it doesn't affect
  the saved image.
- `bg2e::db::saveImage` infers the format from the extension (`.jpg`); 4 bpp RGBA is accepted.

## Validation

- Build the engine + examples; run `gpu_offscreen_triangle`.
- Vulkan (Linux/Windows/macOS) and Metal (macOS): an `out.jpg` appears in the working
  directory, 800×600, showing the triangle (push-constant color) over the clear color.
- No validation errors; clean shutdown.
