# Draw Configuration Quick Start

These snippets use the draw application path. The complete
[window/UI example](../../../examples/draw/01_window_ui/src/main.cpp) registers
all delegates, accepts backend overrides and provides retained-scene controls.
See its [usage notes](../../../examples/draw/README.md).

## Include and configure

```cpp
#include <bg2e/app/all.hpp>
#include <bg2e/draw/all.hpp>

bg2e::draw::EngineConfig config;
// Metal on macOS, Vulkan on other platforms.
config.debug = true;
```

An explicit override remains possible:

```cpp
config.backend = bg2e::gpu::BackendType::Vulkan;
```

## Define the draw delegate contract

```cpp
class DrawDelegate final : public bg2e::draw::RenderLoopDelegate {
public:
    void render(const bg2e::draw::FrameContext& context) override
    {
        // An empty delegate presents the coordinator's background color.
        // For scene work, record commands targeting context.colorTarget and
        // close every rendering/compute scope before returning.
    }
};
```

## Register and select

Given an initialized Application with non-null input and UI delegates:

```cpp
application.setRenderDelegate(std::make_shared<DrawDelegate>());
bg2e::app::MainLoop loop("org.example.draw");
loop.initWindowConfig(bg2e::app::WindowConfig::withSize("Draw", 1280, 720));
loop.run(&application, config);
```

The call validates configuration, creates the selected backend and window,
and presents scene color. Input and UI delegates are required by the
application contract. Both Vulkan and Metal initialize their UI backend;
Metal is available only on macOS.
Using `loop.run(&application)` uses draw with platform-default EngineConfig.

Both overloads require the registered draw graphics delegate. See [MainLoop](../app/MainLoop.md) and [EngineConfig](EngineConfig.md).

See [framework scope and maturity](index.md#production-framework)
for the relationship between draw and GPU backends.

For a scene renderer rather than custom command recording, use the
[deferred scene tutorial](../../tutorials/draw/10_deferred_renderer.md).
