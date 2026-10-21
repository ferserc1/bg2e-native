# Current source anchors and documentation to inspect

[Plan index](index.md) · [Earlier complete inventory](../13-source-evidence.md)

The original table below identifies references at initial planning time; use the current-source supplement below for implementation. New destination paths named in step files are planned outputs and need not exist yet. Re-resolve anchors against code at each milestone review.

| Area | Current source/documentation |
|---|---|
| Render behavior and six-attachment G-buffer | RendererDeferred (`../../../lib/src/bg2e/render/RendererDeferred.cpp`, historical anchor), DeferredLayer (`../../../lib/src/bg2e/render/deferred/DeferredLayer.cpp`, historical anchor), GBufferManager (`../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp`, historical anchor). |
| Drawable and scene component | Drawable (`../../../lib/include/bg2e/scene/Drawable.hpp`, historical anchor), Drawable implementation (`../../../lib/src/bg2e/scene/Drawable.cpp`, historical anchor), DrawableComponent (`../../../lib/include/bg2e/scene/DrawableComponent.hpp`, historical anchor). |
| Scene factories/loading | ComponentFactoryRegistry (`../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp`, historical anchor), db scene (`../../../lib/include/bg2e/db/scene.hpp`, historical anchor), mesh_bg2 (`../../../lib/include/bg2e/db/mesh_bg2.hpp`, historical anchor), glTF (`../../../lib/include/bg2e/db/scene_gltf.hpp`, historical anchor). |
| Existing experimental execution | draw Engine (`../../../lib/src/bg2e/draw/Engine.cpp`, historical anchor), draw RenderLoop (`../../../lib/src/bg2e/draw/RenderLoop.cpp`, historical anchor), MainLoop (`../../../lib/include/bg2e/app/MainLoop.hpp`, historical anchor), DrawGraphicsExecution (`../../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp`, historical anchor). |
| Reflection contracts | Property (`../../../lib/include/bg2e/reflection/Property.hpp`, historical anchor), Builder (`../../../lib/include/bg2e/reflection/Builder.hpp`, historical anchor), TypeInfo (`../../../lib/include/bg2e/reflection/TypeInfo.hpp`, historical anchor), reflection guide (`../../../doc/api/reflection/index.md`, historical anchor). |
| Reflected UI | ReflectionWidget (`../../../lib/include/bg2e/ui/ReflectionWidget.hpp`, historical anchor), RenderSettingsWindow (`../../../lib/include/bg2e/ui/RenderSettingsWindow.hpp`, historical anchor). |
| Primitive GPU state/resources | Common (`../../../lib/include/bg2e/gpu/Common.hpp`, historical anchor), GraphicsPipeline (`../../../lib/include/bg2e/gpu/GraphicsPipeline.hpp`, historical anchor), CommandBuffer (`../../../lib/include/bg2e/gpu/CommandBuffer.hpp`, historical anchor), ResourceSet (`../../../lib/include/bg2e/gpu/ResourceSet.hpp`, historical anchor), Image (`../../../lib/include/bg2e/gpu/Image.hpp`, historical anchor). |
| Device capabilities | PhysicalDevice (`../../../lib/include/bg2e/gpu/PhysicalDevice.hpp`, historical anchor), GPU properties documentation (`../../../doc/api/gpu/PhysicalDeviceProperties.md`, historical anchor). |
| Existing RT design | RT pipeline documentation (`../../../doc/api/gpu/RayTracingPipeline.md`, historical anchor), pipeline tutorial (`../../../doc/tutorials/gpu/13_ray_tracing_pipeline.md`, historical anchor), RT pipeline source (`../../../lib/include/bg2e/gpu/RayTracingPipeline.hpp`, historical anchor), experimental RT example (`../../../examples/draw/09_ray_tracing_pipeline/src/main.cpp`, historical anchor). |
| Query shadows and cubemap mips | query shadow example (`../../../examples/draw/07_ray_query_shadows/src/main.cpp`, historical anchor), cubemap pass example (`../../../examples/draw/06_cubemap_render_pass/src/main.cpp`, historical anchor). |
| Shader loading/build | ShaderLib (`../../../lib/include/bg2e/gpu/ShaderLib.hpp`, historical anchor), actual loader (`../../../lib/src/bg2e/gpu/ShaderLib.cpp`, historical anchor), build helpers (`../../../cmake/utils.cmake`, historical anchor). |
| FSR reference | FSRPostProcessor (`../../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp`, historical anchor), MotionVectorGenerator (`../../../lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp`, historical anchor). |
| Material updates/cache | MaterialModifier (`../../../lib/include/bg2e/utils/MaterialModifier.hpp`, historical anchor), TextureCache (`../../../lib/include/bg2e/utils/TextureCache.hpp`, historical anchor), material uniforms (`../../../lib/include/bg2e/render/uniforms/materials.hpp`, historical anchor). |
| Manipulation | SelectionManager (`../../../lib/include/bg2e/manipulation/SelectionManager.hpp`, historical anchor), GizmoComponent (`../../../lib/include/bg2e/manipulation/GizmoComponent.hpp`, historical anchor), overlay renderer (`../../../lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp`, historical anchor). |
| Viewpoint analysis | Production analyzer (`../../../lib/include/bg2e/render/ViewpointAnalyzer.hpp`, historical anchor), experimental draw analyzer (`../../../lib/include/bg2e/utils/ViewpointAnalyzer.hpp`, historical anchor), camera helpers (`../../../lib/include/bg2e/utils/SceneCameraUtils.hpp`, historical anchor). |
| Baking | IntegratedBakerContext (`../../../lib/include/bg2e/render/IntegratedBakerContext.hpp`, historical anchor), StandaloneBakerContext (`../../../lib/include/bg2e/render/StandaloneBakerContext.hpp`, historical anchor), UvSurfacePass (`../../../lib/src/bg2e/render/UvSurfacePass.cpp`, historical anchor), LightmapOutputWriter (`../../../lib/include/bg2e/db/LightmapOutputWriter.hpp`, historical anchor). |
| Existing executable checks | tests directory (`../../../tests/`, historical anchor), including UV surface, integrated context and UV2 atlas executables; draw examples (`../../../examples/draw/README.md`, historical anchor). No broad unit-test framework is assumed. |
| Headless execution | OffscreenApplication (`../../../lib/include/bg2e/app/OffscreenApplication.hpp`, historical anchor), GPU OffscreenSurface (`../../../lib/include/bg2e/gpu/OffscreenSurface.hpp`, historical anchor). |
| Example and application references | 17_deferred_renderer (`../../../examples/17_deferred_renderer/src/main.cpp`, historical anchor), model_edit (`../../../apps/model_edit/`, historical anchor), bg2e_composer (`../../../apps/bg2e_composer/`, historical anchor), lightmap_generator (`../../../apps/lightmap_generator/`, historical anchor). |

