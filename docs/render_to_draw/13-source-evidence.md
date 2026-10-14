# Source inventory and dependency evidence

[Overview](index.md) · [Production inventory](01-production-inventory.md) · [Dependency map](02-dependency-map.md)

## 1. Snapshot and method

Source snapshot: 2026-10-13, commit `12dac9eba55411267d0025b6bcaa31f4a99de3e7`. The initial working tree was clean. This appendix was generated from local source files; no build or runtime analysis was used.

Inventory scope: `.hpp`, `.h`, `.cpp`, and `.c` below the listed engine directories. Counts include license/comments/blank lines and private implementation headers. Shader files are listed separately. Vendored dependencies and build output are excluded.

Dependency method: literal direct `#include` directives for `bg2e/...` and textual qualified `render::` references. References may occur in comments or forward declarations. Umbrella/transitive includes, aliases, unqualified names under namespace imports, template instantiation and runtime calls are not resolved. Conditional includes are counted regardless of active platform. These results are evidence for review, not a semantic compiler dependency graph.

Relative source links refer to current repository files. Reported line numbers refer to this snapshot and may change after edits.

## 2. Scoped size measurements

| Tree | Files | Physical lines |
|---|---:|---:|
| `lib/include/bg2e/render` | 93 | 9,196 |
| `lib/src/bg2e/render` | 85 | 30,302 |
| `lib/include/bg2e/draw` | 6 | 371 |
| `lib/src/bg2e/draw` | 2 | 643 |
| `lib/include/bg2e/gpu` | 81 | 7,288 |
| `lib/src/bg2e/gpu` | 57 | 12,554 |
| `lib/include/bg2e/scene` | 37 | 2,917 |
| `lib/src/bg2e/scene` | 42 | 4,772 |
| `lib/include/bg2e/manipulation` | 6 | 918 |
| `lib/src/bg2e/manipulation` | 5 | 2,154 |

These counts do not imply that every production file must be copied. Primitive wrappers are replaced, the forward renderer is excluded, and some public helpers become adapters or private services.

## 3. Complete production header inventory

The destination column states a proposed responsibility, not a literal file move. Existing production implementations may remain during coexistence. Umbrella headers do not need independent replacement classes.

### Top-level render

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/BRDFIntegrationMapTextureGenerator.hpp](../../lib/include/bg2e/render/BRDFIntegrationMapTextureGenerator.hpp) | 37 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/BakerContext.hpp](../../lib/include/bg2e/render/BakerContext.hpp) | 58 | Draw baking services; preserve state/lifetime semantics. |
| [lib/include/bg2e/render/BlueNoise.hpp](../../lib/include/bg2e/render/BlueNoise.hpp) | 76 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/ColorAttachments.hpp](../../lib/include/bg2e/render/ColorAttachments.hpp) | 71 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/ColorAttachmentsCanvas.hpp](../../lib/include/bg2e/render/ColorAttachmentsCanvas.hpp) | 63 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/CubemapRenderer.hpp](../../lib/include/bg2e/render/CubemapRenderer.hpp) | 107 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp](../../lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp) | 80 | Experimental draw lifecycle/default adapters; use existing execution foundations. |
| [lib/include/bg2e/render/DefaultRenderLoopDelegate.hpp](../../lib/include/bg2e/render/DefaultRenderLoopDelegate.hpp) | 71 | Experimental draw lifecycle/default adapters; use existing execution foundations. |
| [lib/include/bg2e/render/Engine.hpp](../../lib/include/bg2e/render/Engine.hpp) | 213 | Experimental draw lifecycle/default adapters; use existing execution foundations. |
| [lib/include/bg2e/render/EnvironmentResources.hpp](../../lib/include/bg2e/render/EnvironmentResources.hpp) | 141 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/GPUProcess.hpp](../../lib/include/bg2e/render/GPUProcess.hpp) | 81 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/GPUTextureGenerator.hpp](../../lib/include/bg2e/render/GPUTextureGenerator.hpp) | 56 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/IntegratedBakerContext.hpp](../../lib/include/bg2e/render/IntegratedBakerContext.hpp) | 121 | Draw baking services; preserve state/lifetime semantics. |
| [lib/include/bg2e/render/IrradianceCubemapRenderer.hpp](../../lib/include/bg2e/render/IrradianceCubemapRenderer.hpp) | 38 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/LightmapBaker.hpp](../../lib/include/bg2e/render/LightmapBaker.hpp) | 100 | Draw baking services; preserve state/lifetime semantics. |
| [lib/include/bg2e/render/LightmapSettings.hpp](../../lib/include/bg2e/render/LightmapSettings.hpp) | 64 | Backend-neutral bake configuration/data, exposed through suitable draw/neutral headers. |
| [lib/include/bg2e/render/MaterialBase.hpp](../../lib/include/bg2e/render/MaterialBase.hpp) | 103 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/RenderLoop.hpp](../../lib/include/bg2e/render/RenderLoop.hpp) | 100 | Experimental draw lifecycle/default adapters; use existing execution foundations. |
| [lib/include/bg2e/render/RenderLoopDelegate.hpp](../../lib/include/bg2e/render/RenderLoopDelegate.hpp) | 99 | Experimental draw lifecycle/default adapters; use existing execution foundations. |
| [lib/include/bg2e/render/RenderQueue.hpp](../../lib/include/bg2e/render/RenderQueue.hpp) | 85 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/RenderSettingsPreferences.hpp](../../lib/include/bg2e/render/RenderSettingsPreferences.hpp) | 139 | Application/DB/persistence adapter; isolate from core renderer. |
| [lib/include/bg2e/render/Renderer.hpp](../../lib/include/bg2e/render/Renderer.hpp) | 165 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/RendererBasicForward.hpp](../../lib/include/bg2e/render/RendererBasicForward.hpp) | 206 | Production only; excluded from draw. |
| [lib/include/bg2e/render/RendererDeferred.hpp](../../lib/include/bg2e/render/RendererDeferred.hpp) | 335 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/SkyboxRenderer.hpp](../../lib/include/bg2e/render/SkyboxRenderer.hpp) | 115 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/SpecularReflectionCubemapRenderer.hpp](../../lib/include/bg2e/render/SpecularReflectionCubemapRenderer.hpp) | 41 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/SphereToCubemapRenderer.hpp](../../lib/include/bg2e/render/SphereToCubemapRenderer.hpp) | 113 | Draw environment resource/generation/sky services over GPU cubemap primitives. |
| [lib/include/bg2e/render/StandaloneBakeBatch.hpp](../../lib/include/bg2e/render/StandaloneBakeBatch.hpp) | 79 | Application/DB/persistence adapter; isolate from core renderer. |
| [lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp](../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp) | 85 | Application/DB/persistence adapter; isolate from core renderer. |
| [lib/include/bg2e/render/StandaloneBakerContext.hpp](../../lib/include/bg2e/render/StandaloneBakerContext.hpp) | 118 | Draw baking services; preserve state/lifetime semantics. |
| [lib/include/bg2e/render/Texture.hpp](../../lib/include/bg2e/render/Texture.hpp) | 170 | Draw high-level graphics resource/service, redesigned over GPU. |
| [lib/include/bg2e/render/UvMapPreviewRenderer.hpp](../../lib/include/bg2e/render/UvMapPreviewRenderer.hpp) | 91 | Draw graphics tool; separate UI/scoring policy. |
| [lib/include/bg2e/render/ViewpointAnalyzer.hpp](../../lib/include/bg2e/render/ViewpointAnalyzer.hpp) | 177 | Draw graphics tool; separate UI/scoring policy. |
| [lib/include/bg2e/render/all.hpp](../../lib/include/bg2e/render/all.hpp) | 53 | Umbrella/legacy include surface; rebuild only around approved public APIs. |

### deferred

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/deferred/DeferredLayer.hpp](../../lib/include/bg2e/render/deferred/DeferredLayer.hpp) | 393 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/DenoiseFilter.hpp](../../lib/include/bg2e/render/deferred/DenoiseFilter.hpp) | 109 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/DirectPostProcessor.hpp](../../lib/include/bg2e/render/deferred/DirectPostProcessor.hpp) | 56 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/FSRPostProcessor.hpp](../../lib/include/bg2e/render/deferred/FSRPostProcessor.hpp) | 139 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/FinalPostProcessor.hpp](../../lib/include/bg2e/render/deferred/FinalPostProcessor.hpp) | 106 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/MotionVectorGenerator.hpp](../../lib/include/bg2e/render/deferred/MotionVectorGenerator.hpp) | 91 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp](../../lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp) | 192 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp](../../lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp) | 246 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/RTReflections.hpp](../../lib/include/bg2e/render/deferred/RTReflections.hpp) | 224 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/RenderLayer.hpp](../../lib/include/bg2e/render/deferred/RenderLayer.hpp) | 92 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/SMAAPostProcessor.hpp](../../lib/include/bg2e/render/deferred/SMAAPostProcessor.hpp) | 87 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/SMAAProcessor.hpp](../../lib/include/bg2e/render/deferred/SMAAProcessor.hpp) | 119 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/SkyboxLayer.hpp](../../lib/include/bg2e/render/deferred/SkyboxLayer.hpp) | 57 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |
| [lib/include/bg2e/render/deferred/TemporalAccumulator.hpp](../../lib/include/bg2e/render/deferred/TemporalAccumulator.hpp) | 160 | Draw deferred/effect/postprocess implementation; split policy, bindings and histories. |

### gbuffer

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/gbuffer/GBufferManager.hpp](../../lib/include/bg2e/render/gbuffer/GBufferManager.hpp) | 79 | Draw semantic G-buffer/resource owner; GPU supplies primitive MRT. |

### uniforms

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/uniforms/all.hpp](../../lib/include/bg2e/render/uniforms/all.hpp) | 21 | Umbrella/legacy include surface; rebuild only around approved public APIs. |
| [lib/include/bg2e/render/uniforms/materials.hpp](../../lib/include/bg2e/render/uniforms/materials.hpp) | 122 | Draw-private shader packing or neutral reusable values; preserve shader ABI. |

### vulkan

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/vulkan/Buffer.hpp](../../lib/include/bg2e/render/vulkan/Buffer.hpp) | 91 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/CleanupManager.hpp](../../lib/include/bg2e/render/vulkan/CleanupManager.hpp) | 67 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/Command.hpp](../../lib/include/bg2e/render/vulkan/Command.hpp) | 69 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/DescriptorSet.hpp](../../lib/include/bg2e/render/vulkan/DescriptorSet.hpp) | 179 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/DescriptorSetAllocator.hpp](../../lib/include/bg2e/render/vulkan/DescriptorSetAllocator.hpp) | 79 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/Device.hpp](../../lib/include/bg2e/render/vulkan/Device.hpp) | 80 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/FrameResources.hpp](../../lib/include/bg2e/render/vulkan/FrameResources.hpp) | 71 | Split GPU frame execution from draw-owned resources; no wholesale copy. |
| [lib/include/bg2e/render/vulkan/Image.hpp](../../lib/include/bg2e/render/vulkan/Image.hpp) | 224 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/Info.hpp](../../lib/include/bg2e/render/vulkan/Info.hpp) | 104 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/PhysicalDevice.hpp](../../lib/include/bg2e/render/vulkan/PhysicalDevice.hpp) | 161 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/PipelineDataBinding.hpp](../../lib/include/bg2e/render/vulkan/PipelineDataBinding.hpp) | 60 | GPU resource-set mechanisms plus draw-specific binding implementations. |
| [lib/include/bg2e/render/vulkan/Surface.hpp](../../lib/include/bg2e/render/vulkan/Surface.hpp) | 54 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/Swapchain.hpp](../../lib/include/bg2e/render/vulkan/Swapchain.hpp) | 137 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/all.hpp](../../lib/include/bg2e/render/vulkan/all.hpp) | 37 | Umbrella/legacy include surface; rebuild only around approved public APIs. |
| [lib/include/bg2e/render/vulkan/common.hpp](../../lib/include/bg2e/render/vulkan/common.hpp) | 83 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/extensions.hpp](../../lib/include/bg2e/render/vulkan/extensions.hpp) | 86 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |

