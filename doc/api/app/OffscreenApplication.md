# OffscreenApplication

**Header:** `<bg2e/app/OffscreenApplication.hpp>`  
**Namespace:** `bg2e::app`

`OffscreenApplication` runs a render delegate without an SDL window or
swapchain. It is intended for command-line rendering, exports, and tests that
produce GPU images without an interactive application.

## Configuration

`OffscreenApplicationConfig` selects fixed width and height, color/depth image
creation, and the color format. The defaults are 1920×1080, color and depth
enabled, and `VK_FORMAT_R8G8B8A8_UNORM`.

## Delegate lifecycle

`OffscreenApplicationDelegate` receives:

1. `initConfig()` to parse arguments and fill the configuration;
2. `init()` after engine and target creation;
3. `initFrameResources()` and `initScene()`;
4. one fixed-size `resize()` notification;
5. `frame()` and `render()` for every frame;
6. `didRenderFrame()` after GPU completion;
7. `cleanup()` before shutdown.

`render()` returns `true` to continue or `false` to finish. Offscreen
applications do not use the interactive `MainLoop`, window focus, or its
background frame-rate limiter.

