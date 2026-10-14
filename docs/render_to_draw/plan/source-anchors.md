# Current source anchors and documentation to inspect

[Plan index](index.md) · [Earlier complete inventory](../13-source-evidence.md)

These links identify current references at planning time. New destination paths named in step files are planned outputs and need not exist yet. Re-resolve anchors against code at each milestone review.

| Area | Current source/documentation |
|---|---|
| Render behavior and six-attachment G-buffer | [RendererDeferred](../../../lib/src/bg2e/render/RendererDeferred.cpp), [DeferredLayer](../../../lib/src/bg2e/render/deferred/DeferredLayer.cpp), [GBufferManager](../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp). |
| Drawable and scene component | [Drawable](../../../lib/include/bg2e/scene/Drawable.hpp), [Drawable implementation](../../../lib/src/bg2e/scene/Drawable.cpp), [DrawableComponent](../../../lib/include/bg2e/scene/DrawableComponent.hpp). |
| Scene factories/loading | [ComponentFactoryRegistry](../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp), [db scene](../../../lib/include/bg2e/db/scene.hpp), [mesh_bg2](../../../lib/include/bg2e/db/mesh_bg2.hpp), [glTF](../../../lib/include/bg2e/db/scene_gltf.hpp). |
| Existing experimental execution | [draw Engine](../../../lib/src/bg2e/draw/Engine.cpp), [draw RenderLoop](../../../lib/src/bg2e/draw/RenderLoop.cpp), [MainLoop](../../../lib/include/bg2e/app/MainLoop.hpp), [DrawGraphicsExecution](../../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp). |
| Reflection contracts | [Property](../../../lib/include/bg2e/reflection/Property.hpp), [Builder](../../../lib/include/bg2e/reflection/Builder.hpp), [TypeInfo](../../../lib/include/bg2e/reflection/TypeInfo.hpp), [reflection guide](../../../doc/api/reflection/index.md). |
| Reflected UI | [ReflectionWidget](../../../lib/include/bg2e/ui/ReflectionWidget.hpp), [RenderSettingsWindow](../../../lib/include/bg2e/ui/RenderSettingsWindow.hpp). |
| Primitive GPU state/resources | [Common](../../../lib/include/bg2e/gpu/Common.hpp), [GraphicsPipeline](../../../lib/include/bg2e/gpu/GraphicsPipeline.hpp), [CommandBuffer](../../../lib/include/bg2e/gpu/CommandBuffer.hpp), [ResourceSet](../../../lib/include/bg2e/gpu/ResourceSet.hpp), [Image](../../../lib/include/bg2e/gpu/Image.hpp). |
| Device capabilities | [PhysicalDevice](../../../lib/include/bg2e/gpu/PhysicalDevice.hpp), [GPU properties documentation](../../../doc/api/gpu/PhysicalDeviceProperties.md). |
| Existing RT design | [RT pipeline documentation](../../../doc/api/gpu/RayTracingPipeline.md), [pipeline tutorial](../../../doc/tutorials/gpu/13_ray_tracing_pipeline.md), [RT pipeline source](../../../lib/include/bg2e/gpu/RayTracingPipeline.hpp), [experimental RT example](../../../examples/draw/09_ray_tracing_pipeline/src/main.cpp). |
| Query shadows and cubemap mips | [query shadow example](../../../examples/draw/07_ray_query_shadows/src/main.cpp), [cubemap pass example](../../../examples/draw/06_cubemap_render_pass/src/main.cpp). |
| Shader loading/build | [ShaderLib](../../../lib/include/bg2e/gpu/ShaderLib.hpp), [actual loader](../../../lib/src/bg2e/gpu/ShaderLib.cpp), [build helpers](../../../cmake/utils.cmake). |
| FSR reference | [FSRPostProcessor](../../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp), [MotionVectorGenerator](../../../lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp). |
| Material updates/cache | [MaterialModifier](../../../lib/include/bg2e/utils/MaterialModifier.hpp), [TextureCache](../../../lib/include/bg2e/utils/TextureCache.hpp), [material uniforms](../../../lib/include/bg2e/render/uniforms/materials.hpp). |
| Manipulation | [SelectionManager](../../../lib/include/bg2e/manipulation/SelectionManager.hpp), [GizmoComponent](../../../lib/include/bg2e/manipulation/GizmoComponent.hpp), [overlay renderer](../../../lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp). |
| Baking | [IntegratedBakerContext](../../../lib/include/bg2e/render/IntegratedBakerContext.hpp), [StandaloneBakerContext](../../../lib/include/bg2e/render/StandaloneBakerContext.hpp), [UvSurfacePass](../../../lib/src/bg2e/render/UvSurfacePass.cpp), [LightmapOutputWriter](../../../lib/include/bg2e/db/LightmapOutputWriter.hpp). |
| Existing executable checks | [tests directory](../../../tests/), including UV surface, integrated context and UV2 atlas executables; [draw examples](../../../examples/draw/README.md). No broad unit-test framework is assumed. |
| Headless execution | [OffscreenApplication](../../../lib/include/bg2e/app/OffscreenApplication.hpp), [GPU OffscreenSurface](../../../lib/include/bg2e/gpu/OffscreenSurface.hpp). |
| Example and application references | [17_deferred_renderer](../../../examples/17_deferred_renderer/src/main.cpp), [model_edit](../../../apps/model_edit/), [bg2e_composer](../../../apps/bg2e_composer/), [lightmap_generator](../../../apps/lightmap_generator/). |

Two implementation facts especially relevant to this plan: reflection exposes getter/setter metadata but does not currently provide settings serialization; and the existing Metal RT wrapper executes a raygen compute kernel and ignores separate miss/closest-hit modules. Neither should be mistaken for a missing high-level shader abstraction to invent.
