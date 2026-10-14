# Production inventory and extraction candidates

[Overview](index.md) · [Dependency map](02-dependency-map.md) · [Target architecture](05-target-draw-architecture.md)

## 1. The actual production framework

**Observed.** The current implementation includes both `RendererBasicForward` and `RendererDeferred`, a layered deferred pipeline, RT effects, FSR upscaling, selection/gizmo integration, offscreen execution, and recently expanded lightmap tooling. The architecture description in `AGENTS.md` emphasizes the forward path, but it is not a complete inventory of the current production tree.

The namespace contains several distinct responsibilities:

| Responsibility | Representative objects | Assessment |
|---|---|---|
| Graphics API wrapper | `vulkan::Device`, `Image`, `Buffer`, factories, descriptor allocation | Replace with existing/extended `gpu` primitives; do not copy into `draw`. |
| Execution and global resource lifetime | `Engine`, `RenderLoop`, `FrameResources`, cleanup | Extend experimental execution; separate surface coordination from scene resources. |
| GPU representations of engine data | `Texture`, `MaterialBase`, `EnvironmentResources` | Reimplement as high-level `draw` resources over `gpu`. |
| Rendering policy | `Renderer`, queue classification, layers, deferred composition | Preserve concepts with new contracts and separation from graph ownership. |
| Effect algorithms | AO, GI, reflections, temporal processing, denoise, FSR, SMAA | Preserve functionality; isolate inputs, settings, histories, and backend adapters. |
| Graphics tools and batch services | Baking, UV preview, viewpoint analysis | Retain capabilities in `draw` services; app/DB policy should stay outside core renderer. |

This distinction matters for migration estimates. A graphics factory and a lightmap baker can both be called rendering objects today, but their destination layers are different.

## 2. Execution family

### Engine

[`render::Engine`](../../lib/include/bg2e/render/Engine.hpp) owns the production Vulkan physical/logical devices, surface, swapchain, commands, VMA allocator, descriptor allocator, frame resources, cleanup manager, and deferred closures. It exposes native Vulkan-facing objects and a two-slot `numImages()` result. Its instance wrapper already uses `gpu::vk::Instance`; this is a small shared primitive, not proof that the production engine is backend-agnostic.

[`draw::Engine`](../../lib/include/bg2e/draw/Engine.hpp) already owns the experimental context via `gpu::Backend`, `Device`, and `WindowSurface`. It uses a PImpl and explicit ownership/lifecycle checks. Its windowless path is missing. A new engine should not be a second wrapper around `render::Engine`.

**Proposed split:** context/device/surface lifecycle in `draw::Engine`; persistent high-level resource services associated with that device; renderer resources owned by each renderer; frame acquisition/submission in execution; device-native implementation detail in `gpu` or a restricted integration adapter.

### Loop and delegates

[`RenderLoop`](../../lib/include/bg2e/render/RenderLoop.hpp), [`RenderLoopDelegate`](../../lib/include/bg2e/render/RenderLoopDelegate.hpp), [`DefaultRenderLoopDelegate`](../../lib/include/bg2e/render/DefaultRenderLoopDelegate.hpp), and [`DefaultOffscreenApplicationDelegate`](../../lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp) bind engine execution to renderer lifecycle. Their signatures contain native commands, image layouts, frame descriptor allocators, and production images.

The experimental loop and delegate already cover managed window execution, resize, update/render, pause/resume, dirty retained scenes, and UI composition. Port the default scene-renderer delegate as a convenience adapter using experimental contexts. Do not reintroduce the central production `initFrameResources(DescriptorSetAllocator*)` callback: experimental objects currently own persistent resources indexed by frame slot.

### Settings preferences

[`RenderSettingsPreferences`](../../lib/include/bg2e/render/RenderSettingsPreferences.hpp) is coupled to a concrete deferred renderer and `app::Preferences`. It combines persistence with renderer mutation. Its saved final-rendering values are **indices into dynamically available options**, not `FinalRenderMode` values.

Preserve behavior through a compatibility adapter if required, but define experimental settings as value objects and stable serialized identifiers. Persistence belongs to an application-facing adapter. A renderer should not need a preference store to function headlessly.

## 3. General high-level graphics objects