Two implementation facts especially relevant to this plan: reflection exposes getter/setter metadata but does not currently provide settings serialization; and the existing Metal RT wrapper executes a raygen compute kernel and ignores separate miss/closest-hit modules. Neither should be mistaken for a missing high-level shader abstraction to invent.

## Current optimized implementation anchors — 2026-10-21

Read [engine/plan discrepancies](code-reconciliation-2026-10-21.md). Production
entries above remain behavior/retirement references, not the only current sources.

| Area | Current source |
|---|---|
| Renderer/material revision caches | [RendererDeferred](../../../lib/src/bg2e/draw/RendererDeferred.cpp), [ShaderData](../../../lib/src/bg2e/draw/ShaderData.cpp), [DataBinding](../../../lib/src/bg2e/draw/DataBinding.cpp), [MaterialBase](../../../lib/src/bg2e/draw/MaterialBase.cpp), [glTF alpha import](../../../lib/src/bg2e/db/scene_gltf.cpp) |
| Indexed and persistent tracing/shadow tables, acceleration | [TracingSceneResources](../../../lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp), [persistent owners](../../../lib/src/bg2e/draw/pipeline/TracingPersistentResources.hpp), [native array bindings](../../../lib/src/bg2e/draw/pipeline/TracingPersistentResources.cpp), [lookup keys](../../../lib/src/bg2e/draw/pipeline/TracingTableLookup.hpp), [SoftShadowPass](../../../lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp) |
| Completion, pool and regions | [CommandBuffer](../../../lib/include/bg2e/gpu/CommandBuffer.hpp), [TemporaryResourcePool](../../../lib/include/bg2e/draw/pipeline/TemporaryResourcePool.hpp), [ProcessingRegion](../../../lib/include/bg2e/draw/pipeline/ProcessingRegion.hpp), [uploaded bounds](../../../lib/include/bg2e/gpu/Mesh.hpp) |
| Temporal signatures and update synchronization | [TemporalSignatureCache](../../../lib/src/bg2e/draw/TemporalSignatureCache.cpp), [typed delegate updates](../../../lib/src/bg2e/draw/DefaultRenderLoopDelegate.cpp), [settings path](../../../lib/src/bg2e/draw/RenderSettingsPreferences.cpp) |
| Implemented integrated bake primitives | [IntegratedBakerContext](../../../lib/include/bg2e/bake/IntegratedBakerContext.hpp), [LightmapBakeExecutor](../../../lib/src/bg2e/bake/LightmapBakeExecutor.cpp), [UvTemporalAccumulator](../../../lib/include/bg2e/bake/UvTemporalAccumulator.hpp), [LightmapCompositionPass](../../../lib/include/bg2e/bake/LightmapCompositionPass.hpp) |
| Complete IO and save result | [DB draw export](../../../lib/src/bg2e/db/draw_resource_io.cpp), [scene API](../../../lib/include/bg2e/db/scene.hpp), [Composer document publication](../../../apps/draw/bg2e_composer/src/StageScene.cpp) |
| CPU/GPU telemetry and UI | [CpuProfile](../../../lib/include/bg2e/base/CpuProfile.hpp), [GpuProfile](../../../lib/include/bg2e/base/GpuProfile.hpp), [SubmissionTiming](../../../lib/include/bg2e/gpu/SubmissionTiming.hpp), [Performance](../../../lib/src/bg2e/ui/Performance.cpp), [draw RenderLoop](../../../lib/src/bg2e/draw/RenderLoop.cpp), [maintained profiling reference](../../../doc/architecture/cpu_profiling.md) |
| Independent source/evidence inventories | [optimization phases](../../optimizations/render_to_draw/summary.md), [profiling phases](../../profiling_and_stats/summary.md), [optimization probes](../../../tests/draw/), [profiling probes](../../profiling_and_stats/) |

