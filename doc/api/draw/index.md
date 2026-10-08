# bg2e::draw

`bg2e::draw` is the experimental high-level graphics framework built around the
abstract `bg2e::gpu` API. `bg2e::render` remains the production framework and is
not deprecated. The two frameworks have separate graphics delegate contracts
and share the application entry points in `bg2e::app`.

```cpp
#include <bg2e/draw/all.hpp>
```

## Milestone 01 status

This milestone provides public contracts and private MainLoop execution
selection. GPU initialization and frame execution are not implemented yet.
A correctly configured `MainLoop::run(application, config)` throws
`std::logic_error("Experimental draw execution requires milestone 02")` before
SDL/GPU allocation. Direct Engine/RenderLoop startup also throws as documented
on their class pages. This module does not yet provide a runnable window/UI
example.

## Responsibilities

- [Engine](Engine.md): GPU context ownership and global lifecycle, represented
  by a PImpl shell in this milestone.
- [RenderLoop](RenderLoop.md): scene work, UI composition and presentation
  coordination, with working coordination flags and pending frame execution.
- [RenderLoopDelegate](RenderLoopDelegate.md): new GPU-based callbacks without
  production Vulkan types.
- [FrameContext](FrameContext.md): borrowed references and metadata for scene work.
- [EngineConfig](EngineConfig.md): low-level backend and initial formats.

Resources updated per in-flight frame are intended to live in the consuming
objects as persistent reusable slots. There is no draw delegate callback for
central per-frame descriptor allocation. The slot must be safe to reuse before
an object updates its resources; these contracts do not themselves implement
GPU synchronization.

FrameContext's color target represents the retained scene image. The design
allows scene updates to pause while UI and presentation continue using the last
scene result. The retained image and this execution behavior are planned for
milestone 02; milestone 01 only stores pause/dirty/resize state.

## Application entry point

`run(application)` selects production render. `run(application, config)` selects
draw, independent of the low-level backend. A default `EngineConfig` chooses
Metal on macOS and Vulkan elsewhere; explicit assignment overrides this choice.
The private execution wrapper is constructed by MainLoop, not the application.

See [quick start](quick_start.md), [API reference](reference.md),
[MainLoop selection](../app/MainLoop.md#execution-selection-and-validation) and
[GPU API](../gpu/index.md). OffscreenApplication and render-dependent scene/UI
components have not been migrated by this milestone.
