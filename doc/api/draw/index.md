# bg2e::draw

`bg2e::draw` is the experimental high-level graphics framework built around the
abstract `bg2e::gpu` API. `bg2e::render` remains the production framework and is
not deprecated. The two frameworks have separate graphics delegate contracts
and share the application entry points in `bg2e::app`.

```cpp
#include <bg2e/draw/all.hpp>
```

## Production and experimental frameworks

The terms **production** and **experimental** describe the maturity and scope of
bg2 engine's graphics APIs, not build configurations, deployment environments
or compiler optimization settings.

- **Production: `bg2e::render`.** This is the maintained framework used by the
  engine's applications. It provides Vulkan rendering and integrates with the
  production scene, material and UI components. Use it for applications that
  need those established high-level facilities. Production does not imply that
  an application must run on a server or use a Release build.
- **Experimental: `bg2e::draw`.** This is a high-level framework built on the
  backend-neutral `bg2e::gpu` API. Its windowed path supports Vulkan and Metal,
  retained scene color and separate UI composition. Its high-level scene and
  resource contracts have a narrower scope than render and can evolve as the
  API develops; render components cannot simply be passed to draw. Use it when
  evaluating the multi-backend architecture or building against its GPU-based
  delegate contract. Experimental does not mean the code is only a mock or that
  its window/UI path is unavailable.

Draw is intended to become the successor to render. Both APIs coexist; render
remains supported and is not deprecated. `bg2e::gpu` provides lower-level devices,
queues, commands and resources; it does not supply render's high-level scene
framework. Choosing Vulkan in draw does not select render.

## Window, scene and UI rendering

Engine initializes the selected GPU context. RenderLoop coordinates acquisition,
retained scene color, UI composition and presentation with Vulkan or Metal. The
[window/UI example](../../../examples/draw/README.md) uses Application/MainLoop
and provides scene controls independent of UI updates. UI event processing and
frame overrides require successful backend initialization.

## Responsibilities

- [Engine](Engine.md): GPU context ownership and global lifecycle through PImpl.
- [RenderLoop](RenderLoop.md): scene work, UI composition and presentation
  coordination, owning retained scene color and per-slot command/frame wrappers.
- [RenderLoopDelegate](RenderLoopDelegate.md): GPU-based callbacks without
  production Vulkan types.
- [FrameContext](FrameContext.md): borrowed references and metadata for scene work.
- [EngineConfig](EngineConfig.md): low-level backend and initial formats.

Resources updated per in-flight frame are intended to live in the consuming
objects as persistent reusable slots. There is no draw delegate callback for
central per-frame descriptor allocation. The slot must be safe to reuse before
an object updates its resources. Surface acquisition completes the previous
work in that slot before RenderLoop delivers FrameContext.

FrameContext's color target represents the retained scene image. The design
allows scene updates to pause while UI and presentation continue using the last
scene result. Optional UI composition is available with Vulkan and Metal.

## Application entry point

`run(application)` selects production render. `run(application, config)` selects
draw, independent of the low-level backend. A default `EngineConfig` chooses
Metal on macOS and Vulkan elsewhere; explicit assignment overrides this choice.
The private execution wrapper is constructed by MainLoop, not the application.

See [quick start](quick_start.md), [API reference](reference.md),
[MainLoop selection](../app/MainLoop.md#execution-selection-and-validation) and
[GPU API](../gpu/index.md). OffscreenApplication and render-dependent scene/UI
components use the render API and do not accept draw context/resources.

## Implementation details

Read [MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md) for
source-backed engine/coordination/UI ownership and frame flow, and
[GPU submission tracking](../gpu/Submission_tracking_and_waitIdle.md) for
asynchronous sends, resource-slot reuse and safe cleanup.
