# Draw Configuration Quick Start

These snippets describe milestone 01 contracts. They are not a runnable draw
window tutorial: experimental execution intentionally stops before allocation.

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

## Define the experimental delegate contract

```cpp
class DrawDelegate final : public bg2e::draw::RenderLoopDelegate {
public:
    void render(const bg2e::draw::FrameContext& context) override
    {
        // Record scene work through context.commandBuffer and colorTarget
        // when frame execution becomes available in milestone 02.
    }
};
```

## Register and select

Given an initialized Application with non-null input and UI delegates:

```cpp
application.setRenderDelegate(std::make_shared<DrawDelegate>());
bg2e::app::MainLoop loop("org.example.experimental");
loop.initWindowConfig(bg2e::app::WindowConfig::withSize("Draw", 1280, 720));
loop.run(&application, config);
```

The call validates configuration and throws
`std::logic_error("Experimental draw execution requires milestone 02")`.
No window is created. Using `loop.run(&application)` instead selects production
and throws std::invalid_argument for the draw delegate mismatch.

The existing production entry point remains unchanged and requires a production
render delegate. See [MainLoop](../app/MainLoop.md) and [EngineConfig](EngineConfig.md).
