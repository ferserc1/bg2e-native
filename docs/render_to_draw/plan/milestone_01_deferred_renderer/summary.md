# M01 — Initial deferred renderer

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Existing draw/GPU/MainLoop foundations; read all root decision documents before changing contracts.

## Working product

A runnable `examples/draw/deferred_renderer` scene on the new non-template delegate and RendererDeferred. It retains the supported scene/material/IBL/transparency behavior of example 17, adds alpha-aware query soft shadows when available, defaults to adjustable internal-resolution reduction with fullscreen resampling, and offers optional FSR plus reflected/importable settings.

## Scope exclusions

- No bake classes/map generation or UV2 workflow. Reserve neutral baked-map channels only where necessary to avoid changing the G-buffer later.
- No OffscreenApplication migration, manipulation tools or editor/application copies.
- No engine TemporalAccumulator, DenoiseFilter, advanced RTAO/RTGI/reflection pipelines, MSAA or forward renderer.
- FSR SDK temporal state and its minimal motion/jitter inputs are allowed; they are not a general engine history implementation.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M01-01](step_01_public-contracts.md) | Public object families and configuration contracts |
| [M01-02](step_02_gpu-rendering-primitives.md) | GPU MRT, pipeline state, formats and capabilities |
| [M01-03](step_03_gpu-subresources.md) | Image views, uploads and mipmap operations |
| [M01-04](step_04_shader-libraries.md) | Paired pipeline shader folders and deployment |
| [M01-05](step_05_runtime-renderer.md) | Non-template delegate and runtime renderer lifecycle |
| [M01-06](step_06_drawable-resources.md) | Draw Drawable, materials/textures and dual scene component |
| [M01-07](step_07_db-levels.md) | Graphics-free serialization and simple engine-aware load paths |
| [M01-08](step_08_environment.md) | Public environment conversion, IBL and sky tools |
| [M01-09](step_09_bindings-visitors.md) | Draw data bindings, visitors and queues |
| [M01-10](step_10_raster-layers.md) | Basic deferred renderer and injectable public layers |
| [M01-11](step_11_query-soft-shadows.md) | Alpha-aware query soft shadows without RT pipelines |
| [M01-12](step_12_reflected-settings.md) | Stateless preferences facade, dynamic catalog and settings codec |
| [M01-13](step_13_upscaling-fsr.md) | Open upscaler API, resolution reduction and FSR integration |
| [M01-14](step_14_settings-ui.md) | Dynamic graphics settings widget and minimal texture UI |
| [M01-15](step_15_working-example.md) | Build and adapt examples/draw/deferred_renderer |
| [M01-16](step_16_review.md) | Review M01 code and revise all remaining milestones |

## Milestone exit gate

- Example target and paired shader libraries are registered and packaged; it runs through Application/MainLoop rather than a private execution loop.
- Opaque/transparent/submesh material textures, environment generation, floor/camera/lights and query soft shadows behave correctly; no-RT hardware still renders.
- Default lower internal resolution stays available without FSR. FSR failures expose status and preserve the ordinary resampling path.
- Reflection settings read/write the live objects, serialize/export/import without UI, and rebind when the renderer or upscaler changes.
- Replacing the renderer with a new RendererDeferred instance at a safe boundary works; public layer/pass replacement and independent GPU clone behavior are demonstrated.
- Production examples remain intact; M01 review records actual interfaces and revises M02–M10.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
