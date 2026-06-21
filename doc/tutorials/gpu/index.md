# GPU Abstraction Layer Tutorials

This directory contains step-by-step tutorials for the `bg2e::gpu` namespace,
the backend-agnostic GPU abstraction layer in the bg2e engine.

Each tutorial corresponds to an example in `examples/gpu/` and explains the
code incrementally, including shader code where applicable.

## Tutorials

| # | Tutorial | Description |
|---|----------|-------------|
| 01 | [Instance](01_instance.md) | Initialize the GPU factory and shared instance |
| 02 | [Device](02_device.md) | Create a window, select a physical device, and query properties |
| 03 | [Offscreen Device](03_offscreen_device.md) | Headless GPU initialization without a window |
| 04 | [Clear Loop](04_clear_loop.md) | Animated clear-color render loop |
| 05 | [Simple Triangle](05_simple_triangle.md) | Textured pentagon with compute-generated gradient background |
| 06 | [Offscreen Triangle](06_offscreen_triangle.md) | Render a triangle offscreen and save to disk |
| 07 | [Uniform Buffers](07_uniform_buffers.md) | Rotating cube with camera/model UBOs and FrameResourceRing |
| 08 | [Render to Texture](08_render_to_texture.md) | Offscreen render + compute Sobel edge detection + copy to swapchain |
| 09 | [Cubemap](09_cubemap.md) | Equirect-to-cubemap projection with mirror reflection cube |
| 10 | [Cubemap Render Pass](10_cubemap_render_pass.md) | IBL pipeline: environment, irradiance, and specular cubemaps with CubemapRenderPass |
| 11 | [Ray Query Shadows](11_ray_query_shadows.md) | Hardware ray queries for hard shadows in a Lambert-lit scene |
| 12 | [Deferred Cleanup](12_deferred_cleanup.md) | Deferred resource destruction with CleanupManager::defer() |
| 13 | [Ray Tracing Pipeline](13_ray_tracing_pipeline.md) | Cornell box path tracer using RayTracingPipeline |

## Prerequisites

These tutorials assume you have already set up your build environment and can
compile the engine. They focus exclusively on the GPU abstraction layer code
and shaders.

## Key API References

- [GPU Abstraction Layer Overview](../../api/gpu/index.md)
- [Quick Start Guide](../../api/gpu/quick_start.md)
- [Backend](../../api/gpu/Backend.md)
- [Device](../../api/gpu/Device.md)
- [CommandBuffer](../../api/gpu/CommandBuffer.md)
- [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)
- [ComputePipeline](../../api/gpu/ComputePipeline.md)
- [RayTracingPipeline](../../api/gpu/RayTracingPipeline.md)
- [ResourceSet](../../api/gpu/ResourceSet.md)
- [Buffer](../../api/gpu/Buffer.md)
- [Image](../../api/gpu/Image.md)
- [Mesh](../../api/gpu/Mesh.md)
- [CleanupManager](../../api/gpu/CleanupManager.md)
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md)
