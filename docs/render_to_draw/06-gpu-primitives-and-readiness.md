# GPU primitives, experimental readiness, and draw responsibilities

[Overview](index.md) · [Architecture](05-target-draw-architecture.md) · [RT details](08-ray-tracing-and-fallback.md)

## 1. Low-level scope is the deciding criterion

`gpu` is a very low-level framework, comparable in responsibility to Vulkan. Missing high-level engine objects are **not GPU gaps**. It is appropriate for GPU to express multiple attachments, arbitrary shader resources, pipeline depth/blend state and non-opaque geometry. It is inappropriate for GPU to know what a G-buffer channel means, which scene lights affect reflections, or how a material is imported.

| Requirement | `gpu` mechanism | `draw` policy/resource |
|---|---|---|
| Deferred G-buffer | Multiple color/depth attachments and pipeline formats. | Channel schema, resource ownership, packing, resize and pass order. |
| Material textures | Images, samplers, resource sets and buffer bindings. | Material realization, defaults, caches, channel/UV semantics. |
| RT scene | BLAS/TLAS, instance transforms/IDs, tracing commands. | Scene filtering, material/texture tables, change tracking and effect participation. |
| Histories | Images/buffers, dependencies and lifetime primitives. | Reprojection, sample validity, resets and per-view storage. |
| IBL | Cubemaps, mip/face targeting, shaders and commands. | Convolution/prefilter/LUT algorithms and environment selection. |
| Offscreen | Windowless surfaces/device execution/readback. | Camera/render configuration, batch flow and export. |
| FSR | Resource/native interop and synchronization. | SDK integration, quality modes, jitter, context resources and fallback. |
| Picking | Render targets, integer or encoded output and readback. | Stable IDs, scene mapping, selection/tool behavior. |

## 2. Existing foundations

**Observed.** The current public API already has Device/Backend/Factory, Instance/PhysicalDevice, window and offscreen Surface, SurfaceFrame, Queue, CommandBuffer, Buffer/Image/Sampler, ResourceSet/PipelineLayout, graphics/compute/RT pipelines, ShaderLib, MeshGeneric, CubeMap/CubemapRenderPass, RayTracingMesh/Scene, FrameResourceRing and CleanupManager. Vulkan and Metal implementation trees exist for these families.

Relevant evidence:

- [`Device.hpp`](../../lib/include/bg2e/gpu/Device.hpp) exposes resource factories and immediate submission.
- [`CommandBuffer.hpp`](../../lib/include/bg2e/gpu/CommandBuffer.hpp) exposes indexed graphics, compute, image transitions/copy/blit, acceleration builds and trace dispatch.
- [`PhysicalDevice.hpp`](../../lib/include/bg2e/gpu/PhysicalDevice.hpp) exposes RT capabilities.
- [`examples/gpu`](../../examples/gpu/) demonstrates windowless rendering, image processing, IBL and tracing primitives.
- [`examples/draw`](../../examples/draw/) integrates those primitives with managed application/UI execution.

This is substantial reuse. It does not establish that every effect/resource configuration needed by production is supported or tested. Some base virtual methods throw by default, and certain backend semantics are intentionally simplified.

## 3. Blocking primitive gap: multiple render targets

[`GraphicsPipelineDescription`](../../lib/include/bg2e/gpu/GraphicsPipeline.hpp) contains one `colorFormat`. Public beginRendering overloads accept one color image or a surface frame, optionally depth. Production G-buffer needs six simultaneously written color attachments plus depth.

**Required primitive extension:** a rendering target/attachment description with an array of color attachments, optional depth/stencil, per-attachment format/view/subresource and load/store/clear behavior. Graphics pipeline configuration must declare matching color formats and states. Implement in both backend command/pipeline paths.

**Draw-owned counterpart:** a `GBuffer` chooses the six semantic resources and a `GBufferPass` records them. Do not add a `createDeferredGBuffer()` factory to Device or a scene-aware `beginDeferredRendering()` method to CommandBuffer.