### vulkan/factory

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/vulkan/factory/ComputePipeline.hpp](../../lib/include/bg2e/render/vulkan/factory/ComputePipeline.hpp) | 51 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/DescriptorSetLayout.hpp](../../lib/include/bg2e/render/vulkan/factory/DescriptorSetLayout.hpp) | 52 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp](../../lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp) | 113 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/PipelineLayout.hpp](../../lib/include/bg2e/render/vulkan/factory/PipelineLayout.hpp) | 62 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp](../../lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp) | 102 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/Sampler.hpp](../../lib/include/bg2e/render/vulkan/factory/Sampler.hpp) | 50 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/ShaderModule.hpp](../../lib/include/bg2e/render/vulkan/factory/ShaderModule.hpp) | 44 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/factory/all.hpp](../../lib/include/bg2e/render/vulkan/factory/all.hpp) | 27 | Umbrella/legacy include surface; rebuild only around approved public APIs. |

### vulkan/geo

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/vulkan/geo/Mesh.hpp](../../lib/include/bg2e/render/vulkan/geo/Mesh.hpp) | 119 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/geo/VertexDescription.hpp](../../lib/include/bg2e/render/vulkan/geo/VertexDescription.hpp) | 70 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/geo/all.hpp](../../lib/include/bg2e/render/vulkan/geo/all.hpp) | 22 | Umbrella/legacy include surface; rebuild only around approved public APIs. |

### vulkan/macros

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/vulkan/macros/all.hpp](../../lib/include/bg2e/render/vulkan/macros/all.hpp) | 23 | Umbrella/legacy include surface; rebuild only around approved public APIs. |
| [lib/include/bg2e/render/vulkan/macros/descriptor_set.hpp](../../lib/include/bg2e/render/vulkan/macros/descriptor_set.hpp) | 74 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/macros/frame_resources.hpp](../../lib/include/bg2e/render/vulkan/macros/frame_resources.hpp) | 106 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/macros/graphics.hpp](../../lib/include/bg2e/render/vulkan/macros/graphics.hpp) | 60 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |

### vulkan/rt

| Header | Lines | Proposed responsibility/destination |
|---|---:|---|
| [lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp](../../lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp) | 66 | Draw scene/material/light extraction and bindings; keep primitive native handling in GPU. |
| [lib/include/bg2e/render/vulkan/rt/RTAccelerationStructureSize.hpp](../../lib/include/bg2e/render/vulkan/rt/RTAccelerationStructureSize.hpp) | 64 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/rt/RTMaterialData.h](../../lib/include/bg2e/render/vulkan/rt/RTMaterialData.h) | 68 | Draw scene/material/light extraction and bindings; keep primitive native handling in GPU. |
| [lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp](../../lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp) | 65 | Draw scene/material/light extraction and bindings; keep primitive native handling in GPU. |
| [lib/include/bg2e/render/vulkan/rt/RayTracingMesh.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingMesh.hpp) | 83 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp) | 76 | Split: GPU TLAS primitive, draw traversal/material tables and frame ownership. |
| [lib/include/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp) | 50 | Draw scene/material/light extraction and bindings; keep primitive native handling in GPU. |
| [lib/include/bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp) | 100 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |
| [lib/include/bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp](../../lib/include/bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp) | 69 | Draw scene/material/light extraction and bindings; keep primitive native handling in GPU. |
| [lib/include/bg2e/render/vulkan/rt/all.hpp](../../lib/include/bg2e/render/vulkan/rt/all.hpp) | 26 | Umbrella/legacy include surface; rebuild only around approved public APIs. |
| [lib/include/bg2e/render/vulkan/rt/utils.hpp](../../lib/include/bg2e/render/vulkan/rt/utils.hpp) | 28 | Primitive counterpart in GPU or legacy/private backend helper; no draw Vulkan wrapper hierarchy. |

## 4. Complete production implementation/private-file inventory

All scoped implementation/private files are listed, including baked-light passes and embedded noise data. An outgoing dependency lists literal non-render `bg2e` module includes, not inherited/transitive dependencies.

### Top-level render

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/BRDFIntegrationMapTextureGenerator.cpp](../../lib/src/bg2e/render/BRDFIntegrationMapTextureGenerator.cpp) | 53 | `base`, `math` |
| [lib/src/bg2e/render/BakerContext.cpp](../../lib/src/bg2e/render/BakerContext.cpp) | 29 | `scene` |
| [lib/src/bg2e/render/BlueNoise.cpp](../../lib/src/bg2e/render/BlueNoise.cpp) | 179 | — |
| [lib/src/bg2e/render/ColorAttachments.cpp](../../lib/src/bg2e/render/ColorAttachments.cpp) | 66 | — |
| [lib/src/bg2e/render/ColorAttachmentsCanvas.cpp](../../lib/src/bg2e/render/ColorAttachmentsCanvas.cpp) | 128 | — |
| [lib/src/bg2e/render/CubemapRenderer.cpp](../../lib/src/bg2e/render/CubemapRenderer.cpp) | 329 | `geo` |
| [lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp](../../lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp) | 134 | `app`, `utils` |
| [lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp](../../lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp) | 117 | `utils` |
| [lib/src/bg2e/render/Engine.cpp](../../lib/src/bg2e/render/Engine.cpp) | 306 | `base`, `gpu` |
| [lib/src/bg2e/render/EnvironmentResources.cpp](../../lib/src/bg2e/render/EnvironmentResources.cpp) | 275 | — |
| [lib/src/bg2e/render/GPUProcess.cpp](../../lib/src/bg2e/render/GPUProcess.cpp) | 160 | — |
| [lib/src/bg2e/render/GPUTextureGenerator.cpp](../../lib/src/bg2e/render/GPUTextureGenerator.cpp) | 54 | — |
| [lib/src/bg2e/render/IntegratedBakerContext.cpp](../../lib/src/bg2e/render/IntegratedBakerContext.cpp) | 487 | `scene` |
| [lib/src/bg2e/render/IrradianceCubemapRenderer.cpp](../../lib/src/bg2e/render/IrradianceCubemapRenderer.cpp) | 50 | — |
| [lib/src/bg2e/render/LightmapBakeExecutor.cpp](../../lib/src/bg2e/render/LightmapBakeExecutor.cpp) | 80 | `scene` |
| [lib/src/bg2e/render/LightmapBakeExecutor.hpp](../../lib/src/bg2e/render/LightmapBakeExecutor.hpp) | 35 | `base` |
| [lib/src/bg2e/render/LightmapBaker.cpp](../../lib/src/bg2e/render/LightmapBaker.cpp) | 306 | `geo`, `scene` |
| [lib/src/bg2e/render/LightmapCompositionPass.cpp](../../lib/src/bg2e/render/LightmapCompositionPass.cpp) | 160 | — |
| [lib/src/bg2e/render/LightmapCompositionPass.hpp](../../lib/src/bg2e/render/LightmapCompositionPass.hpp) | 62 | — |
| [lib/src/bg2e/render/MaterialBase.cpp](../../lib/src/bg2e/render/MaterialBase.cpp) | 152 | `utils` |
| [lib/src/bg2e/render/RenderLoop.cpp](../../lib/src/bg2e/render/RenderLoop.cpp) | 387 | — |
| [lib/src/bg2e/render/RenderQueue.cpp](../../lib/src/bg2e/render/RenderQueue.cpp) | 172 | — |
| [lib/src/bg2e/render/RenderSettingsPreferences.cpp](../../lib/src/bg2e/render/RenderSettingsPreferences.cpp) | 580 | — |
| [lib/src/bg2e/render/Renderer.cpp](../../lib/src/bg2e/render/Renderer.cpp) | 79 | `base` |
| [lib/src/bg2e/render/RendererBasicForward.cpp](../../lib/src/bg2e/render/RendererBasicForward.cpp) | 386 | `base`, `scene` |
| [lib/src/bg2e/render/RendererDeferred.cpp](../../lib/src/bg2e/render/RendererDeferred.cpp) | 1335 | `base`, `math`, `scene` |
| [lib/src/bg2e/render/SkyboxRenderer.cpp](../../lib/src/bg2e/render/SkyboxRenderer.cpp) | 171 | `geo` |
| [lib/src/bg2e/render/SpecularReflectionCubemapRenderer.cpp](../../lib/src/bg2e/render/SpecularReflectionCubemapRenderer.cpp) | 71 | — |
| [lib/src/bg2e/render/SphereToCubemapRenderer.cpp](../../lib/src/bg2e/render/SphereToCubemapRenderer.cpp) | 301 | `base`, `db`, `geo` |
| [lib/src/bg2e/render/StandaloneBakeBatch.cpp](../../lib/src/bg2e/render/StandaloneBakeBatch.cpp) | 338 | `geo`, `scene` |
| [lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp](../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp) | 935 | `base`, `db`, `json`, `scene` |
| [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../lib/src/bg2e/render/StandaloneBakerContext.cpp) | 680 | `base`, `scene` |
| [lib/src/bg2e/render/Texture_render.cpp](../../lib/src/bg2e/render/Texture_render.cpp) | 232 | `db` |
| [lib/src/bg2e/render/UvMapPreviewRenderer.cpp](../../lib/src/bg2e/render/UvMapPreviewRenderer.cpp) | 317 | `base` |
| [lib/src/bg2e/render/UvSurfacePass.cpp](../../lib/src/bg2e/render/UvSurfacePass.cpp) | 219 | `scene` |
| [lib/src/bg2e/render/UvSurfacePass.hpp](../../lib/src/bg2e/render/UvSurfacePass.hpp) | 64 | `scene` |
| [lib/src/bg2e/render/UvTemporalAccumulator.cpp](../../lib/src/bg2e/render/UvTemporalAccumulator.cpp) | 284 | — |
| [lib/src/bg2e/render/UvTemporalAccumulator.hpp](../../lib/src/bg2e/render/UvTemporalAccumulator.hpp) | 58 | — |
| [lib/src/bg2e/render/ViewpointAnalyzer.cpp](../../lib/src/bg2e/render/ViewpointAnalyzer.cpp) | 559 | `scene` |
| [lib/src/bg2e/render/blue_noise_data.h](../../lib/src/bg2e/render/blue_noise_data.h) | 8205 | — |