The settings codec/facade now exists in draw; the original note about reflection
not serializing settings is not evidence that this implemented codec is missing.
The public low-level Metal RT model and current v3/v5 native contracts must be
checked directly rather than reconstructed from initial pipeline examples.

## Delivered M08 source anchors

Use [M08 review](milestone_08_lightmap_generator/review.md) and actual bake headers:
[StandaloneBakerContext](../../../lib/include/bg2e/bake/StandaloneBakerContext.hpp),
[StandaloneLightmapBaker](../../../lib/include/bg2e/bake/StandaloneLightmapBaker.hpp),
[StandaloneBakeSceneAssembler](../../../lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp),
[StandaloneBakeBatch](../../../lib/include/bg2e/bake/StandaloneBakeBatch.hpp) and
[LightmapOutputWriter](../../../lib/include/bg2e/bake/LightmapOutputWriter.hpp).
The [draw CLI](../../../apps/draw/lightmap_generator/src/main.cpp) consumes these
actual APIs; generic [file publication](../../../lib/include/bg2e/db/file.hpp),
[native archive](../../../lib/include/bg2e/db/texture_archive.hpp) and OBJ/MTL
inventory remain DB. Production baking anchors above remain retirement references,
not evidence that standalone code is absent. [Candidate inventory](milestone_08_lightmap_generator/removal-impact.md)
records shared transitive dependencies and current public include limitations.

## Accepted production anchors — M09-07

The initial table is historical. Current production anchors are
[Renderer](../../../lib/include/bg2e/draw/Renderer.hpp),
[settings catalog](../../../lib/include/bg2e/draw/SettingsCatalog.hpp),
[upscaler](../../../lib/include/bg2e/draw/Upscaler.hpp),
[bake context](../../../lib/include/bg2e/bake/BakerContext.hpp),
[drawable](../../../lib/include/bg2e/draw/Drawable.hpp), and
[final product/API evidence](milestone_09_cleanup/review.md).
Reflection settings serialization is now implemented by SettingsCodec; the initial
absence statement describes the planning snapshot only.