Portable MRT count and attachment format support should be queried/validated. The API should describe primitives even when a particular device cannot satisfy the requested renderer configuration.

## 4. Blocking primitive gap: depth and blend state

Experimental Vulkan graphics creation currently configures one attachment with blending disabled. With a depth format it enables testing and writes using `LESS_OR_EQUAL`. Metal similarly uses a simplified fixed graphics/depth state. The public description cannot express independent depth-test/write/compare choices or attachment blending.

Production needs writable opaque depth, non-writing transparent depth, fullscreen composition, wireframe/highlight/gizmo blending and depth-tested/depth-disabled overlay variants. Some native configuration details differ by pass.

**Required primitive extension:** generic depth/stencil and per-attachment blend/write-mask state. Add viewport/scissor/subregion controls where graphics tools need them. The renderer/layer chooses those states; GPU does not have a “transparent material” or “selection highlight” switch.

Do not infer that disabling depth writes alone provides correct transparent composition. Attachment load/store and copied opaque depth must also be preserved.

## 5. Primitive formats and readback

[`PixelFormat`](../../lib/include/bg2e/gpu/Common.hpp) currently includes RGBA8/BGRA8, RGBA16F/RGBA32F and several depth/stencil formats. Production uses additional formats:

| Format family | Existing production use |
|---|---|
| `R8_UNORM` | Raw AO, neutral AO and SMAA lookup data. |
| `R8G8_UNORM` | SMAA edge/area resources. |
| `R16_SFLOAT` | Scalar temporal history. |
| `R16G16_SFLOAT` | Motion vectors/FSR data. |
| `R32_UINT` | UV surface identifiers and FSR reconstruction resources. |
| `R32_SFLOAT` | FSR depth resources and specialized readback/views. |

These are legitimate low-level format additions with backend mappings, usage capability checks and pixel-size/conversion handling. Wider formats can sometimes substitute, but blindly changing shader storage qualifiers or SDK resource formats is not equivalent. Bake validation explicitly checks some formats.

`Image::readPixelsRGBA8` supports an important baseline. Production baking and viewpoint analysis also need HDR float data and depth/ID readback. Extend generic typed/raw/subregion readback or explicit primitive image-to-buffer copy; keep scoring, color encoding and file writing outside GPU. A separate render-to-RGBA8 conversion pass is useful but cannot preserve HDR lightmap export on its own.

## 6. Resource binding expressiveness

`gpu::ResourceType` exposes separate sampled images and samplers. Production GLSL commonly uses combined image samplers. That is an interface-porting issue: define matching Vulkan/Metal binding schemas rather than assuming production descriptor layouts can be reused byte-for-byte.

`ResourceBinding::count` exists, and Vulkan layouts/pools account for counts, but current ResourceSet setters accept one object with no array element parameter. Vulkan writes set `descriptorCount = 1`; the public API does not expose production's per-object texture/buffer-array assignment.

**Required primitive decision:** add resource-array assignment/indexing and associated device limits, or choose another generic resource-addressing primitive such as packed buffers and texture arrays/atlases. Production RT currently uses arrays indexed by instance custom ID, so this is a blocker for a faithful direct port.

**Draw responsibility:** populate material/geometry tables, choose capacity and growth, supply fallbacks, keep the instance-ID mapping stable for each submitted snapshot. Do not make GPU ResourceSet accept scene materials.

Multi-stage visibility also needs review: `ResourceBinding::stage` is currently a single stage. Production bindings can be visible to fragment, compute and multiple RT stages. Use a stage-mask/merged binding contract or carefully coordinated descriptions; the primitive must reject ambiguous/incompatible layouts.

## 7. RT primitive gaps

GPU already provides BLAS/TLAS and trace dispatch, but full cutout parity needs more:

- `ShaderStage` and `RayTracingPipelineDescription` expose raygen/miss/closest-hit, with no any-hit stage/shader.
- Experimental Vulkan RayTracingMesh sets `VK_GEOMETRY_OPAQUE_BIT_KHR` during description/build; there is no public non-opaque geometry flag.
- The pipeline description represents one raygen/miss/hit family rather than a generalized set of hit groups.
- Metal implements the raygen stage as a compute kernel and intentionally ignores miss/closest-hit modules; equivalent hit/miss/cutout behavior must be implemented in that kernel or a future primitive intersection contract.

Non-opaque geometry flags and additional shader/group configuration belong in GPU. Alpha-cutoff values, UV/material lookup, scene filtering, light participation and effect sampling remain in `draw`.

The current common RT capability booleans include Vulkan-flavored names such as buffer device address; Metal maps them to its available path. That is a useful existing convention, but effect readiness should not assume Vulkan semantics from those flags. Distinguish device primitive availability from renderer shader/material/SDK support.

## 8. Image operations and synchronization

GPU Vulkan image transitions currently track whole-image layouts and use broad all-commands memory dependencies. This supplies a conservative basis for ordered work; it is not a renderer graph. More precise subresource/stage/access control can be added as generic primitives when a pass requires it.

Depth copy must be checked against image usages and backend implementation. The Vulkan depth-image helper's default usage is sampled/depth attachment; renderer-owned copied depth must explicitly support transfers. Metal beginRendering currently uses depth `DontCare` store behavior in the inspected simple paths, which is insufficient for later lighting/RT reconstruction or transparent depth reuse without a primitive attachment-policy extension.

The public `blitImage` is marked not fully tested. Metal's inspected implementation issues a texture copy rather than a scaling operation. Do not depend on it for render-scale conversion; use a fullscreen sampling pass in `draw` until the generic primitive defines and implements scaling/format-conversion semantics.

FSR native commands need explicit state reconciliation. Backend unwraps should stay private and checked; do not expose native Vulkan handles on every high-level object merely to make one SDK convenient.

The generic image beginRendering overloads expose a mip-level parameter, but the inspected Vulkan and Metal implementations of the ordinary 2D paths ignore it; the dedicated cubemap paths do handle face/mip targeting. If ordinary 2D mip rendering is selected for texture generation, that is another primitive behavior to complete. It does not require placing texture-generation policy in GPU.

## 9. Existing capabilities that belong above GPU

These are **draw work, not missing GPU primitives**:

- G-buffer semantic layout and layer chain.
- CPU scene extraction and opaque/transparent classification.
- Texture/material/default-resource caches and edit revisions.
- IBL generation service and environment switching.
- View/object/light/environment shader-data packing.
- RTAO, RTGI, reflection and shadow algorithms.
- Temporal reprojection, denoise, history reset and debug views.
- FSR mode selection/context management and final-output fallback.
- Picking ID mapping, selection overlays and transform-gizmo graphics.
- UV previews, viewpoint scoring orchestration and baking services.

The existence of a generic graphics helper in GPU, such as CubemapRenderPass, does not justify moving IBL policy or scene resources into GPU.

## 10. Readiness priorities

| Priority | Primitive work | Why it comes first |
|---|---|---|
| Baseline deferred | MRT, explicit attachments, depth/blend state, format/usage validation. | Required even without any RT effects. |
| Resource correctness | Resource arrays/multi-stage binding decisions, readback, depth copy/store, lifetime/state checks. | Needed by tools, tracing tables and offscreen parity. |
| RT parity | Non-opaque geometry, any-hit/equivalent intersection path, binding/addressing limits. | Prevents cutout/shadow/GI/reflection regressions. |
| Optional optimization | Precise barriers, subresource transitions, improved blits, more flexible hit groups. | Add when required by chosen algorithms, not as an unlimited GPU redesign. |

Every proposed extension should be demonstrated with a primitive example independent of `scene` or `draw` renderer classes. No new GPU functionality was implemented or compiled for this assessment.