### deferred

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/deferred/DeferredLayer.cpp](../../lib/src/bg2e/render/deferred/DeferredLayer.cpp) | 1596 | `scene` |
| [lib/src/bg2e/render/deferred/DenoiseFilter.cpp](../../lib/src/bg2e/render/deferred/DenoiseFilter.cpp) | 255 | — |
| [lib/src/bg2e/render/deferred/DirectPostProcessor.cpp](../../lib/src/bg2e/render/deferred/DirectPostProcessor.cpp) | 71 | — |
| [lib/src/bg2e/render/deferred/FSRPostProcessor.cpp](../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp) | 490 | `base` |
| [lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp](../../lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp) | 215 | — |
| [lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp](../../lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp) | 545 | — |
| [lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp](../../lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp) | 580 | — |
| [lib/src/bg2e/render/deferred/RTReflections.cpp](../../lib/src/bg2e/render/deferred/RTReflections.cpp) | 369 | — |
| [lib/src/bg2e/render/deferred/RenderLayer.cpp](../../lib/src/bg2e/render/deferred/RenderLayer.cpp) | 60 | — |
| [lib/src/bg2e/render/deferred/SMAAPostProcessor.cpp](../../lib/src/bg2e/render/deferred/SMAAPostProcessor.cpp) | 155 | — |
| [lib/src/bg2e/render/deferred/SMAAProcessor.cpp](../../lib/src/bg2e/render/deferred/SMAAProcessor.cpp) | 390 | — |
| [lib/src/bg2e/render/deferred/SkyboxLayer.cpp](../../lib/src/bg2e/render/deferred/SkyboxLayer.cpp) | 73 | — |
| [lib/src/bg2e/render/deferred/TemporalAccumulator.cpp](../../lib/src/bg2e/render/deferred/TemporalAccumulator.cpp) | 496 | — |

### gbuffer

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/gbuffer/GBufferManager.cpp](../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp) | 339 | — |

### vulkan

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/vulkan/Buffer.cpp](../../lib/src/bg2e/render/vulkan/Buffer.cpp) | 131 | `base` |
| [lib/src/bg2e/render/vulkan/Command.cpp](../../lib/src/bg2e/render/vulkan/Command.cpp) | 103 | — |
| [lib/src/bg2e/render/vulkan/DescriptorSet.cpp](../../lib/src/bg2e/render/vulkan/DescriptorSet.cpp) | 202 | — |
| [lib/src/bg2e/render/vulkan/DescriptorSetAllocator.cpp](../../lib/src/bg2e/render/vulkan/DescriptorSetAllocator.cpp) | 194 | `base` |
| [lib/src/bg2e/render/vulkan/Device.cpp](../../lib/src/bg2e/render/vulkan/Device.cpp) | 230 | `base` |
| [lib/src/bg2e/render/vulkan/FrameResources.cpp](../../lib/src/bg2e/render/vulkan/FrameResources.cpp) | 117 | — |
| [lib/src/bg2e/render/vulkan/Image.cpp](../../lib/src/bg2e/render/vulkan/Image.cpp) | 594 | `base` |
| [lib/src/bg2e/render/vulkan/Info.cpp](../../lib/src/bg2e/render/vulkan/Info.cpp) | 263 | — |
| [lib/src/bg2e/render/vulkan/PhysicalDevice.cpp](../../lib/src/bg2e/render/vulkan/PhysicalDevice.cpp) | 526 | `base` |
| [lib/src/bg2e/render/vulkan/Surface.cpp](../../lib/src/bg2e/render/vulkan/Surface.cpp) | 53 | — |
| [lib/src/bg2e/render/vulkan/Swapchain.cpp](../../lib/src/bg2e/render/vulkan/Swapchain.cpp) | 221 | — |
| [lib/src/bg2e/render/vulkan/common.cpp](../../lib/src/bg2e/render/vulkan/common.cpp) | 70 | `base` |
| [lib/src/bg2e/render/vulkan/extensions.cpp](../../lib/src/bg2e/render/vulkan/extensions.cpp) | 209 | `base` |

### vulkan/factory

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/vulkan/factory/ComputePipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/ComputePipeline.cpp) | 102 | `base` |
| [lib/src/bg2e/render/vulkan/factory/DescriptorSetLayout.cpp](../../lib/src/bg2e/render/vulkan/factory/DescriptorSetLayout.cpp) | 68 | — |
| [lib/src/bg2e/render/vulkan/factory/GraphicsPipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/GraphicsPipeline.cpp) | 361 | `base` |
| [lib/src/bg2e/render/vulkan/factory/PipelineLayout.cpp](../../lib/src/bg2e/render/vulkan/factory/PipelineLayout.cpp) | 65 | `base` |
| [lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp) | 365 | `base` |
| [lib/src/bg2e/render/vulkan/factory/Sampler.cpp](../../lib/src/bg2e/render/vulkan/factory/Sampler.cpp) | 62 | — |
| [lib/src/bg2e/render/vulkan/factory/ShaderModule.cpp](../../lib/src/bg2e/render/vulkan/factory/ShaderModule.cpp) | 66 | `base` |

### vulkan/geo

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/vulkan/geo/Mesh.cpp](../../lib/src/bg2e/render/vulkan/geo/Mesh.cpp) | 456 | — |
| [lib/src/bg2e/render/vulkan/geo/VertexDescription.cpp](../../lib/src/bg2e/render/vulkan/geo/VertexDescription.cpp) | 342 | — |

### vulkan/macros

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/vulkan/macros/graphics.cpp](../../lib/src/bg2e/render/vulkan/macros/graphics.cpp) | 171 | — |

### vulkan/rt

| File | Lines | Direct outgoing modules |
|---|---:|---|
| [lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp](../../lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp) | 115 | `scene` |
| [lib/src/bg2e/render/vulkan/rt/RTAccelerationStructureSize.cpp](../../lib/src/bg2e/render/vulkan/rt/RTAccelerationStructureSize.cpp) | 116 | — |
| [lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp](../../lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp) | 246 | — |
| [lib/src/bg2e/render/vulkan/rt/RayTracingMesh.cpp](../../lib/src/bg2e/render/vulkan/rt/RayTracingMesh.cpp) | 250 | — |
| [lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp](../../lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp) | 184 | `scene` |
| [lib/src/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.cpp](../../lib/src/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.cpp) | 75 | — |
| [lib/src/bg2e/render/vulkan/rt/ReflectionLightDataBinding.cpp](../../lib/src/bg2e/render/vulkan/rt/ReflectionLightDataBinding.cpp) | 131 | — |
| [lib/src/bg2e/render/vulkan/rt/utils.cpp](../../lib/src/bg2e/render/vulkan/rt/utils.cpp) | 45 | — |

## 5. Direct outgoing include edges from render

Each entry identifies a concrete source/header and the external engine includes. Names such as scene or DB beneath a Vulkan directory expose responsibilities worth splitting; they do not all belong in GPU.

