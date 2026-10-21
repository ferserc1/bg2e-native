# Tutorial 01: Window and UI with the Draw Draw API

This tutorial walks through the `01_window_ui` example: opening a window,
clearing a retained scene image with `bg2e::gpu` commands, and presenting an
interactive interface through `bg2e::ui`. It introduces the application and
render delegate lifecycle used by the `bg2e::draw` framework.

**Source:** [examples/draw/01_window_ui/src/main.cpp](../../../examples/draw/01_window_ui/src/main.cpp)

## What you will learn

- How to select the draw execution path through `Application` and `MainLoop`
- How to select Vulkan or Metal with `draw::EngineConfig`
- How to record GPU commands inside a `draw::RenderLoopDelegate`
- How to share state between scene and UI delegates
- How to build controls using the `bg2e::ui` wrappers
- How scene invalidation, pause/resume, and retained composition work

## Prerequisites

- A configured bg2e-native build environment with the required Vulkan SDK
- Familiarity with C++ classes, delegates, and `std::shared_ptr`
- Optional: [GPU Clear Loop](../gpu/04_clear_loop.md) for lower-level frame recording

The example does not require application scene shaders. The engine supplies
its own UI rendering resources.

## Understanding the example

The window contains a background color, the ImGui demo, and a control window.
The control window displays the selected backend, a UI frame counter, and a
scene update counter. It also provides Background, Pause scene, and Refresh
scene controls.

`bg2e::draw` is built over `bg2e::gpu`. Both MainLoop overloads now use draw.
These examples pass configuration explicitly to support backend overrides.
Legacy shared engine APIs remain until subsequent cleanup.

The scene is rendered into a retained image. On subsequent presentation frames,
draw copies that image to the acquired presentation image and composes the UI.
The scene delegate runs again when the scene is invalidated, allowing the UI
to continue while scene rendering is paused or unchanged.

## Step-by-step code explanation

### 1. Headers and includes

```cpp
#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/DemoWindow.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Window.hpp>
```

| Header | Purpose |
|--------|---------|
| `Application.hpp` | Register the render, input, and UI delegates |
| `MainLoop.hpp` | Window configuration and application execution |
| `RenderLoopDelegate.hpp` | Scene callbacks and the borrowed frame context |
| `Button.hpp`, `Value.hpp` | Checkbox, button, and color picker wrappers |
| `Text.hpp`, `Window.hpp` | Information text and the control window |
| `DemoWindow.hpp` | Show the ImGui demo through an engine wrapper |

All interface calls use `bg2e::ui`. The application does not include ImGui
headers or call ImGui functions directly.

### 2. Share state through a controller

```cpp
struct Controller {
    app::MainLoop& loop;
    base::Color color{0.12f, 0.18f, 0.28f, 1.0f};
    bool paused = false;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
};
```

The application constructs one shared controller and passes it to both
delegates. The controller holds a reference to the running loop, the selected
background color, the pause flag, two counters, and a backend label.

Both delegates access this state on the main thread. The reference to the loop
remains valid because `main()` creates the loop before the application.

### 3. Record scene commands

```cpp
void render(const draw::FrameContext& context) override
{
    const auto& color = _controller->color;
    context.commandBuffer.beginRendering(&context.colorTarget);
    context.commandBuffer.clearColor(0, {color.r, color.g, color.b, color.a});
    context.commandBuffer.endRendering();
    ++_controller->sceneFrames;
}
```

The frame context supplies an already-recording command buffer and the retained
scene color image in `ColorAttachment` layout. The delegate opens its rendering
scope, clears attachment 0, and closes the scope before returning.

The delegate does not acquire presentation frames, begin or submit command
buffers, or present the surface. Draw owns those operations. `sceneFrames`
counts calls to this scene callback, rather than all presentation frames.

| FrameContext field | Meaning |
|--------------------|---------|
| `engine` | Current draw engine and access to its GPU device |
| `commandBuffer` | Borrowed command buffer for the current operation |
| `colorTarget` | Retained scene image |
| `extent` | Scene image dimensions |
| `frameNumber` | Surface frame counter |
| `frameSlot` | Current in-flight frame slot |
| `deltaSeconds` | Current frame time interval in seconds |

Do not store references from `FrameContext` for later frames.

**Reference:** [RenderLoopDelegate](../../api/draw/RenderLoopDelegate.md), [FrameContext](../../api/draw/FrameContext.md), [CommandBuffer](../../api/gpu/CommandBuffer.md)

### 4. Configure the information window

```cpp
void init(draw::Engine*, ui::UserInterface*) override
{
    _window.setTitle("Draw draw controls");
    _window.setPosition(20, 20);
    _window.setSize(380, 300);
    _window.options.noClose = true;
}
```
`noClose` keeps the control window available. The UI delegate receives draw's
engine in this overload, although this example only needs its shared controller.
Each `drawUI()` call increments `uiFrames`, invokes `ui::DemoWindow::draw()`, and
uses `_window.draw(...)` to emit the controls and information text.

**Reference:** [UserInterface](../../api/ui/UserInterface.md), [Window](../../api/ui/Window.md)

### 5. Invalidate the scene when controls change

