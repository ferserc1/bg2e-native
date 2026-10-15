# M01 — Initial deferred renderer

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Existing draw/GPU/MainLoop foundations; read all root decision documents before changing contracts.

## Working product

A runnable `examples/draw/10_deferred_renderer` scene on the new non-template delegate and RendererDeferred. It retains the supported scene/material/IBL/transparency behavior of example 17, adds alpha-aware query soft shadows when available, defaults to adjustable internal-resolution reduction with fullscreen resampling, and offers optional FSR plus reflected/importable settings.

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
| [M01-15](step_15_working-example.md) | Build and adapt examples/draw/10_deferred_renderer |
| [M01-16](step_16_review.md) | Review M01 code and revise all remaining milestones |
| [M01-17](step_17_documentation.md) | Update maintained documentation cumulatively through M01 |

## Milestone exit gate

- The final [documentation step](step_17_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Example target and paired shader libraries are registered and packaged; it runs through Application/MainLoop rather than a private execution loop.
- Opaque/transparent/submesh material textures, environment generation, floor/camera/lights and query soft shadows behave correctly; no-RT hardware still renders.
- Default lower internal resolution stays available without FSR. FSR failures expose status and preserve the ordinary resampling path.
- Reflection settings read/write the live objects, serialize/export/import without UI, and rebind when the renderer or upscaler changes.
- Replacing the renderer with a new RendererDeferred instance at a safe boundary works; public layer/pass replacement and independent GPU clone behavior are demonstrated.
- Production examples remain intact; M01 review records actual interfaces and revises M02–M10.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01-12 implementation handoff

[reflected-settings.md](reflected-settings.md) records the delivered live catalog/codec/facade, extension surface, ownership/generations, pre-acquire queue, typed JSON and optional persistence. CPU-only manual fixture: tests/draw/settings_codec. Compilation/runtime validation remains with the project lead; M01-13 is the next required step after acceptance.

## M01-13 implementation handoff

[upscaling.md](upscaling.md) records open Upscaler injection/live reflection, ResolutionSettings, reconstructed HDR/final contract, FSR Vulkan bridge/camera motion/reset/state/availability and fallback limits. Manual lead fixture: tests/draw/upscaling. M01-13 awaits lead validation; M01-14 is the linked successor.

## M01-14 implementation handoff

[settings-ui.md](settings-ui.md) records the new live GraphicsSettingsWindow, transient reflection editors, queued file/restore actions, optional persistence and minimal resource-picker scope. Lead checks are in tests/draw/settings_ui; M01-15 is next after acceptance.

## M01-15 implementation handoff

[working-example.md](working-example.md) records target draw_deferred_renderer, actual native-free scene/CLI/fixtures/settings/replacement integration and lead checks. M01-14 was accepted after the lead's virtual Window destructor correction, which is preserved. M01-15 awaits validation; M01-16 is the required separate milestone review.

## M01-16 review delivery

[Review report](review.md) reconciles source contracts and all pending M02–M10 work. M01-15 accepted by project lead continuation; backend/scenario coverage unspecified. The review is delivered for lead validation and has not yet accepted M01 as a whole. M01-17 cumulative documentation is the next linked step after technical review acceptance; M02-01 follows documentation closure.

## Documentation closure order

The technical review now hands off to [M01-17](step_17_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).