| File | Includes outside render |
|---|---|
| [lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp](../../lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp) | `bg2e/app/OffscreenApplication.hpp` (line 21)<br>`bg2e/scene/Node.hpp` (line 22) |
| [lib/include/bg2e/render/DefaultRenderLoopDelegate.hpp](../../lib/include/bg2e/render/DefaultRenderLoopDelegate.hpp) | `bg2e/scene/Node.hpp` (line 23) |
| [lib/include/bg2e/render/Engine.hpp](../../lib/include/bg2e/render/Engine.hpp) | `bg2e/gpu/Instance.hpp` (line 22)<br>`bg2e/gpu/vk/Instance.hpp` (line 33) |
| [lib/include/bg2e/render/MaterialBase.hpp](../../lib/include/bg2e/render/MaterialBase.hpp) | `bg2e/base/MaterialAttributes.hpp` (line 21) |
| [lib/include/bg2e/render/RenderLoopDelegate.hpp](../../lib/include/bg2e/render/RenderLoopDelegate.hpp) | `bg2e/scene/Node.hpp` (line 23) |
| [lib/include/bg2e/render/RenderQueue.hpp](../../lib/include/bg2e/render/RenderQueue.hpp) | `bg2e/math/base.hpp` (line 22)<br>`bg2e/scene/Drawable.hpp` (line 23) |
| [lib/include/bg2e/render/RenderSettingsPreferences.hpp](../../lib/include/bg2e/render/RenderSettingsPreferences.hpp) | `bg2e/app/Preferences.hpp` (line 22) |
| [lib/include/bg2e/render/Renderer.hpp](../../lib/include/bg2e/render/Renderer.hpp) | `bg2e/scene/Node.hpp` (line 23)<br>`bg2e/scene/Scene.hpp` (line 24)<br>`bg2e/scene/ResizeViewportVisitor.hpp` (line 28)<br>`bg2e/scene/UpdateVisitor.hpp` (line 29)<br>`bg2e/scene/RenderQueueVisitor.hpp` (line 30)<br>`bg2e/scene/vk/LightDataBinding.hpp` (line 32) |
| [lib/include/bg2e/render/RendererBasicForward.hpp](../../lib/include/bg2e/render/RendererBasicForward.hpp) | `bg2e/scene/vk/FrameDataBinding.hpp` (line 22)<br>`bg2e/scene/vk/ObjectDataBinding.hpp` (line 23)<br>`bg2e/scene/vk/EnvironmentDataBinding.hpp` (line 24)<br>`bg2e/scene/vk/LightDataBinding.hpp` (line 25)<br>`bg2e/scene/ResizeViewportVisitor.hpp` (line 28)<br>`bg2e/scene/UpdateVisitor.hpp` (line 29)<br>`bg2e/scene/DrawVisitor.hpp` (line 30)<br>`bg2e/scene/Scene.hpp` (line 31)<br>`bg2e/scene/Node.hpp` (line 32)<br>`bg2e/scene/RenderQueueVisitor.hpp` (line 33)<br>`bg2e/manipulation/GizmoAndSelectionRenderer.hpp` (line 36) |
| [lib/include/bg2e/render/RendererDeferred.hpp](../../lib/include/bg2e/render/RendererDeferred.hpp) | `bg2e/scene/Scene.hpp` (line 21)<br>`bg2e/scene/Node.hpp` (line 22)<br>`bg2e/scene/vk/DeferredLightDataBinding.hpp` (line 28)<br>`bg2e/manipulation/GizmoAndSelectionRenderer.hpp` (line 31) |
| [lib/include/bg2e/render/SkyboxRenderer.hpp](../../lib/include/bg2e/render/SkyboxRenderer.hpp) | `bg2e/base/PlatformTools.hpp` (line 22) |
| [lib/include/bg2e/render/SphereToCubemapRenderer.hpp](../../lib/include/bg2e/render/SphereToCubemapRenderer.hpp) | `bg2e/base/Image.hpp` (line 22) |
| [lib/include/bg2e/render/StandaloneBakeBatch.hpp](../../lib/include/bg2e/render/StandaloneBakeBatch.hpp) | `bg2e/db/LightmapOutputWriter.hpp` (line 21)<br>`bg2e/geo/GenerateUv2AtlasModifier.hpp` (line 22) |
| [lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp](../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp) | `bg2e/geo/GenerateUv2AtlasModifier.hpp` (line 22) |
| [lib/include/bg2e/render/StandaloneBakerContext.hpp](../../lib/include/bg2e/render/StandaloneBakerContext.hpp) | `bg2e/base/Light.hpp` (line 25) |
| [lib/include/bg2e/render/Texture.hpp](../../lib/include/bg2e/render/Texture.hpp) | `bg2e/base/Texture.hpp` (line 22)<br>`bg2e/base/Color.hpp` (line 23) |
| [lib/include/bg2e/render/UvMapPreviewRenderer.hpp](../../lib/include/bg2e/render/UvMapPreviewRenderer.hpp) | `bg2e/geo/Mesh.hpp` (line 22) |
| [lib/include/bg2e/render/ViewpointAnalyzer.hpp](../../lib/include/bg2e/render/ViewpointAnalyzer.hpp) | `bg2e/math/base.hpp` (line 14) |
| [lib/include/bg2e/render/deferred/DeferredLayer.hpp](../../lib/include/bg2e/render/deferred/DeferredLayer.hpp) | `bg2e/scene/vk/FrameDataBinding.hpp` (line 33)<br>`bg2e/scene/vk/ObjectDataBinding.hpp` (line 34)<br>`bg2e/scene/vk/EnvironmentDataBinding.hpp` (line 35)<br>`bg2e/scene/vk/DeferredLightDataBinding.hpp` (line 36) |
| [lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp](../../lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp) | `bg2e/base/Light.hpp` (line 32) |
| [lib/include/bg2e/render/deferred/RTReflections.hpp](../../lib/include/bg2e/render/deferred/RTReflections.hpp) | `bg2e/base/Light.hpp` (line 32) |
| [lib/include/bg2e/render/deferred/RenderLayer.hpp](../../lib/include/bg2e/render/deferred/RenderLayer.hpp) | `bg2e/scene/Scene.hpp` (line 26) |
| [lib/include/bg2e/render/uniforms/materials.hpp](../../lib/include/bg2e/render/uniforms/materials.hpp) | `bg2e/math/base.hpp` (line 21)<br>`bg2e/base/Color.hpp` (line 22) |
| [lib/include/bg2e/render/vulkan/Surface.hpp](../../lib/include/bg2e/render/vulkan/Surface.hpp) | `bg2e/base/PlatformTools.hpp` (line 22) |
| [lib/include/bg2e/render/vulkan/common.hpp](../../lib/include/bg2e/render/vulkan/common.hpp) | `bg2e/base/PlatformTools.hpp` (line 23)<br>`bg2e/math/base.hpp` (line 68) |
| [lib/include/bg2e/render/vulkan/extensions.hpp](../../lib/include/bg2e/render/vulkan/extensions.hpp) | `bg2e/base/PlatformTools.hpp` (line 22) |
| [lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp](../../lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp) | `bg2e/geo/Mesh.hpp` (line 23) |
| [lib/include/bg2e/render/vulkan/geo/Mesh.hpp](../../lib/include/bg2e/render/vulkan/geo/Mesh.hpp) | `bg2e/geo/Mesh.hpp` (line 22) |
| [lib/include/bg2e/render/vulkan/geo/VertexDescription.hpp](../../lib/include/bg2e/render/vulkan/geo/VertexDescription.hpp) | `bg2e/geo/Vertex.hpp` (line 23) |
| [lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp](../../lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp) | `bg2e/scene/NodeVisitor.hpp` (line 23)<br>`bg2e/math/base.hpp` (line 24) |
| [lib/include/bg2e/render/vulkan/rt/RTMaterialData.h](../../lib/include/bg2e/render/vulkan/rt/RTMaterialData.h) | `bg2e/base/Color.hpp` (line 21)<br>`bg2e/math/base.hpp` (line 22) |
| [lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp) | `bg2e/scene/Node.hpp` (line 24) |
| [lib/include/bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp](../../lib/include/bg2e/render/vulkan/rt/RayTracingVertexInfo.hpp) | `bg2e/geo/Mesh.hpp` (line 23)<br>`bg2e/geo/Vertex.hpp` (line 24) |
| [lib/include/bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp](../../lib/include/bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp) | `bg2e/base/Light.hpp` (line 23) |
| [lib/include/bg2e/render/vulkan/rt/utils.hpp](../../lib/include/bg2e/render/vulkan/rt/utils.hpp) | `bg2e/math/base.hpp` (line 22) |
| [lib/src/bg2e/render/BRDFIntegrationMapTextureGenerator.cpp](../../lib/src/bg2e/render/BRDFIntegrationMapTextureGenerator.cpp) | `bg2e/math/all.hpp` (line 20)<br>`bg2e/base/PlatformTools.hpp` (line 21) |
| [lib/src/bg2e/render/BakerContext.cpp](../../lib/src/bg2e/render/BakerContext.cpp) | `bg2e/scene/Node.hpp` (line 4) |
| [lib/src/bg2e/render/CubemapRenderer.cpp](../../lib/src/bg2e/render/CubemapRenderer.cpp) | `bg2e/geo/cube.hpp` (line 23) |
| [lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp](../../lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp) | `bg2e/app/OffscreenApplication.hpp` (line 20)<br>`bg2e/utils/TextureCache.hpp` (line 21) |
| [lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp](../../lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp) | `bg2e/utils/TextureCache.hpp` (line 23) |
| [lib/src/bg2e/render/Engine.cpp](../../lib/src/bg2e/render/Engine.cpp) | `bg2e/gpu/vk/Instance.hpp` (line 20)<br>`bg2e/base/Log.hpp` (line 25) |
| [lib/src/bg2e/render/IntegratedBakerContext.cpp](../../lib/src/bg2e/render/IntegratedBakerContext.cpp) | `bg2e/scene/EnvironmentComponent.hpp` (line 12)<br>`bg2e/scene/LightComponent.hpp` (line 13)<br>`bg2e/scene/Node.hpp` (line 14)<br>`bg2e/scene/Scene.hpp` (line 15)<br>`bg2e/scene/vk/LightDataBinding.hpp` (line 16) |
| [lib/src/bg2e/render/LightmapBakeExecutor.cpp](../../lib/src/bg2e/render/LightmapBakeExecutor.cpp) | `bg2e/scene/Node.hpp` (line 5) |
| [lib/src/bg2e/render/LightmapBakeExecutor.hpp](../../lib/src/bg2e/render/LightmapBakeExecutor.hpp) | `bg2e/base/Light.hpp` (line 3) |
| [lib/src/bg2e/render/LightmapBaker.cpp](../../lib/src/bg2e/render/LightmapBaker.cpp) | `bg2e/scene/Drawable.hpp` (line 6)<br>`bg2e/scene/DrawableComponent.hpp` (line 7)<br>`bg2e/scene/Mesh.hpp` (line 8)<br>`bg2e/scene/Node.hpp` (line 9)<br>`bg2e/geo/UvAtlasValidator.hpp` (line 13) |
| [lib/src/bg2e/render/MaterialBase.cpp](../../lib/src/bg2e/render/MaterialBase.cpp) | `bg2e/utils/TextureCache.hpp` (line 20) |
| [lib/src/bg2e/render/Renderer.cpp](../../lib/src/bg2e/render/Renderer.cpp) | `bg2e/base/Log.hpp` (line 21) |
| [lib/src/bg2e/render/RendererBasicForward.cpp](../../lib/src/bg2e/render/RendererBasicForward.cpp) | `bg2e/scene/SkyDomeTextureGenerator.hpp` (line 20)<br>`bg2e/base/PlatformTools.hpp` (line 25) |
| [lib/src/bg2e/render/RendererDeferred.cpp](../../lib/src/bg2e/render/RendererDeferred.cpp) | `bg2e/base/Log.hpp` (line 19)<br>`bg2e/scene/SkyDomeTextureGenerator.hpp` (line 22)<br>`bg2e/math/projections.hpp` (line 30) |
| [lib/src/bg2e/render/SkyboxRenderer.cpp](../../lib/src/bg2e/render/SkyboxRenderer.cpp) | `bg2e/geo/cube.hpp` (line 23) |
| [lib/src/bg2e/render/SphereToCubemapRenderer.cpp](../../lib/src/bg2e/render/SphereToCubemapRenderer.cpp) | `bg2e/base/Texture.hpp` (line 23)<br>`bg2e/geo/sphere.hpp` (line 26)<br>`bg2e/db/image.hpp` (line 27) |
| [lib/src/bg2e/render/StandaloneBakeBatch.cpp](../../lib/src/bg2e/render/StandaloneBakeBatch.cpp) | `bg2e/geo/UvAtlasValidator.hpp` (line 21)<br>`bg2e/scene/Drawable.hpp` (line 24)<br>`bg2e/scene/DrawableComponent.hpp` (line 25)<br>`bg2e/scene/Node.hpp` (line 26)<br>`bg2e/scene/Scene.hpp` (line 27) |
| [lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp](../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp) | `bg2e/base/Log.hpp` (line 3)<br>`bg2e/db/mesh_bg2.hpp` (line 4)<br>`bg2e/db/scene.hpp` (line 5)<br>`bg2e/db/scene_bg2.hpp` (line 6)<br>`bg2e/json/JsonParser.hpp` (line 7)<br>`bg2e/json/NodeReader.hpp` (line 8)<br>`bg2e/scene/ComponentFactoryRegistry.hpp` (line 10)<br>`bg2e/scene/Drawable.hpp` (line 11)<br>`bg2e/scene/DrawableComponent.hpp` (line 12)<br>`bg2e/scene/EnvironmentComponent.hpp` (line 13)<br>`bg2e/scene/Node.hpp` (line 14)<br>`bg2e/scene/Scene.hpp` (line 15)<br>`bg2e/scene/TransformComponent.hpp` (line 16) |
| [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../lib/src/bg2e/render/StandaloneBakerContext.cpp) | `bg2e/scene/EnvironmentComponent.hpp` (line 13)<br>`bg2e/scene/LightComponent.hpp` (line 14)<br>`bg2e/scene/Node.hpp` (line 15)<br>`bg2e/scene/ResizeViewportVisitor.hpp` (line 16)<br>`bg2e/scene/Scene.hpp` (line 17)<br>`bg2e/scene/SkyDomeTextureGenerator.hpp` (line 18)<br>`bg2e/scene/UpdateVisitor.hpp` (line 19)<br>`bg2e/base/Texture.hpp` (line 20)<br>`bg2e/scene/vk/LightDataBinding.hpp` (line 22) |
| [lib/src/bg2e/render/Texture_render.cpp](../../lib/src/bg2e/render/Texture_render.cpp) | `bg2e/db/image.hpp` (line 21) |
| [lib/src/bg2e/render/UvMapPreviewRenderer.cpp](../../lib/src/bg2e/render/UvMapPreviewRenderer.cpp) | `bg2e/base/Texture.hpp` (line 23) |
| [lib/src/bg2e/render/UvSurfacePass.cpp](../../lib/src/bg2e/render/UvSurfacePass.cpp) | `bg2e/scene/Drawable.hpp` (line 27) |
| [lib/src/bg2e/render/UvSurfacePass.hpp](../../lib/src/bg2e/render/UvSurfacePass.hpp) | `bg2e/scene/Drawable.hpp` (line 22) |
| [lib/src/bg2e/render/ViewpointAnalyzer.cpp](../../lib/src/bg2e/render/ViewpointAnalyzer.cpp) | `bg2e/scene/BoundingBox.hpp` (line 15)<br>`bg2e/scene/Drawable.hpp` (line 16)<br>`bg2e/scene/DrawableComponent.hpp` (line 17)<br>`bg2e/scene/Node.hpp` (line 18)<br>`bg2e/scene/Scene.hpp` (line 19) |
| [lib/src/bg2e/render/deferred/DeferredLayer.cpp](../../lib/src/bg2e/render/deferred/DeferredLayer.cpp) | `bg2e/scene/Drawable.hpp` (line 22) |
| [lib/src/bg2e/render/deferred/FSRPostProcessor.cpp](../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp) | `bg2e/base/Log.hpp` (line 22) |
| [lib/src/bg2e/render/vulkan/Buffer.cpp](../../lib/src/bg2e/render/vulkan/Buffer.cpp) | `bg2e/base/Log.hpp` (line 19) |
| [lib/src/bg2e/render/vulkan/DescriptorSetAllocator.cpp](../../lib/src/bg2e/render/vulkan/DescriptorSetAllocator.cpp) | `bg2e/base/Log.hpp` (line 19) |
| [lib/src/bg2e/render/vulkan/Device.cpp](../../lib/src/bg2e/render/vulkan/Device.cpp) | `bg2e/base/Log.hpp` (line 20) |
| [lib/src/bg2e/render/vulkan/Image.cpp](../../lib/src/bg2e/render/vulkan/Image.cpp) | `bg2e/base/PlatformTools.hpp` (line 19)<br>`bg2e/base/Log.hpp` (line 24) |
| [lib/src/bg2e/render/vulkan/PhysicalDevice.cpp](../../lib/src/bg2e/render/vulkan/PhysicalDevice.cpp) | `bg2e/base/Log.hpp` (line 20) |
| [lib/src/bg2e/render/vulkan/common.cpp](../../lib/src/bg2e/render/vulkan/common.cpp) | `bg2e/base/PlatformTools.hpp` (line 20) |
| [lib/src/bg2e/render/vulkan/extensions.cpp](../../lib/src/bg2e/render/vulkan/extensions.cpp) | `bg2e/base/PlatformTools.hpp` (line 20)<br>`bg2e/base/Log.hpp` (line 22) |
| [lib/src/bg2e/render/vulkan/factory/ComputePipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/ComputePipeline.cpp) | `bg2e/base/Log.hpp` (line 21) |
| [lib/src/bg2e/render/vulkan/factory/GraphicsPipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/GraphicsPipeline.cpp) | `bg2e/base/Log.hpp` (line 21) |
| [lib/src/bg2e/render/vulkan/factory/PipelineLayout.cpp](../../lib/src/bg2e/render/vulkan/factory/PipelineLayout.cpp) | `bg2e/base/Log.hpp` (line 22) |
| [lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp](../../lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp) | `bg2e/base/Log.hpp` (line 22) |
| [lib/src/bg2e/render/vulkan/factory/ShaderModule.cpp](../../lib/src/bg2e/render/vulkan/factory/ShaderModule.cpp) | `bg2e/base/PlatformTools.hpp` (line 21) |
| [lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp](../../lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp) | `bg2e/scene/TransformComponent.hpp` (line 21)<br>`bg2e/scene/Node.hpp` (line 22)<br>`bg2e/scene/Drawable.hpp` (line 23) |
| [lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp](../../lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp) | `bg2e/scene/Node.hpp` (line 22) |