| Existing object | What it actually does | Suggested experimental treatment |
|---|---|---|
| [`Texture`](../../lib/include/bg2e/render/Texture.hpp) | Combines `base::Texture`, Vulkan image, sampler, color interpretation, default textures, loading, ownership. | `draw::Texture`/texture resource backed by `gpu::Image` and `gpu::Sampler`; device-scoped defaults and cache. |
| [`MaterialBase`](../../lib/include/bg2e/render/MaterialBase.hpp) | Editable attributes plus six loaded texture resources and optional global cache use. | `draw::MaterialResource`; keep editable values in `base::MaterialAttributes`; resource binding separate from renderer shader packing. |
| [`RenderQueue`](../../lib/include/bg2e/render/RenderQueue.hpp) | Holds GPU mesh/material/world transform; classifies opaque, transparent, solid-transparent; sorts and issues Vulkan drawing. | Separate `draw::DrawList` construction/sorting from pass command recording. |
| [`ColorAttachments`](../../lib/include/bg2e/render/ColorAttachments.hpp) | Owns an arbitrary array of renderable images and formats. | `draw::RenderTargets` owner over primitive GPU attachments. |
| [`ColorAttachmentsCanvas`](../../lib/include/bg2e/render/ColorAttachmentsCanvas.hpp) | Fullscreen graphics pipeline consuming attachment images and a custom fragment shader. | Fullscreen pass/service in `draw`; shader bindings explicit and backend-aware. |
| [`GPUProcess`](../../lib/include/bg2e/render/GPUProcess.hpp) | Binds images/buffers with native descriptor types and executes a compute shader. | Small `draw` compute-pass helper over `gpu`; preserve arbitrary shader processing without native types in public API. |
| [`GPUTextureGenerator`](../../lib/include/bg2e/render/GPUTextureGenerator.hpp) | Abstract generator using an engine and wrapping generated Vulkan images. | Keep generator concept in `draw`; distinguish command-recorded generation from synchronous convenience execution. |
| [`BlueNoise`](../../lib/include/bg2e/render/BlueNoise.hpp) | Shared texture used by multiple RT effects. | Device-scoped immutable sampling resource in `draw`; reuse source data. |

These are graphics-engine building blocks worth having independently of the concrete deferred renderer. They should not require instantiating that renderer or traversing a scene merely to create a texture.

## 4. Renderer and deferred families

[`Renderer`](../../lib/include/bg2e/render/Renderer.hpp) is virtual, but its contract includes Vulkan formats, commands, images, layouts, sample counts, scene ownership, environment resources, update/resize visitors, and a production queue. Preserve **virtual extensibility**, not this entire interface.

`RendererBasicForward` is excluded from experimental implementation. Existing forward applications continue using production. Forward-related general facilities such as texture loading, IBL, geometry, skybox generation, and selection remain in scope wherever they are renderer-independent.

[`RendererDeferred`](../../lib/include/bg2e/render/RendererDeferred.hpp) orchestrates skybox, opaque and transparent layers, shared lights/RT data, intermediate images, final processing, motion vectors, blue noise, editor overlays, settings forwarding, and previous camera matrices. It currently duplicates a large settings accessor surface across renderer and layers. A cleaner experimental renderer can expose grouped settings while compatibility adapters keep older accessors where useful.

| Production object | Preserve | Refactoring direction |
|---|---|---|
| `deferred::RenderLayer` | Layer lifecycle and composition extension point. | Explicit inputs/outputs and view data; avoid every layer reading a mutable `scene::Scene`. |
| `deferred::SkyboxLayer` | Background/skybox stage. | Consume environment resources and camera/view values. |
| `deferred::DeferredLayer` | G-buffer, lighting, effects, debug, transparent composition. | Split orchestration from G-buffer, lighting and effect passes. |
| `GBufferManager` | Six color resources plus depth, format checks and transitions. | `draw::GBuffer` semantic owner; primitive attachment creation/recording in `gpu`. |
| `RTAmbientOcclusion` | Ray-query compute AO, UV-space path, quality/settings. | Independent screen-space and bake inputs with shared kernels where possible. |
| `RTGlobalIllumination` | RT pipelines, scene/material data, quality, UV-space path. | Shared tracing/material scene service with per-effect resource ownership. |
| `RTReflections` | RT radiance/mask, roughness gating, quality and history. | Explicit display/internal resolution relationship and effect status. |
| `TemporalAccumulator` | Scalar and HDR accumulation with depth/normal rejection. | History instances per effect, layer and view; separate scene-frame sequence from presentation sequence. |
| `DenoiseFilter` | Bilateral filtering and HDR variant. | Configurable pass preserving signal semantics and shader layout. |
| `MotionVectorGenerator` | Depth/camera-based reprojection vectors. | Preserve baseline; moving-object velocity is an optional improvement. |
| `FinalPostProcessor` | Virtual final-output stage. | Backend-independent inputs/status; native SDK code behind private adapters. |
| `DirectPostProcessor` | Native direct output. | Required universal fallback. |
| `FSRPostProcessor` | FSR3 upscaler via Vulkan FidelityFX interface. | Optional Vulkan integration; do not claim Metal support or frame generation. |
| `SMAAProcessor`, `SMAAPostProcessor` | Implemented spatial-AA processing family. | Preserve as available capability; it is not currently a `FinalRenderMode` option. |

