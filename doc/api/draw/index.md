# bg2e::draw

`bg2e::draw` is the high-level graphics framework built on `bg2e::gpu`.
MainLoop and OffscreenApplication now use draw exclusively, over Vulkan or Metal.

```cpp
#include <bg2e/draw/all.hpp>
```

## Production framework

Draw is the production high-level renderer over Vulkan or Metal. Shared scene,
DB, UI and manipulation APIs use draw resources exclusively. Retired rendering
headers and application targets have been removed without compatibility aliases.

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

Both `run(application)` and `run(application, config)` select draw. A default `EngineConfig` chooses
Metal on macOS and Vulkan elsewhere; explicit assignment overrides this choice.
The private execution wrapper is constructed by MainLoop, not the application.

See [quick start](quick_start.md), [API reference](reference.md),
[MainLoop selection](../app/MainLoop.md#execution-selection-and-validation) and
[GPU API](../gpu/index.md). [OffscreenApplication](../app/OffscreenApplication.md)
uses the typed draw path exclusively. DrawableComponent::drawable() exposes the canonical draw resource.

## Implementation details

Read [MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md) for
source-backed engine/coordination/UI ownership and frame flow, and
[GPU submission tracking](../gpu/Submission_tracking_and_waitIdle.md) for
asynchronous sends, resource-slot reuse and safe cleanup.

## Deferred scene API

Use [RendererDeferred](RendererDeferred.md) with the non-template
DefaultRenderLoopDelegate for scene rendering, [resources](Resources.md),
[environment](Environment.md) and [live settings/upscaling](Settings.md).
Start with the [deferred tutorial](../../tutorials/draw/10_deferred_renderer.md).
For selection, gizmos, material edits and viewpoint tools, continue with the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md) and the
[manipulation API](../manipulation/index.md).

See [RTAmbientOcclusion](RTAmbientOcclusion.md) for the optional public scalar AO pass and shared tracing snapshots.

See [RTGlobalIllumination](RTGlobalIllumination.md) for the optional diffuse HDR GI pass and IBL/AO/GI mode selection.

See [RTReflections](RTReflections.md) for output-relative traced HDR specular radiance and confidence-based IBL composition.

See [TemporalAccumulator](TemporalAccumulator.md) for independent rendered-view effect histories, borrowed public surfaces, camera motion, bounded convergence and invalidateHistories application notifications.

See [DenoiseFilter](DenoiseFilter.md) for reusable scalar/HDR bilateral filtering after temporal accumulation, explicit surface guides, confidence semantics and live accessor metadata.

- [Optional SMAA final processing](SMAAProcessor.md)

## Advanced effect contracts

- [SurfaceInputs](SurfaceInputs.md): camera reconstruction and explicit world surfaces.
- [TracingSceneResources](TracingSceneResources.md): packed tables and retained snapshots.
- [RTAmbientOcclusion](RTAmbientOcclusion.md), [RTGlobalIllumination](RTGlobalIllumination.md), [RTReflections](RTReflections.md).
- [TemporalAccumulator](TemporalAccumulator.md), [MotionVectorGenerator](MotionVectorGenerator.md), [DenoiseFilter](DenoiseFilter.md).
- [FinalProcessor and SMAAProcessor](SMAAProcessor.md).
- [Advanced lighting architecture](../../architecture/draw_advanced_lighting.md).

The separate [bake namespace](../bake/index.md) provides validated settings/pixels,
Device-backed UV surfaces, integrated AO/GI execution, exact accumulation and
composition/dilation, finished maps and CPU UV diagnostics. The
[lightmap tutorial](../../tutorials/draw/12_lightmaps.md) follows actual material
adoption and UI ownership. [Standalone baking](../bake/StandaloneBakeBatch.md) now
adds camera-less serial UV jobs and protected complete-generation file/model
publication; see the [standalone tutorial](../../tutorials/draw/standalone_lightmaps.md).

See [Baked indirect maps](BakedIndirectMaps.md) for explicit RGB lightmap metadata,
UV2 sampling, HDR G-buffer storage and application through the AO texture slot.

## Headless scene jobs

- [DefaultOffscreenApplicationDelegate](DefaultOffscreenApplicationDelegate.md):
  exact completed camera samples, frozen state, settings import and final output.
- [FrameAccumulator](FrameAccumulator.md): public synchronous linear RGB mean.
- [OffscreenApplication](../app/OffscreenApplication.md): typed execution lifecycle.
- [Offscreen tutorial](../../tutorials/draw/13_offscreen_application.md) and
  [CLI/build guide](../../apps/draw_offscreen_application.md).

- [Draw Composer](../../apps/draw_composer.md) and [scene ownership API](../scene/index.md): full scene editing/import/bake integration.


- [Standalone UV bake CLI](../../apps/draw_lightmap_generator.md),
  [C++ tutorial](../../tutorials/draw/standalone_lightmaps.md) and
  [batch publication architecture](../../architecture/draw_standalone_lightmaps.md).

- [Renderer configuration and injection](Renderer.md)
- [Migration](../../migration_to_draw.md)

- [Upscaler: FSR, MetalFX and bilinear reconstruction](Upscaler.md)
- [Depth- and normal-aware GI upsampling](GIUpsampling.md)
- [Deferred rendering model and resolution domains](../../deferred_render_model.md)

## Resource reuse and diagnostics

- [ImageLoadContext](ImageLoadContext.md): import-scoped CPU decoding and GPU image reuse with independent texture settings/samplers.
- [TemporaryResourcePool](TemporaryResourcePool.md): recording completion, resource leases and bounded retention.
- [TemporalInvalidation](TemporalInvalidation.md): lazy signatures and explicit invalidation.
- [TransparentRegions](TransparentRegions.md): conservative raster/composition bounds and full fallbacks.
- [UpdateSynchronization](UpdateSynchronization.md): typed final-color updates and conservative generic callbacks.
- [CPU/GPU profiling](../../architecture/cpu_profiling.md): collection, recording, native granularity and Performance UI ownership.