## 6. Consumers outside render

A row is included when a file has a direct production-header include or a qualified production namespace reference. The evidence column lists production includes or the first qualified reference line. Consumers reached only through `scene::Drawable`, `scene/all.hpp`, `bg2e/all.hpp`, or namespace aliases may be absent.

| Root | Files with direct render includes | Files with includes or qualified references |
|---|---:|---:|
| `lib/include/bg2e` | 24 | 45 |
| `lib/src/bg2e` | 17 | 45 |
| `apps` | 7 | 19 |
| `examples` | 1 | 27 |

### lib/include/bg2e

| Consumer file | Direct evidence |
|---|---|
| [lib/include/bg2e/all.hpp](../../lib/include/bg2e/all.hpp) | `render/all.hpp` |
| [lib/include/bg2e/app/Application.hpp](../../lib/include/bg2e/app/Application.hpp) | `render/RenderLoop.hpp`; qualified reference at line 35 |
| [lib/include/bg2e/app/GPUSelectionDialog.hpp](../../lib/include/bg2e/app/GPUSelectionDialog.hpp) | `render/vulkan/PhysicalDevice.hpp`; qualified reference at line 34 |
| [lib/include/bg2e/app/OffscreenApplication.hpp](../../lib/include/bg2e/app/OffscreenApplication.hpp) | `render/Engine.hpp`; qualified reference at line 103 |
| [lib/include/bg2e/db/LightmapOutputWriter.hpp](../../lib/include/bg2e/db/LightmapOutputWriter.hpp) | qualified reference at line 76 |
| [lib/include/bg2e/db/mesh_bg2.hpp](../../lib/include/bg2e/db/mesh_bg2.hpp) | `render/Engine.hpp`; qualified reference at line 68 |
| [lib/include/bg2e/db/mesh_obj.hpp](../../lib/include/bg2e/db/mesh_obj.hpp) | `render/Engine.hpp`; qualified reference at line 79 |
| [lib/include/bg2e/db/scene.hpp](../../lib/include/bg2e/db/scene.hpp) | qualified reference at line 37 |
| [lib/include/bg2e/db/scene_bg2.hpp](../../lib/include/bg2e/db/scene_bg2.hpp) | qualified reference at line 41 |
| [lib/include/bg2e/db/scene_gltf.hpp](../../lib/include/bg2e/db/scene_gltf.hpp) | `render/Engine.hpp`; qualified reference at line 32 |
| [lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp](../../lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp) | `render/Engine.hpp`; qualified reference at line 37 |
| [lib/include/bg2e/manipulation/GizmoComponent.hpp](../../lib/include/bg2e/manipulation/GizmoComponent.hpp) | qualified reference at line 73 |
| [lib/include/bg2e/manipulation/SelectionManager.hpp](../../lib/include/bg2e/manipulation/SelectionManager.hpp) | `render/Engine.hpp`, `render/vulkan/Image.hpp`; qualified reference at line 72 |
| [lib/include/bg2e/scene/CameraComponent.hpp](../../lib/include/bg2e/scene/CameraComponent.hpp) | qualified reference at line 80 |
| [lib/include/bg2e/scene/Chain.hpp](../../lib/include/bg2e/scene/Chain.hpp) | qualified reference at line 32 |
| [lib/include/bg2e/scene/ChainJoint.hpp](../../lib/include/bg2e/scene/ChainJoint.hpp) | qualified reference at line 41 |
| [lib/include/bg2e/scene/Component.hpp](../../lib/include/bg2e/scene/Component.hpp) | qualified reference at line 118 |
| [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp) | qualified reference at line 40 |
| [lib/include/bg2e/scene/Drawable.hpp](../../lib/include/bg2e/scene/Drawable.hpp) | `render/Engine.hpp`, `render/MaterialBase.hpp`, `render/vulkan/geo/Mesh.hpp`, `render/vulkan/rt/RayTracingMesh.hpp`; qualified reference at line 42 |
| [lib/include/bg2e/scene/DrawableComponent.hpp](../../lib/include/bg2e/scene/DrawableComponent.hpp) | qualified reference at line 52 |
| [lib/include/bg2e/scene/EnvironmentComponent.hpp](../../lib/include/bg2e/scene/EnvironmentComponent.hpp) | qualified reference at line 58 |
| [lib/include/bg2e/scene/FixedScaleTransformController.hpp](../../lib/include/bg2e/scene/FixedScaleTransformController.hpp) | qualified reference at line 33 |
| [lib/include/bg2e/scene/LightComponent.hpp](../../lib/include/bg2e/scene/LightComponent.hpp) | qualified reference at line 48 |
| [lib/include/bg2e/scene/Mesh.hpp](../../lib/include/bg2e/scene/Mesh.hpp) | `render/vulkan/geo/Mesh.hpp`; qualified reference at line 29 |
| [lib/include/bg2e/scene/Node.hpp](../../lib/include/bg2e/scene/Node.hpp) | qualified reference at line 196 |
| [lib/include/bg2e/scene/OrbitCameraComponent.hpp](../../lib/include/bg2e/scene/OrbitCameraComponent.hpp) | qualified reference at line 34 |
| [lib/include/bg2e/scene/PolarTransformController.hpp](../../lib/include/bg2e/scene/PolarTransformController.hpp) | qualified reference at line 33 |
| [lib/include/bg2e/scene/RenderQueueVisitor.hpp](../../lib/include/bg2e/scene/RenderQueueVisitor.hpp) | `render/RenderQueue.hpp`; qualified reference at line 35 |
| [lib/include/bg2e/scene/ResizeViewportVisitor.hpp](../../lib/include/bg2e/scene/ResizeViewportVisitor.hpp) | `render/Engine.hpp` |
| [lib/include/bg2e/scene/Scene.hpp](../../lib/include/bg2e/scene/Scene.hpp) | qualified reference at line 38 |
| [lib/include/bg2e/scene/TransformComponent.hpp](../../lib/include/bg2e/scene/TransformComponent.hpp) | qualified reference at line 91 |
| [lib/include/bg2e/scene/vk/DeferredLightDataBinding.hpp](../../lib/include/bg2e/scene/vk/DeferredLightDataBinding.hpp) | `render/vulkan/PipelineDataBinding.hpp`; qualified reference at line 33 |
| [lib/include/bg2e/scene/vk/EnvironmentDataBinding.hpp](../../lib/include/bg2e/scene/vk/EnvironmentDataBinding.hpp) | `render/EnvironmentResources.hpp`, `render/vulkan/FrameResources.hpp`, `render/vulkan/PipelineDataBinding.hpp`; qualified reference at line 29 |
| [lib/include/bg2e/scene/vk/FrameDataBinding.hpp](../../lib/include/bg2e/scene/vk/FrameDataBinding.hpp) | `render/vulkan/FrameResources.hpp`, `render/vulkan/PipelineDataBinding.hpp`; qualified reference at line 28 |
| [lib/include/bg2e/scene/vk/LightDataBinding.hpp](../../lib/include/bg2e/scene/vk/LightDataBinding.hpp) | `render/vulkan/PipelineDataBinding.hpp`; qualified reference at line 33 |
| [lib/include/bg2e/scene/vk/ObjectDataBinding.hpp](../../lib/include/bg2e/scene/vk/ObjectDataBinding.hpp) | `render/MaterialBase.hpp`, `render/uniforms/materials.hpp`, `render/vulkan/FrameResources.hpp`, `render/vulkan/PipelineDataBinding.hpp`; qualified reference at line 30 |
| [lib/include/bg2e/ui/MaterialEditor.hpp](../../lib/include/bg2e/ui/MaterialEditor.hpp) | `render/MaterialBase.hpp`; qualified reference at line 41 |
| [lib/include/bg2e/ui/RenderSettingsWindow.hpp](../../lib/include/bg2e/ui/RenderSettingsWindow.hpp) | qualified reference at line 31 |
| [lib/include/bg2e/ui/TextureWidgets.hpp](../../lib/include/bg2e/ui/TextureWidgets.hpp) | `render/MaterialBase.hpp`, `render/Texture.hpp`; qualified reference at line 39 |
| [lib/include/bg2e/ui/UserInterface.hpp](../../lib/include/bg2e/ui/UserInterface.hpp) | `render/Engine.hpp`; qualified reference at line 46 |
| [lib/include/bg2e/ui/UserInterfaceDelegate.hpp](../../lib/include/bg2e/ui/UserInterfaceDelegate.hpp) | `render/Engine.hpp`; qualified reference at line 44 |
| [lib/include/bg2e/ui/UvMapPreview.hpp](../../lib/include/bg2e/ui/UvMapPreview.hpp) | `render/UvMapPreviewRenderer.hpp`; qualified reference at line 34 |
| [lib/include/bg2e/utils/MaterialModifier.hpp](../../lib/include/bg2e/utils/MaterialModifier.hpp) | qualified reference at line 35 |
| [lib/include/bg2e/utils/SceneCameraUtils.hpp](../../lib/include/bg2e/utils/SceneCameraUtils.hpp) | qualified reference at line 44 |
| [lib/include/bg2e/utils/TextureCache.hpp](../../lib/include/bg2e/utils/TextureCache.hpp) | `render/Engine.hpp`, `render/Texture.hpp`; qualified reference at line 38 |

