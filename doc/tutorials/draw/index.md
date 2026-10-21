# Experimental Draw API Tutorials

This directory contains step-by-step tutorials for `bg2e::draw`, the
experimental application rendering framework built over `bg2e::gpu`.

The numbered tutorials correspond to examples in `examples/draw/` and explain
application setup, scene/UI delegate behavior, and GPU implementation details,
including shader code where applicable. The series adapts selected GPU examples
to draw's managed lifecycle and retained scene/UI composition.

`bg2e::render` remains the production rendering API. The draw and GPU examples
exercise the experimental APIs separately from that production path.

## Tutorials

| # | Tutorial | Description |
|---|----------|-------------|
| 01 | [Window and UI](01_window_ui.md) | Application/MainLoop setup, retained scene clearing, UI controls, and pause/resume |
| 02 | [Simple Triangle](02_simple_triangle.md) | Textured pentagon, compute gradient, animated push constants, and resize-safe resources |
| 03 | [Uniform Buffers](03_uniform_buffers.md) | Rotating textured cube, camera/model UBOs, per-slot resource rings, and an owned depth attachment |
| 04 | [Render to Texture](04_render_to_texture.md) | Offscreen cube rendering, Sobel compute filtering, image copying, and retained output composition |
| 05 | [Cubemap](05_cubemap.md) | HDR environment conversion, six cubemap face passes, and a mirror cube with reflection/debug modes |
| 06 | [Cubemap Render Pass](06_cubemap_render_pass.md) | One-time environment generation, diffuse irradiance, specular mip prefiltering, and map/LOD selection |
| 07 | [Ray Query Shadows](07_ray_query_shadows.md) | Rasterized scene with fragment visibility rays, BLAS/TLAS builds, and per-slot light/acceleration resources |
| 08 | [Deferred Cleanup](08_deferred_cleanup.md) | Cube/sphere replacement, completion-based mesh retirement, release counters, and resize/shutdown draining |
| 09 | [Ray Tracing Pipeline](09_ray_tracing_pipeline.md) | Cornell box path tracing, progressive HDR accumulation, per-slot camera data, and sampled retained output |
| 10 | [Deferred renderer](10_deferred_renderer.md) | Scene resources, query shadows, live settings, public pass injection and reduced resolution/optional FSR |
| 11 | [Scene manipulation](11_manipulation.md) | Build a selectable scene step by step, route camera/tool gestures, edit materials and apply analyzed viewpoints |
| 12 | [Integrated lightmaps](12_lightmaps.md) | Independent UV2, exact completed bake samples, RGB material adoption and preview ownership |
| 13 | [Offscreen scene export](13_offscreen_application.md) | Windowless execution, exact linear camera samples, settings precedence and verified PNG output |

New tutorials will be added to this table as the draw example series grows.
The numbering follows the corresponding directories in `examples/draw/`.

## Application workflow

[Editing, lighting and baking a model](model_edit.md) walks through the delivered
`apps/draw/model_edit` application. It is an application tutorial, not an additional
numbered example target. It covers complete resource saving and focused component
editors in addition to the numbered rendering examples.

## Prerequisites

These tutorials assume a configured bg2e-native build environment with a valid
Vulkan SDK. Use the project's platform-specific CMake generator and resource
packaging conventions. Examples with Metal shader sources also require the
Metal compiler toolchain on macOS.

Start with tutorial 01 to understand the draw application lifecycle. The
[GPU tutorials](../gpu/index.md) provide additional detail on the lower-level
resources and commands used inside draw delegates.

## Key API References

- [Draw API Overview](../../api/draw/index.md)
- [Draw Quick Start](../../api/draw/quick_start.md)
- [Engine](../../api/draw/Engine.md)
- [EngineConfig](../../api/draw/EngineConfig.md)
- [RenderLoop](../../api/draw/RenderLoop.md)
- [RenderLoopDelegate](../../api/draw/RenderLoopDelegate.md)
- [FrameContext](../../api/draw/FrameContext.md)
- [Application and input](../../api/app/Application_and_input.md)
- [MainLoop](../../api/app/MainLoop.md)
- [UserInterface](../../api/ui/UserInterface.md)
- [GPU API Overview](../../api/gpu/index.md)
- [Device](../../api/gpu/Device.md)
- [CommandBuffer](../../api/gpu/CommandBuffer.md)
- [PipelineLayout](../../api/gpu/PipelineLayout.md)
- [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)
- [ComputePipeline](../../api/gpu/ComputePipeline.md)
- [ResourceSet](../../api/gpu/ResourceSet.md)
- [Image](../../api/gpu/Image.md)
- [Mesh](../../api/gpu/Mesh.md)
- [Buffer](../../api/gpu/Buffer.md)
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md)
- [CleanupManager](../../api/gpu/CleanupManager.md)

## Advanced lighting in existing examples

Tutorials [10](10_deferred_renderer.md#advanced-lighting-and-visible-controls) and [11](11_manipulation.md#advanced-lighting-and-visible-controls) now include actual settings-window wiring, GI/reflection activation, capability diagnostics, histories/denoise and final-AA choices. Tutorial 11 also covers per-light local position/orientation controls and active GI/reflection defaults. No extra example target is required.

- [Compose, light and save a scene](composer.md): the delivered draw Composer workflow.


- [Bake model/prefab lightmaps without a camera](standalone_lightmaps.md):
  standalone Engine/Scene lifetime, UV2 preparation, serial batch and typed file output.
