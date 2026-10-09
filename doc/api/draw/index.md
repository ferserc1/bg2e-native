# bg2e::draw

`bg2e::draw` is the experimental high-level graphics framework built around the
abstract `bg2e::gpu` API. `bg2e::render` remains the production framework and is
not deprecated. The two frameworks have separate graphics delegate contracts
and share the application entry points in `bg2e::app`.

```cpp
#include <bg2e/draw/all.hpp>
```

## Milestone 02 implementation status

Engine initialization, retained scene presentation and UI integration are
implemented for Vulkan and Metal. The [window/UI example](../../../examples/draw/README.md)
uses Application/MainLoop and exposes scene controls independent of UI updates.
UI event processing and frame overrides are enabled only after backend
initialization. Project-lead compilation and runtime acceptance are pending;
this status records implementation, not verified runtime results.

## Responsibilities

- [Engine](Engine.md): GPU context ownership and global lifecycle through PImpl.
- [RenderLoop](RenderLoop.md): scene work, UI composition and presentation
  coordination, owning retained scene color and per-slot command/frame wrappers.
- [RenderLoopDelegate](RenderLoopDelegate.md): new GPU-based callbacks without
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
scene result. Retained scene presentation and pause are implemented; optional
UI composition is available with Vulkan and Metal.

## Application entry point

`run(application)` selects production render. `run(application, config)` selects
draw, independent of the low-level backend. A default `EngineConfig` chooses
Metal on macOS and Vulkan elsewhere; explicit assignment overrides this choice.
The private execution wrapper is constructed by MainLoop, not the application.

See [quick start](quick_start.md), [API reference](reference.md),
[MainLoop selection](../app/MainLoop.md#execution-selection-and-validation) and
[GPU API](../gpu/index.md). OffscreenApplication and render-dependent scene/UI
components have not been migrated by this milestone.
