# Draw API Reference

The experimental high-level successor to production render uses the abstract
gpu backend. The umbrella header is `<bg2e/draw/all.hpp>`; production render is
supported and is not deprecated.

| Symbol | Header | Responsibility |
|---|---|---|
| [EngineConfig](EngineConfig.md) | `EngineConfig.hpp` | Backend, debug, name and requested formats; Metal default on macOS, Vulkan elsewhere. |
| [Engine](Engine.md) | `Engine.hpp` | GPU context initialization, backend lease and staged cleanup via PImpl. |
| [FrameContext](FrameContext.md) | `FrameContext.hpp` | Borrowed commands/color target and actual extent, frame number/slot and seconds delta. |
| [RenderLoopDelegate](RenderLoopDelegate.md) | `RenderLoopDelegate.hpp` | GPU-based init/scene/resize/update/render/cleanup callbacks. |
| [RenderLoop](RenderLoop.md) | `RenderLoop.hpp` | Retained scene, slot wrappers, invalidation/pause, UI callbacks and asynchronous presentation. |

MainLoop's config overload constructs the private draw adapter; applications do
not initialize Factory or expose GraphicsExecution. UI initialization supports
Vulkan and Metal using private adapters. The scene contract is color-only;
production scene/UI components and OffscreenApplication use render-compatible
resources and delegates. Per-object persistent resource rings replace a central transient frame
resource allocation contract.

See [MainLoop](../app/MainLoop.md), [architecture](../../architecture/MainLoop_render_draw_gpu.md),
[GPU synchronization](../gpu/Submission_tracking_and_waitIdle.md) and
[window/UI example](../../../examples/draw/README.md).