```cpp
if (ui::Value::colorPicker("Background", state.color))
{
    state.loop.requestSceneFrame();
}
if (ui::Button::button("Refresh scene"))
{
    state.loop.requestSceneFrame();
}
```

Changing a value in the controller does not by itself redraw the retained scene.
`requestSceneFrame()` marks it dirty and requests presentation. A plain
`requestFrame()` requests presentation without invalidating the scene.

The example does not request continuous scene updates: after the first valid
render, the scene counter remains stable until a control or resize invalidates
it. The UI counter continues advancing as presentation frames are processed.

### 6. Pause and resume the retained scene

```cpp
if (ui::Button::checkBox("Pause scene", &state.paused))
{
    if (state.paused)
    {
        state.loop.pauseScene({state.color.r, state.color.g, state.color.b, state.color.a});
    }
    else
    {
        state.loop.resumeScene();
    }
}
```

Pause preserves the last valid scene image while the UI remains interactive.
Color changes and refresh requests made during the pause stay pending until
resume. The Refresh scene button does not force a render while paused.

Resizing replaces the retained image, so the previous image cannot be preserved
at the new extent. While paused, draw initializes the replacement using the
clear color supplied to `pauseScene()`. In this example, that color is captured
when Pause scene is enabled; later picker changes do not update that pause
clear color. Resuming renders the controller's currently selected color.

**Reference:** [MainLoop](../../api/app/MainLoop.md), [RenderLoop](../../api/draw/RenderLoop.md)

### 7. Select the backend and install delegates

```cpp
_controller->backend = _config.backend == gpu::BackendType::Metal ? "Metal" : "Vulkan";
setRenderDelegate(std::make_shared<SceneDelegate>(_controller));
setInputDelegate(std::make_shared<app::InputDelegate>());
setUiDelegate(std::make_shared<UiDelegate>(_controller));
```

The application owns a `draw::EngineConfig`. Its default backend is Metal on
macOS and Vulkan on other platforms. Command-line parsing accepts one optional
`--backend=vulkan` or `--backend=metal` argument. Unknown arguments, repeated
backend selection, and Metal on a non-macOS platform raise descriptive errors
before the window and GPU context are created.

The render delegate's type selects the draw delegate registration overload.
The default input delegate provides the application's input callback object;
UI event handling is managed by the application loop.

### 8. Enter the draw execution path

```cpp
bg2e::app::MainLoop loop("org.bg2e.examples.draw.window-ui");
Application application(loop);
application.init(argc, argv);
loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: window and UI", 1280, 720));
return loop.run(&application, application.config());
```

Passing `draw::EngineConfig` explicitly to `run()` configures the draw
execution path. MainLoop creates the window and draw engine, initializes the
delegates, processes events, presents frames, and coordinates shutdown.

The example does not initialize `gpu::Factory` or construct an independent SDL
render loop. Errors reach `main()`'s exception handler, which reports the
`draw_window_ui` prefix and returns a nonzero exit status.

**Reference:** [Application and input](../../api/app/Application_and_input.md), [EngineConfig](../../api/draw/EngineConfig.md), [Engine](../../api/draw/Engine.md)

## Key concepts

### Scene rendering and UI presentation have separate lifecycles

The retained image lets presentation continue without invoking the scene
callbacks. The two counters make this behavior visible and provide a simple
way to verify pause and invalidation.

### Command scope ownership

The delegate closes every compute or rendering scope it opens. Draw verifies
that callbacks return without an active scope, then performs retained-image
copying, UI composition, and presentation.

### Main-thread state changes

UI controls run on the main thread. If future examples introduce workers,
queue scene changes with `MainLoop::safeUpdateScene()` instead of directly
mutating the shared controller from a worker.

## Complete source

The complete application is available in
[main.cpp](../../../examples/draw/01_window_ui/src/main.cpp).
Its [CMakeLists.txt](../../../examples/draw/01_window_ui/CMakeLists.txt)
registers the `draw_window_ui` target through `bundle_app_sdl()`.

## Building and running

Use an already configured build directory with the required Vulkan SDK and
platform generator: Ninja on Linux, Xcode on macOS for native file dialogs,
and Visual Studio 17 2022 on Windows. Replace `build` below with your configured
build directory when necessary.

```sh
cmake --build build --target draw_window_ui --config Debug
```

On macOS, run the executable inside the bundle:

```sh
./bin/macos/draw_window_ui.app/Contents/MacOS/draw_window_ui --backend=metal
./bin/macos/draw_window_ui.app/Contents/MacOS/draw_window_ui --backend=vulkan
```

On Linux, run `./bin/linux/draw_window_ui --backend=vulkan`.
On Windows, run the `draw_window_ui.exe` executable under `bin/windows`.

To inspect the retained-scene behavior:

1. Change Background and observe a scene update.
2. Enable Pause scene and observe that UI frames continue increasing.
3. Change Background and press Refresh scene; the retained scene stays visible.
4. Disable Pause scene; the new background appears.
5. Resize while paused to inspect the replacement image's pause clear color.

## Next steps

- [02_simple_triangle](02_simple_triangle.md) — Add textured geometry, compute shaders, and animation.
- [Draw API Reference](../../api/draw/index.md) — Engine lifecycle and retained composition.
- [Tutorial index](index.md) — Browse the draw example series.