### lib/src/bg2e

| Consumer file | Direct evidence |
|---|---|
| [lib/src/bg2e/app/GPUSelectionDialog.cpp](../../lib/src/bg2e/app/GPUSelectionDialog.cpp) | qualified reference at line 42 |
| [lib/src/bg2e/app/OffscreenApplication.cpp](../../lib/src/bg2e/app/OffscreenApplication.cpp) | `render/vulkan/all.hpp`; qualified reference at line 36 |
| [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp) | qualified reference at line 74 |
| [lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp](../../lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp) | `render/Engine.hpp`, `render/RenderLoop.hpp`; qualified reference at line 74 |
| [lib/src/bg2e/db/LightmapOutputWriter.cpp](../../lib/src/bg2e/db/LightmapOutputWriter.cpp) | `render/LightmapBaker.hpp`; qualified reference at line 160 |
| [lib/src/bg2e/db/mesh_bg2.cpp](../../lib/src/bg2e/db/mesh_bg2.cpp) | qualified reference at line 606 |
| [lib/src/bg2e/db/mesh_obj_db.cpp](../../lib/src/bg2e/db/mesh_obj_db.cpp) | qualified reference at line 287 |
| [lib/src/bg2e/db/scene.cpp](../../lib/src/bg2e/db/scene.cpp) | qualified reference at line 150 |
| [lib/src/bg2e/db/scene_bg2.cpp](../../lib/src/bg2e/db/scene_bg2.cpp) | qualified reference at line 34 |
| [lib/src/bg2e/db/scene_gltf.cpp](../../lib/src/bg2e/db/scene_gltf.cpp) | qualified reference at line 593 |
| [lib/src/bg2e/manipulation/GizmoAndSelectionRenderer.cpp](../../lib/src/bg2e/manipulation/GizmoAndSelectionRenderer.cpp) | `render/all.hpp`; qualified reference at line 30 |
| [lib/src/bg2e/manipulation/GizmoComponent.cpp](../../lib/src/bg2e/manipulation/GizmoComponent.cpp) | qualified reference at line 76 |
| [lib/src/bg2e/manipulation/SelectionManager.cpp](../../lib/src/bg2e/manipulation/SelectionManager.cpp) | `render/all.hpp`; qualified reference at line 29 |
| [lib/src/bg2e/scene/CameraComponent.cpp](../../lib/src/bg2e/scene/CameraComponent.cpp) | qualified reference at line 65 |
| [lib/src/bg2e/scene/Chain.cpp](../../lib/src/bg2e/scene/Chain.cpp) | qualified reference at line 76 |
| [lib/src/bg2e/scene/ChainJoint.cpp](../../lib/src/bg2e/scene/ChainJoint.cpp) | qualified reference at line 28 |
| [lib/src/bg2e/scene/Component.cpp](../../lib/src/bg2e/scene/Component.cpp) | qualified reference at line 51 |
| [lib/src/bg2e/scene/ComponentFactoryRegistry.cpp](../../lib/src/bg2e/scene/ComponentFactoryRegistry.cpp) | qualified reference at line 60 |
| [lib/src/bg2e/scene/Drawable.cpp](../../lib/src/bg2e/scene/Drawable.cpp) | qualified reference at line 215 |
| [lib/src/bg2e/scene/DrawableComponent.cpp](../../lib/src/bg2e/scene/DrawableComponent.cpp) | qualified reference at line 76 |
| [lib/src/bg2e/scene/EnvironmentComponent.cpp](../../lib/src/bg2e/scene/EnvironmentComponent.cpp) | qualified reference at line 63 |
| [lib/src/bg2e/scene/FixedScaleTransformController.cpp](../../lib/src/bg2e/scene/FixedScaleTransformController.cpp) | qualified reference at line 36 |
| [lib/src/bg2e/scene/LightComponent.cpp](../../lib/src/bg2e/scene/LightComponent.cpp) | qualified reference at line 60 |
| [lib/src/bg2e/scene/Node.cpp](../../lib/src/bg2e/scene/Node.cpp) | qualified reference at line 254 |
| [lib/src/bg2e/scene/OrbitCameraComponent.cpp](../../lib/src/bg2e/scene/OrbitCameraComponent.cpp) | qualified reference at line 37 |
| [lib/src/bg2e/scene/PolarTransformController.cpp](../../lib/src/bg2e/scene/PolarTransformController.cpp) | qualified reference at line 106 |
| [lib/src/bg2e/scene/RenderQueueVisitor.cpp](../../lib/src/bg2e/scene/RenderQueueVisitor.cpp) | `render/MaterialBase.hpp`, `render/vulkan/geo/Mesh.hpp`; qualified reference at line 44 |
| [lib/src/bg2e/scene/Scene.cpp](../../lib/src/bg2e/scene/Scene.cpp) | qualified reference at line 29 |
| [lib/src/bg2e/scene/TransformComponent.cpp](../../lib/src/bg2e/scene/TransformComponent.cpp) | qualified reference at line 247 |
| [lib/src/bg2e/scene/vk/DeferredLightDataBinding.cpp](../../lib/src/bg2e/scene/vk/DeferredLightDataBinding.cpp) | `render/vulkan/factory/DescriptorSetLayout.hpp`, `render/vulkan/macros/frame_resources.hpp`; qualified reference at line 25 |
| [lib/src/bg2e/scene/vk/EnvironmentDataBinding.cpp](../../lib/src/bg2e/scene/vk/EnvironmentDataBinding.cpp) | `render/vulkan/factory/DescriptorSetLayout.hpp`, `render/vulkan/macros/frame_resources.hpp`; qualified reference at line 25 |
| [lib/src/bg2e/scene/vk/FrameDataBinding.cpp](../../lib/src/bg2e/scene/vk/FrameDataBinding.cpp) | `render/vulkan/factory/DescriptorSetLayout.hpp`, `render/vulkan/macros/frame_resources.hpp`; qualified reference at line 25 |
| [lib/src/bg2e/scene/vk/LightDataBinding.cpp](../../lib/src/bg2e/scene/vk/LightDataBinding.cpp) | `render/vulkan/factory/DescriptorSetLayout.hpp`, `render/vulkan/macros/frame_resources.hpp`; qualified reference at line 25 |
| [lib/src/bg2e/scene/vk/ObjectDataBinding.cpp](../../lib/src/bg2e/scene/vk/ObjectDataBinding.cpp) | `render/uniforms/materials.hpp`, `render/vulkan/factory/DescriptorSetLayout.hpp`, `render/vulkan/macros/frame_resources.hpp`; qualified reference at line 26 |
| [lib/src/bg2e/ui/ImGuiMetalBackend.cpp](../../lib/src/bg2e/ui/ImGuiMetalBackend.cpp) | qualified reference at line 44 |
| [lib/src/bg2e/ui/ImGuiVulkanBackend.cpp](../../lib/src/bg2e/ui/ImGuiVulkanBackend.cpp) | `render/vulkan/Info.hpp`, `render/vulkan/extensions.hpp`; qualified reference at line 60 |
| [lib/src/bg2e/ui/MaterialEditor.cpp](../../lib/src/bg2e/ui/MaterialEditor.cpp) | `render/vulkan/Image.hpp`, `render/vulkan/common.hpp`; qualified reference at line 38 |
| [lib/src/bg2e/ui/RenderSettingsWindow.cpp](../../lib/src/bg2e/ui/RenderSettingsWindow.cpp) | `render/RenderSettingsPreferences.hpp`, `render/RendererDeferred.hpp`; qualified reference at line 27 |
| [lib/src/bg2e/ui/TextureWidgets.cpp](../../lib/src/bg2e/ui/TextureWidgets.cpp) | `render/Engine.hpp`; qualified reference at line 36 |
| [lib/src/bg2e/ui/UserInterface.cpp](../../lib/src/bg2e/ui/UserInterface.cpp) | qualified reference at line 104 |
| [lib/src/bg2e/ui/UvMapPreview.cpp](../../lib/src/bg2e/ui/UvMapPreview.cpp) | `render/Texture.hpp`; qualified reference at line 34 |
| [lib/src/bg2e/ui/detail/ImGuiBackend.hpp](../../lib/src/bg2e/ui/detail/ImGuiBackend.hpp) | qualified reference at line 30 |
| [lib/src/bg2e/utils/MaterialModifier.cpp](../../lib/src/bg2e/utils/MaterialModifier.cpp) | qualified reference at line 371 |
| [lib/src/bg2e/utils/SceneCameraUtils.cpp](../../lib/src/bg2e/utils/SceneCameraUtils.cpp) | `render/ViewpointAnalyzer.hpp`; qualified reference at line 68 |
| [lib/src/bg2e/utils/TextureCache.cpp](../../lib/src/bg2e/utils/TextureCache.cpp) | qualified reference at line 42 |

### apps

| Consumer file | Direct evidence |
|---|---|
| [apps/bg2e_composer/src/AppDelegate.cpp](../../apps/bg2e_composer/src/AppDelegate.cpp) | qualified reference at line 27 |
| [apps/bg2e_composer/src/AppDelegate.hpp](../../apps/bg2e_composer/src/AppDelegate.hpp) | `render/RenderSettingsPreferences.hpp`; qualified reference at line 39 |
| [apps/bg2e_composer/src/SceneLightmapWindow.cpp](../../apps/bg2e_composer/src/SceneLightmapWindow.cpp) | qualified reference at line 118 |
| [apps/bg2e_composer/src/StageScene.cpp](../../apps/bg2e_composer/src/StageScene.cpp) | qualified reference at line 88 |
| [apps/bg2e_composer/src/StageScene.hpp](../../apps/bg2e_composer/src/StageScene.hpp) | `render/Engine.hpp`, `render/ViewpointAnalyzer.hpp`; qualified reference at line 40 |
| [apps/bg2e_composer/src/ToolBar.cpp](../../apps/bg2e_composer/src/ToolBar.cpp) | `render/RenderSettingsPreferences.hpp`; qualified reference at line 29 |
| [apps/bg2e_composer/src/ToolBar.hpp](../../apps/bg2e_composer/src/ToolBar.hpp) | qualified reference at line 42 |
| [apps/lightmap_generator/src/CommandLine.cpp](../../apps/lightmap_generator/src/CommandLine.cpp) | qualified reference at line 262 |
| [apps/lightmap_generator/src/CommandLine.hpp](../../apps/lightmap_generator/src/CommandLine.hpp) | `render/StandaloneBakeBatch.hpp`; qualified reference at line 42 |
| [apps/lightmap_generator/src/main.cpp](../../apps/lightmap_generator/src/main.cpp) | qualified reference at line 36 |
| [apps/model_edit/src/AppDelegate.cpp](../../apps/model_edit/src/AppDelegate.cpp) | qualified reference at line 22 |
| [apps/model_edit/src/AppDelegate.hpp](../../apps/model_edit/src/AppDelegate.hpp) | `render/RenderSettingsPreferences.hpp`; qualified reference at line 31 |
| [apps/model_edit/src/EnvironmentSettings.cpp](../../apps/model_edit/src/EnvironmentSettings.cpp) | qualified reference at line 26 |
| [apps/model_edit/src/EnvironmentSettings.hpp](../../apps/model_edit/src/EnvironmentSettings.hpp) | qualified reference at line 28 |
| [apps/model_edit/src/ModelLightmapWindow.cpp](../../apps/model_edit/src/ModelLightmapWindow.cpp) | qualified reference at line 68 |
| [apps/model_edit/src/StageScene.cpp](../../apps/model_edit/src/StageScene.cpp) | qualified reference at line 31 |
| [apps/model_edit/src/StageScene.hpp](../../apps/model_edit/src/StageScene.hpp) | `render/Engine.hpp`, `render/ViewpointAnalyzer.hpp`; qualified reference at line 38 |
| [apps/model_edit/src/ToolBar.cpp](../../apps/model_edit/src/ToolBar.cpp) | `render/RenderSettingsPreferences.hpp`; qualified reference at line 27 |
| [apps/model_edit/src/ToolBar.hpp](../../apps/model_edit/src/ToolBar.hpp) | qualified reference at line 41 |