See [pipeline/parity](07-deferred-pipeline-and-parity.md) and [RT analysis](08-ray-tracing-and-fallback.md) for behavior, rather than relying on class names alone.

## 5. Environment family

`CubemapRenderer`, `SphereToCubemapRenderer`, `IrradianceCubemapRenderer`, `SpecularReflectionCubemapRenderer`, `SkyboxRenderer`, `BRDFIntegrationMapTextureGenerator`, and `EnvironmentResources` cover equirectangular conversion, diffuse convolution, roughness-prefiltered reflections, BRDF LUT generation, and sky rendering.

These names include “Renderer,” but most are specialized image-generation passes, not scene-renderer implementations. They remain necessary even though the only scene renderer will be deferred. Split `EnvironmentResources` into generated IBL resources, generation/update logic, and sky drawing. Reuse `gpu::CubeMap` and `CubemapRenderPass` for API mechanics. Experimental IBL example code is a useful reference, not already a reusable production-equivalent service.

## 6. Graphics tools and baking family

Keep these in the scope implied by “all render features except forward”:

| Family | Production objects | Destination suggestion |
|---|---|---|
| UV preview | `UvMapPreviewRenderer` | `draw` graphics tool over CPU mesh; UI wrapper remains `ui`. |
| Automatic viewpoint selection | `ViewpointAnalyzer`, configuration/candidate/result records | GPU visibility measurement in `draw`; camera-selection policy in utility/app layer. |
| Baking resources/execution | `BakerContext`, `IntegratedBakerContext`, `StandaloneBakerContext`, `LightmapBaker` | Optional `draw::baking` service with clean frame/offscreen context. |
| Baking configuration | `LightmapSettings`, `LightmapPixels` | Backend-neutral values; accessible to DB output writers without renderer dependency. |
| Batch/assembly | `StandaloneBakeBatch`, `StandaloneBakeSceneAssembler` | Application/DB orchestration; GPU baking underneath `draw`. |
| Private baking passes | `LightmapBakeExecutor`, `UvSurfacePass`, `UvTemporalAccumulator`, `LightmapCompositionPass` | Private `draw::baking` pass/resource layer. |

Production also exposes renderer uniforms in `render::uniforms`. Material packing is a shader ABI concern. Keep renderer-specific packed representations private in `draw` and preserve neutral material/light values separately.

## 7. Vulkan subnamespace: do not move it wholesale

The production Vulkan headers contain:

- Device/surface/swapchain, commands, buffers/images, physical-device inspection, extensions and Vulkan helpers.
- Descriptor allocation, descriptor-set wrappers, pipeline data binding, frame resources and cleanup.
- Graphics/compute/ray-tracing pipeline, shader module, sampler, descriptor-layout and pipeline-layout factories.
- Vulkan geometry/vertex descriptions.
- BLAS/TLAS, tracing helpers, instance collection, RT material and light data bindings.

Most primitive responsibilities already have counterparts in `gpu`. However, `vulkan::rt::CollectRayTracingInstancesVisitor`, `RTMaterialDataBinding`, `RTObjectInstance`, and `ReflectionLightDataBinding` are **not purely low-level** despite their location. They know about scene traversal, materials, texture resources and light policies. Move their high-level counterparts into `draw`; leave acceleration-structure creation and tracing execution in `gpu`.

Likewise, do not copy `FrameResources` as a central owner of both command synchronization and a scene TLAS. The new framework already assigns frame lifecycle to `gpu::Surface` and high-level resource ownership to the objects using them. A tracing scene is a renderer/view/bake service, not an obligatory member of every frame record.

## 8. Reuse versus movement

“Move” in this assessment means transfer responsibility in the eventual architecture. During coexistence, it often means an **independent implementation using the old algorithm as reference**, not physically deleting the production source file.

Reuse candidates are material semantics, CPU geometry/math, shader algorithms, noise data, settings definitions and validation rules. Reimplement native resource and descriptor plumbing. Split policy from mechanism where imports, preferences, scene traversal, or UI have become embedded in resource classes.

The [source appendix](13-source-evidence.md) lists every scoped file so less prominent helpers and umbrella headers are accounted for even when no dedicated new public class is suggested.
