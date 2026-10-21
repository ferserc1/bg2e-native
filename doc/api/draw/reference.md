# Draw API Reference

The high-level successor to production render uses the abstract
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
Vulkan and Metal using private adapters. The loop supplies retained color; RendererDeferred owns its internal depth/G-buffer graph.
[OffscreenApplication](../app/OffscreenApplication.md) accepts only its typed draw
delegate. UI uses draw resources. Per-object persistent resource slots replace
central transient allocation.

See [MainLoop](../app/MainLoop.md), [architecture](../../architecture/MainLoop_render_draw_gpu.md),
[GPU synchronization](../gpu/Submission_tracking_and_waitIdle.md) and
[window/UI example](../../../examples/draw/README.md).

## Scene renderer and resources

- [RendererDeferred and DefaultRenderLoopDelegate](RendererDeferred.md): runtime ownership, lifecycle and public pipeline replacement.
- [Drawable, material, scene and bindings](Resources.md): canonical ownership, deep clones and safe edits.
- [Environment tools](Environment.md): public conversion/IBL and HDR precision.
- [Settings and upscaling](Settings.md): live catalog, codec/facade/UI, resolution and optional FSR.

## Advanced effect contracts

- [SurfaceInputs](SurfaceInputs.md): camera reconstruction and explicit world surfaces.
- [TracingSceneResources](TracingSceneResources.md): packed tables and retained snapshots.
- [RTAmbientOcclusion](RTAmbientOcclusion.md), [RTGlobalIllumination](RTGlobalIllumination.md), [RTReflections](RTReflections.md).
- [TemporalAccumulator](TemporalAccumulator.md), [MotionVectorGenerator](MotionVectorGenerator.md), [DenoiseFilter](DenoiseFilter.md).
- [FinalProcessor and SMAAProcessor](SMAAProcessor.md).
- [Advanced lighting architecture](../../architecture/draw_advanced_lighting.md).

[Integrated bake](../bake/index.md) supplies actual UV2 AO/GI maps independently
of a camera renderer. [BakedIndirectMaps](BakedIndirectMaps.md) documents AO-slot
mode metadata, live/stored adoption and the updated HDR G-buffer format.

## Accumulated headless output

- [DefaultOffscreenApplicationDelegate](DefaultOffscreenApplicationDelegate.md):
  renderer/scene owner, settings precedence and exact completed samples.
- [FrameAccumulator](FrameAccumulator.md): synchronous CPU double linear mean
  over completed RGBA16F samples, final upload and count validation.
- [Offscreen scene tutorial](../../tutorials/draw/13_offscreen_application.md).