### examples

| Consumer file | Direct evidence |
|---|---|
| [examples/01_setup/src/main.cpp](../../examples/01_setup/src/main.cpp) | qualified reference at line 20 |
| [examples/02_compute_shader/src/main.cpp](../../examples/02_compute_shader/src/main.cpp) | qualified reference at line 20 |
| [examples/02_ui/src/main.cpp](../../examples/02_ui/src/main.cpp) | qualified reference at line 20 |
| [examples/03_vertex_buffer/src/main.cpp](../../examples/03_vertex_buffer/src/main.cpp) | `render/vulkan/Image.hpp`; qualified reference at line 27 |
| [examples/04_submeshes/src/main.cpp](../../examples/04_submeshes/src/main.cpp) | qualified reference at line 22 |
| [examples/05_sky_sphere/src/main.cpp](../../examples/05_sky_sphere/src/main.cpp) | qualified reference at line 23 |
| [examples/06_cubemap_renderer/src/main.cpp](../../examples/06_cubemap_renderer/src/main.cpp) | qualified reference at line 23 |
| [examples/07_environment_render/src/main.cpp](../../examples/07_environment_render/src/main.cpp) | qualified reference at line 23 |
| [examples/08_multi_color_attachments/src/main.cpp](../../examples/08_multi_color_attachments/src/main.cpp) | qualified reference at line 24 |
| [examples/09_drawable/src/main.cpp](../../examples/09_drawable/src/main.cpp) | qualified reference at line 46 |
| [examples/10_scene_objects/src/main.cpp](../../examples/10_scene_objects/src/main.cpp) | qualified reference at line 43 |
| [examples/11_scene_renderer_raw/src/main.cpp](../../examples/11_scene_renderer_raw/src/main.cpp) | qualified reference at line 40 |
| [examples/12_scene_renderer/src/main.cpp](../../examples/12_scene_renderer/src/main.cpp) | qualified reference at line 40 |
| [examples/13_pbr_demo/src/main.cpp](../../examples/13_pbr_demo/src/main.cpp) | qualified reference at line 111 |
| [examples/14_bg2_model_load/src/main.cpp](../../examples/14_bg2_model_load/src/main.cpp) | qualified reference at line 116 |
| [examples/15_offscreen_render_cli/src/main.cpp](../../examples/15_offscreen_render_cli/src/main.cpp) | qualified reference at line 33 |
| [examples/16_offscreen_scene_render_cli/src/main.cpp](../../examples/16_offscreen_scene_render_cli/src/main.cpp) | qualified reference at line 20 |
| [examples/17_deferred_renderer/src/main.cpp](../../examples/17_deferred_renderer/src/main.cpp) | qualified reference at line 52 |
| [examples/18_material_modifier/src/main.cpp](../../examples/18_material_modifier/src/main.cpp) | qualified reference at line 14 |
| [examples/debug-app/src/DevSceneDelegate.cpp](../../examples/debug-app/src/DevSceneDelegate.cpp) | qualified reference at line 20 |
| [examples/debug-app/src/DevSceneDelegate.hpp](../../examples/debug-app/src/DevSceneDelegate.hpp) | qualified reference at line 22 |
| [examples/debug-app/src/LoadGltfSceneDelegate.cpp](../../examples/debug-app/src/LoadGltfSceneDelegate.cpp) | qualified reference at line 20 |
| [examples/debug-app/src/LoadGltfSceneDelegate.hpp](../../examples/debug-app/src/LoadGltfSceneDelegate.hpp) | qualified reference at line 22 |
| [examples/debug-app/src/PickSelectionDelegate.cpp](../../examples/debug-app/src/PickSelectionDelegate.cpp) | qualified reference at line 20 |
| [examples/debug-app/src/PickSelectionDelegate.hpp](../../examples/debug-app/src/PickSelectionDelegate.hpp) | qualified reference at line 22 |
| [examples/debug-app/src/SpheresSceneDelegate.cpp](../../examples/debug-app/src/SpheresSceneDelegate.cpp) | qualified reference at line 20 |
| [examples/debug-app/src/SpheresSceneDelegate.hpp](../../examples/debug-app/src/SpheresSceneDelegate.hpp) | qualified reference at line 22 |

## 7. Shader inventory and primitive examples

The following engine-wide shader sources include shared and forward files. Forward scene rendering is excluded from draw, but shared lighting/environment helpers remain relevant. Test shaders are listed for completeness, not treated as required public engine features.

| Shader source | Lines |
|---|---:|
| [shaders/src/glsl/basic_forward.frag.glsl](../../shaders/src/glsl/basic_forward.frag.glsl) | 130 |
| [shaders/src/glsl/basic_forward.vert.glsl](../../shaders/src/glsl/basic_forward.vert.glsl) | 66 |
| [shaders/src/glsl/basic_forward_rt_shadows.frag.glsl](../../shaders/src/glsl/basic_forward_rt_shadows.frag.glsl) | 141 |
| [shaders/src/glsl/brdf_lut.comp.glsl](../../shaders/src/glsl/brdf_lut.comp.glsl) | 113 |
| [shaders/src/glsl/color_attachment_canvas.vert.glsl](../../shaders/src/glsl/color_attachment_canvas.vert.glsl) | 46 |
| [shaders/src/glsl/color_correction.comp.glsl](../../shaders/src/glsl/color_correction.comp.glsl) | 38 |
| [shaders/src/glsl/cubemap_renderer.vert.glsl](../../shaders/src/glsl/cubemap_renderer.vert.glsl) | 51 |
| [shaders/src/glsl/deferred_composite.frag.glsl](../../shaders/src/glsl/deferred_composite.frag.glsl) | 150 |
| [shaders/src/glsl/deferred_composite.vert.glsl](../../shaders/src/glsl/deferred_composite.vert.glsl) | 42 |
| [shaders/src/glsl/deferred_composite_rt.frag.glsl](../../shaders/src/glsl/deferred_composite_rt.frag.glsl) | 286 |
| [shaders/src/glsl/deferred_debug_blit.frag.glsl](../../shaders/src/glsl/deferred_debug_blit.frag.glsl) | 59 |
| [shaders/src/glsl/deferred_debug_blit.vert.glsl](../../shaders/src/glsl/deferred_debug_blit.vert.glsl) | 42 |
| [shaders/src/glsl/deferred_gbuffer.frag.glsl](../../shaders/src/glsl/deferred_gbuffer.frag.glsl) | 101 |
| [shaders/src/glsl/deferred_gbuffer.vert.glsl](../../shaders/src/glsl/deferred_gbuffer.vert.glsl) | 60 |
| [shaders/src/glsl/denoise_bilateral.comp.glsl](../../shaders/src/glsl/denoise_bilateral.comp.glsl) | 101 |
| [shaders/src/glsl/denoise_bilateral_hdr.comp.glsl](../../shaders/src/glsl/denoise_bilateral_hdr.comp.glsl) | 99 |
| [shaders/src/glsl/gizmo.frag.glsl](../../shaders/src/glsl/gizmo.frag.glsl) | 28 |
| [shaders/src/glsl/gizmo.vert.glsl](../../shaders/src/glsl/gizmo.vert.glsl) | 36 |
| [shaders/src/glsl/irradiance_map_renderer.frag.glsl](../../shaders/src/glsl/irradiance_map_renderer.frag.glsl) | 65 |
| [shaders/src/glsl/lib/basic_lighting.glsl](../../shaders/src/glsl/lib/basic_lighting.glsl) | 99 |
| [shaders/src/glsl/lib/blue_noise.glsl](../../shaders/src/glsl/lib/blue_noise.glsl) | 68 |
| [shaders/src/glsl/lib/color_correction.glsl](../../shaders/src/glsl/lib/color_correction.glsl) | 93 |
| [shaders/src/glsl/lib/constants.glsl](../../shaders/src/glsl/lib/constants.glsl) | 19 |
| [shaders/src/glsl/lib/deferred_utils.glsl](../../shaders/src/glsl/lib/deferred_utils.glsl) | 212 |
| [shaders/src/glsl/lib/normal_map.glsl](../../shaders/src/glsl/lib/normal_map.glsl) | 28 |
| [shaders/src/glsl/lib/pbr.glsl](../../shaders/src/glsl/lib/pbr.glsl) | 257 |
| [shaders/src/glsl/lib/ray_tracing.glsl](../../shaders/src/glsl/lib/ray_tracing.glsl) | 184 |
| [shaders/src/glsl/lib/rt_material_data.glsl](../../shaders/src/glsl/lib/rt_material_data.glsl) | 205 |
| [shaders/src/glsl/lib/smaa.glsl](../../shaders/src/glsl/lib/smaa.glsl) | 38 |
| [shaders/src/glsl/lib/uniforms.glsl](../../shaders/src/glsl/lib/uniforms.glsl) | 164 |
| [shaders/src/glsl/lightmap_accumulation.comp.glsl](../../shaders/src/glsl/lightmap_accumulation.comp.glsl) | 39 |
| [shaders/src/glsl/lightmap_composition.comp.glsl](../../shaders/src/glsl/lightmap_composition.comp.glsl) | 54 |
| [shaders/src/glsl/lightmap_dilation.comp.glsl](../../shaders/src/glsl/lightmap_dilation.comp.glsl) | 49 |
| [shaders/src/glsl/motion_vectors.comp.glsl](../../shaders/src/glsl/motion_vectors.comp.glsl) | 72 |
| [shaders/src/glsl/pick_selection.frag.glsl](../../shaders/src/glsl/pick_selection.frag.glsl) | 36 |
| [shaders/src/glsl/pick_selection.vert.glsl](../../shaders/src/glsl/pick_selection.vert.glsl) | 36 |
| [shaders/src/glsl/rt_alpha_test.rahit.glsl](../../shaders/src/glsl/rt_alpha_test.rahit.glsl) | 30 |
| [shaders/src/glsl/rt_ao.comp.glsl](../../shaders/src/glsl/rt_ao.comp.glsl) | 130 |
| [shaders/src/glsl/rt_ao_uv.comp.glsl](../../shaders/src/glsl/rt_ao_uv.comp.glsl) | 110 |
| [shaders/src/glsl/rt_gi.rchit.glsl](../../shaders/src/glsl/rt_gi.rchit.glsl) | 118 |
| [shaders/src/glsl/rt_gi.rgen.glsl](../../shaders/src/glsl/rt_gi.rgen.glsl) | 151 |
| [shaders/src/glsl/rt_gi.rmiss.glsl](../../shaders/src/glsl/rt_gi.rmiss.glsl) | 22 |
| [shaders/src/glsl/rt_gi_uv.rgen.glsl](../../shaders/src/glsl/rt_gi_uv.rgen.glsl) | 139 |
| [shaders/src/glsl/rt_reflections.rchit.glsl](../../shaders/src/glsl/rt_reflections.rchit.glsl) | 115 |
| [shaders/src/glsl/rt_reflections.rgen.glsl](../../shaders/src/glsl/rt_reflections.rgen.glsl) | 186 |
| [shaders/src/glsl/rt_reflections.rmiss.glsl](../../shaders/src/glsl/rt_reflections.rmiss.glsl) | 18 |
| [shaders/src/glsl/selection_highlight.frag.glsl](../../shaders/src/glsl/selection_highlight.frag.glsl) | 28 |
| [shaders/src/glsl/selection_highlight.vert.glsl](../../shaders/src/glsl/selection_highlight.vert.glsl) | 36 |
| [shaders/src/glsl/skybox_renderer.frag.glsl](../../shaders/src/glsl/skybox_renderer.frag.glsl) | 46 |
| [shaders/src/glsl/skybox_renderer.vert.glsl](../../shaders/src/glsl/skybox_renderer.vert.glsl) | 37 |
| [shaders/src/glsl/smaa_blend_weight.comp.glsl](../../shaders/src/glsl/smaa_blend_weight.comp.glsl) | 225 |
| [shaders/src/glsl/smaa_edge_detection.comp.glsl](../../shaders/src/glsl/smaa_edge_detection.comp.glsl) | 110 |
| [shaders/src/glsl/smaa_neighborhood_blend.comp.glsl](../../shaders/src/glsl/smaa_neighborhood_blend.comp.glsl) | 108 |
| [shaders/src/glsl/specular_reflection_renderer.frag.glsl](../../shaders/src/glsl/specular_reflection_renderer.frag.glsl) | 116 |
| [shaders/src/glsl/sphere_to_cubemap.frag.glsl](../../shaders/src/glsl/sphere_to_cubemap.frag.glsl) | 33 |
| [shaders/src/glsl/sphere_to_cubemap.vert.glsl](../../shaders/src/glsl/sphere_to_cubemap.vert.glsl) | 51 |
| [shaders/src/glsl/temporal_accumulation.comp.glsl](../../shaders/src/glsl/temporal_accumulation.comp.glsl) | 181 |
| [shaders/src/glsl/test/color_attachment_canvas_test.frag.glsl](../../shaders/src/glsl/test/color_attachment_canvas_test.frag.glsl) | 35 |
| [shaders/src/glsl/test/environment_render_example.frag.glsl](../../shaders/src/glsl/test/environment_render_example.frag.glsl) | 34 |
| [shaders/src/glsl/test/environment_render_example.vert.glsl](../../shaders/src/glsl/test/environment_render_example.vert.glsl) | 47 |
| [shaders/src/glsl/test/scene_objects.frag.glsl](../../shaders/src/glsl/test/scene_objects.frag.glsl) | 46 |
| [shaders/src/glsl/test/scene_objects.vert.glsl](../../shaders/src/glsl/test/scene_objects.vert.glsl) | 47 |
| [shaders/src/glsl/test/test.frag.glsl](../../shaders/src/glsl/test/test.frag.glsl) | 28 |
| [shaders/src/glsl/test/test.vert.glsl](../../shaders/src/glsl/test/test.vert.glsl) | 37 |
| [shaders/src/glsl/test/texture.frag.glsl](../../shaders/src/glsl/test/texture.frag.glsl) | 31 |
| [shaders/src/glsl/test/texture.vert.glsl](../../shaders/src/glsl/test/texture.vert.glsl) | 43 |
| [shaders/src/glsl/test/texture_gi.frag.glsl](../../shaders/src/glsl/test/texture_gi.frag.glsl) | 40 |
| [shaders/src/glsl/test/texture_gi.vert.glsl](../../shaders/src/glsl/test/texture_gi.vert.glsl) | 47 |
| [shaders/src/glsl/test/vertbuffer.vert.glsl](../../shaders/src/glsl/test/vertbuffer.vert.glsl) | 30 |
| [shaders/src/glsl/uv_map_preview.frag.glsl](../../shaders/src/glsl/uv_map_preview.frag.glsl) | 39 |
| [shaders/src/glsl/uv_map_preview.vert.glsl](../../shaders/src/glsl/uv_map_preview.vert.glsl) | 19 |
| [shaders/src/glsl/uv_surface.frag.glsl](../../shaders/src/glsl/uv_surface.frag.glsl) | 21 |
| [shaders/src/glsl/uv_surface.vert.glsl](../../shaders/src/glsl/uv_surface.vert.glsl) | 32 |
| [shaders/src/metal/compile_test.metal](../../shaders/src/metal/compile_test.metal) | 17 |

Additional experimental backend shader examples exist in example-local directories. Their presence is static implementation evidence, not proof of runtime parity.

| Example | Why relevant |
|---|---|
| [examples/gpu/03_offscreen_device](../../examples/gpu/03_offscreen_device) | Windowless primitive context. |
| [examples/gpu/06_offscreen_triangle](../../examples/gpu/06_offscreen_triangle) | Offscreen graphics/readback foundation. |
| [examples/gpu/10_cubemap_render_pass](../../examples/gpu/10_cubemap_render_pass) | Vulkan/Metal conversion, irradiance convolution and specular prefilter shaders. |
| [examples/gpu/11_ray_query_shadows](../../examples/gpu/11_ray_query_shadows) | Primitive query shadow shaders on both backends. |
| [examples/gpu/12_deferred_cleanup](../../examples/gpu/12_deferred_cleanup) | Completion/lifetime primitive exercise. |
| [examples/gpu/13_ray_tracing_pipeline](../../examples/gpu/13_ray_tracing_pipeline) | Trace pipeline with Vulkan stages and Metal raygen kernel. |
| [examples/draw/01_window_ui](../../examples/draw/01_window_ui) | Managed experimental lifecycle and retained scene/UI. |
| [examples/draw/03_uniform_buffers](../../examples/draw/03_uniform_buffers) | Per-slot buffers/resource sets and delegate-owned depth. |
| [examples/draw/04_render_to_texture](../../examples/draw/04_render_to_texture) | Intermediate graphics/compute and final sampling. |
| [examples/draw/05_cubemap](../../examples/draw/05_cubemap) | Cubemap drawing within managed execution. |
| [examples/draw/07_ray_query_shadows](../../examples/draw/07_ray_query_shadows) | Queries inside managed draw execution. |
| [examples/draw/08_deferred_cleanup](../../examples/draw/08_deferred_cleanup) | Retirement within application execution. |
| [examples/draw/09_ray_tracing_pipeline](../../examples/draw/09_ray_tracing_pipeline) | Trace output within managed draw execution. |
| [examples/17_deferred_renderer](../../examples/17_deferred_renderer) | Production deferred reference. |
| [examples/15_offscreen_render_cli](../../examples/15_offscreen_render_cli) | Production low-level offscreen reference. |
| [examples/16_offscreen_scene_render_cli](../../examples/16_offscreen_scene_render_cli) | Production scene offscreen reference. |

## 8. Targeted evidence for major findings

| Finding | Main checked source |
|---|---|
| Drawable owns production engine/materials/mesh/BLAS and native draw callbacks | [lib/include/bg2e/scene/Drawable.hpp](../../lib/include/bg2e/scene/Drawable.hpp) |
| Loaded clone rebuilds GPU realization on the same production engine | [lib/src/bg2e/scene/Drawable.cpp](../../lib/src/bg2e/scene/Drawable.cpp) |
| Factory creators require a production engine | [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp) |
| Current camera G-buffer contains six color attachments plus depth | [lib/src/bg2e/render/gbuffer/GBufferManager.cpp](../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp) |
| Deferred chain, jitter, output, overlays and FSR availability behavior | [lib/src/bg2e/render/RendererDeferred.cpp](../../lib/src/bg2e/render/RendererDeferred.cpp) |
| Transparent depth copy and optional indirect/reflection pass orchestration | [lib/src/bg2e/render/deferred/DeferredLayer.cpp](../../lib/src/bg2e/render/deferred/DeferredLayer.cpp) |
| Primitive graphics description has one color format and no configurable blend/depth state | [lib/include/bg2e/gpu/GraphicsPipeline.hpp](../../lib/include/bg2e/gpu/GraphicsPipeline.hpp) |
| Metal simple depth attachments use DontCare store behavior | [lib/src/bg2e/gpu/metal/CommandBuffer.cpp](../../lib/src/bg2e/gpu/metal/CommandBuffer.cpp) |
| Primitive formats/stages/resource binding count limitations | [lib/include/bg2e/gpu/Common.hpp](../../lib/include/bg2e/gpu/Common.hpp) |
| Vulkan ResourceSet setters write one descriptor with no public array element parameter | [lib/src/bg2e/gpu/vk/ResourceSet.cpp](../../lib/src/bg2e/gpu/vk/ResourceSet.cpp) |
| RT pipeline description has no any-hit shader | [lib/include/bg2e/gpu/RayTracingPipeline.hpp](../../lib/include/bg2e/gpu/RayTracingPipeline.hpp) |
| Experimental Vulkan BLAS geometry is opaque | [lib/src/bg2e/gpu/vk/RayTracingMesh.cpp](../../lib/src/bg2e/gpu/vk/RayTracingMesh.cpp) |
| Metal uses raygen compute and ignores separate miss/closest-hit modules | [lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp](../../lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp) |
| Production alpha-cutout query and instance material lookup | [shaders/src/glsl/lib/rt_material_data.glsl](../../shaders/src/glsl/lib/rt_material_data.glsl) |
| Production any-hit alpha rejection | [shaders/src/glsl/rt_alpha_test.rahit.glsl](../../shaders/src/glsl/rt_alpha_test.rahit.glsl) |
| FSR Vulkan upscaler dispatch, missing reactive input, reset flag and resource states | [lib/src/bg2e/render/deferred/FSRPostProcessor.cpp](../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp) |
| Draw Engine currently requires an SDL window | [lib/src/bg2e/draw/Engine.cpp](../../lib/src/bg2e/draw/Engine.cpp) |
| Retained color usage, dirty scene and presentation composition | [lib/src/bg2e/draw/RenderLoop.cpp](../../lib/src/bg2e/draw/RenderLoop.cpp) |
| Resource retirement captures submitted-work completion rather than logical frames | [lib/include/bg2e/gpu/CleanupManager.hpp](../../lib/include/bg2e/gpu/CleanupManager.hpp) |
| Texture cache key lacks device identity | [lib/src/bg2e/utils/TextureCache.cpp](../../lib/src/bg2e/utils/TextureCache.cpp) |
| Integrated baking matching-frame/preparation validation and isolated resources | [lib/src/bg2e/render/IntegratedBakerContext.cpp](../../lib/src/bg2e/render/IntegratedBakerContext.cpp) |
| Standalone baking state and generation management | [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../lib/src/bg2e/render/StandaloneBakerContext.cpp) |
| Writer currently consumes production LightmapPixels | [lib/include/bg2e/db/LightmapOutputWriter.hpp](../../lib/include/bg2e/db/LightmapOutputWriter.hpp) |

## 9. What was not verified

No compilation, CMake regeneration, execution, GPU capture, device capability probe, automated C++ test, image comparison or packaging run was performed. SDK availability and device-specific limits were not established. Existing historical planning documents were consulted only as context; current code wins where layouts or feature sets differ. The repository references `skills.md`, but that file was not found in the repository. Only documentation files were created.
