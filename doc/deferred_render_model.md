# bg2e Deferred Renderer — Technical Reference

This document describes the deferred rendering pipeline implemented by
`bg2e::render::RendererDeferred` and its subsystems: render layers, G-buffer
layout, lighting composition, ray-traced effects (shadows, ambient occlusion,
global illumination, reflections), temporal accumulation, denoising, motion
vector generation and the final post-processing / upscaling stage (SMAA or
FSR 3.1).

All paths are relative to the repository root. Line references use the
`file:line` convention.

---

## Table of contents

1. [High-level architecture](#1-high-level-architecture)
2. [Render scale and resolution management](#2-render-scale-and-resolution-management)
3. [Per-frame execution sequence](#3-per-frame-execution-sequence)
4. [Render layers](#4-render-layers)
   - 4.1 [RenderLayer base class](#41-renderlayer-base-class)
   - 4.2 [SkyboxLayer](#42-skyboxlayer)
   - 4.3 [DeferredLayer (opaque and transparent)](#43-deferredlayer-opaque-and-transparent)
5. [G-buffer](#5-g-buffer)
   - 5.1 [Attachment layout](#51-attachment-layout)
   - 5.2 [Image creation and layout transitions](#52-image-creation-and-layout-transitions)
   - 5.3 [G-buffer fill pass](#53-g-buffer-fill-pass)
   - 5.4 [Transparent layer depth handling](#54-transparent-layer-depth-handling)
6. [Lighting composition pass](#6-lighting-composition-pass)
   - 6.1 [Pipelines and descriptor layout](#61-pipelines-and-descriptor-layout)
   - 6.2 [Non-RT composite shader](#62-non-rt-composite-shader)
   - 6.3 [RT composite shader](#63-rt-composite-shader)
   - 6.4 [Layer blending and refraction](#64-layer-blending-and-refraction)
7. [Ray-traced shadows](#7-ray-traced-shadows)
8. [Ray-traced ambient occlusion (RTAO)](#8-ray-traced-ambient-occlusion-rtao)
9. [Ray-traced global illumination (RTGI)](#9-ray-traced-global-illumination-rtgi)
10. [Ray-traced reflections](#10-ray-traced-reflections)
11. [Temporal accumulation](#11-temporal-accumulation)
12. [Denoising (bilateral filter)](#12-denoising-bilateral-filter)
13. [Motion vector generation](#13-motion-vector-generation)
14. [Final post-processing](#14-final-post-processing)
    - 14.1 [SMAA path](#141-smaa-path)
    - 14.2 [FSR 3.1 path](#142-fsr-31-path)
    - 14.3 [Path selection and scale options](#143-path-selection-and-scale-options)
15. [Debug visualization](#15-debug-visualization)
16. [Ray tracing data bindings](#16-ray-tracing-data-bindings)
17. [Known limitations and implementation notes](#17-known-limitations-and-implementation-notes)
18. [File and shader reference index](#18-file-and-shader-reference-index)

---

## 1. High-level architecture

The deferred renderer is implemented by `bg2e::render::RendererDeferred`
(`lib/include/bg2e/render/RendererDeferred.hpp:44`,
`lib/src/bg2e/render/RendererDeferred.cpp`). It derives from the abstract
`bg2e::render::Renderer` base class and replaces the forward renderer
(`RendererBasicForward`) when deferred mode is selected.

Key characteristics:

- **Vulkan dynamic rendering** — no `VkRenderPass` objects; every pass uses
  `vkCmdBeginRendering`/`vkCmdEndRendering`.
- **No MSAA** — `supportsMsaa()` returns `false`
  (`RendererDeferred.hpp:96`) and the swapchain sample count is ignored
  (`RendererDeferred.cpp:133-136`). Anti-aliasing is deferred to the final
  post-processor (SMAA or FSR 3.1).
- **Layered composition** — the frame is produced by three stacked layers
  (`RendererDeferred.hpp:222-224`):

  | Order | Layer | Class | Output image |
  |---|---|---|---|
  | 1 | Skybox | `deferred::SkyboxLayer` | `_skyboxImage` |
  | 2 | Opaque geometry | `deferred::DeferredLayer` (`LayerType::Opaque`) | `_opaqueImage` |
  | 3 | Transparent geometry | `deferred::DeferredLayer` (`LayerType::Transparent`) | `_transparentImage` |

  Each layer receives the previous layer's image as input and composites
  over it. The three intermediate images are allocated with usage
  `COLOR_ATTACHMENT | TRANSFER_SRC | TRANSFER_DST | SAMPLED`
  (`RendererDeferred.cpp:193-240`). The transparent layer must render into
  its own image (not directly into the swapchain) because the final
  post-processor reads that image and writes into the destination
  (`RendererDeferred.cpp:223-226`).

- **Per-frame G-buffers** — each `DeferredLayer` owns one `GBufferManager`
  per frame-in-flight (`DeferredLayer.cpp:93-98`).
- **Optional ray tracing** — RT shadows, RTAO/RTGI and RT reflections are
  compiled in only when `_engine->rayTracingSupported()` is true
  (`RendererDeferred.cpp:160-163`, `DeferredLayer.cpp:105-147`).
- **Final upscaling / AA** — an abstract `deferred::FinalPostProcessor`
  (`lib/include/bg2e/render/deferred/FinalPostProcessor.hpp:40`) converts the
  render-resolution image into the display-resolution image. Two
  implementations exist: `SMAAPostProcessor` (all platforms) and
  `FSRPostProcessor` (Windows/Linux only).

---

## 2. Render scale and resolution management

The renderer decouples the **viewport extent** (display/swapchain size) from
the **render extent** (internal deferred rendering resolution):

- `_renderScalePercent` defaults to **50 %** (`RendererDeferred.cpp:140`).
- `computeRenderExtent()` rounds `viewportExtent * percent / 100`, clamped to
  at least 1×1 (`RendererDeferred.cpp:41-48`).
- `setRenderScalePercent()` clamps the percentage to **[25, 150]**
  (`RendererDeferred.cpp:53-54`), waits for the device to be idle, resizes
  all three layers, recreates the three intermediate images, and resizes the
  motion vector generator and the final post-processor
  (`RendererDeferred.cpp:50-126`).
- `resize()` applies the same logic when the viewport size changes; if the
  computed render extent is unchanged it only updates camera viewports
  through `_resizeVisitor` (`RendererDeferred.cpp:317-399`).

The active post-processor drives the scale options exposed to the UI:
`RendererDeferred::setScaleOption()` forwards to the post-processor and then
applies `renderScalePercent()` (`RendererDeferred.cpp:964-969`). See
[§14.3](#143-path-selection-and-scale-options).

---

## 3. Per-frame execution sequence

`RendererDeferred::draw()` (`RendererDeferred.cpp:408-566`) executes the
following steps every frame:

1. **Camera capture** — view matrix from the main camera's inverted world
   matrix; original (unjittered) projection matrix (`:420-422`).
2. **Jitter computation** —
   `jitteredProj = _finalPostProcessor->prepare(origProj, _frameCounter, _renderExtent)`
   (`:426`). SMAA returns the matrix unchanged; FSR applies Halton-sequence
   sub-pixel jitter.
3. **Scene preparation** — `prepareSceneRender()` updates scene uniforms,
   environment and the ray tracing scene (`:431`). The skybox projection is
   then overridden with the jittered matrix (`:432-435`).
4. **Projection override** — the jittered matrix is pushed into both deferred
   layers via `setProjectionOverride()` (`:438-439`), so the G-buffer pass
   rasterizes with the jittered frustum.
5. **Light upload** — `_lights` (all lights, up to
   `BG2E_MAX_DEFERRED_LIGHTS = 8`) is pushed to both layers; a filtered
   `_reflectionLights` list containing only lights with
   `affectsReflections != 0` is built for the RT passes (`:442-458`).
   `updateLights()` copies each `LightComponent` into `base::LightData`,
   including `castShadows`, `sourceSize`, `shadowSamples` and
   `affectsReflections` (`:976-1001`).
6. **Color correction** — brightness/contrast/exposure forwarded to all
   three layers (`:461-463`).
7. **Layer 1: skybox** — renders into `_skyboxImage`, then transitions it to
   `SHADER_READ_ONLY_OPTIMAL` (`:466-471`).
8. **Layer 2: opaque** — renders G-buffer + lighting, compositing over the
   skybox image, into `_opaqueImage`; transitioned to
   `SHADER_READ_ONLY_OPTIMAL` (`:474-479`).
9. **Layer 3: transparent** — receives the opaque depth buffer
   (`setOpaqueDepthBuffer`) and renders into `_transparentImage`
   (`:482-484`). Projection overrides are cleared afterwards (`:487-488`).
10. **Motion vectors** — `_motionVectorGenerator->generate()` is called with
    the transparent layer's depth buffer, the inverse of the **unjittered**
    current view-projection and the previous frame's unjittered
    view-projection (`:491-496`).
11. **Final post-processing** — camera near/far and vertical FOV are
    extracted from the camera projection (`PerspectiveProjection::fov()` or
    computed from `OpticalProjection` parameters) and
    `_finalPostProcessor->process()` is invoked with the transparent image,
    depth and motion vectors; it writes the display-resolution `colorImage`
    and leaves it in `COLOR_ATTACHMENT_OPTIMAL` (`:499-526`).
12. **Gizmos (editor only)** — in non-offscreen mode the
    `GizmoAndSelectionRenderer` draws on top of the final image using the
    **original unjittered** matrices so overlays stay sharp, with a dedicated
    cleared depth attachment (`:530-552`).
13. **Epilogue** — `endSceneRender()`; the unjittered matrices are stored as
    `_prevViewMatrix`/`_prevProjMatrix` for next frame's motion vectors, and
    `_frameCounter` is incremented (`:556-565`).

---

## 4. Render layers

### 4.1 RenderLayer base class

`deferred::RenderLayer` (`lib/include/bg2e/render/deferred/RenderLayer.hpp:34`)
is a thin abstract base holding the engine pointer, extent, output format,
scene and environment pointers, color-correction state
(`_brightness = 0`, `_contrast = 1`, `_exposure = 1`) and the
`_projectionOverride` pointer used to inject the jittered projection.

### 4.2 SkyboxLayer

`deferred::SkyboxLayer` (`lib/src/bg2e/render/deferred/SkyboxLayer.cpp:39-63`)
is the first layer:

- Clears the output image to opaque black and begins dynamic rendering —
  **no depth attachment**.
- If `_drawSkybox` is set, calls
  `_environment->drawSkybox(cmd, currentFrame, frameResources)`, which draws
  the environment cubemap using the (jittered) projection last set via
  `EnvironmentResources::updateSkybox()`.
- Ignores its input image entirely (there is no previous layer).

Empty skybox pixels are later identified in the composite pass by the
G-buffer albedo alpha channel being zero (see §6.2).

### 4.3 DeferredLayer (opaque and transparent)

`deferred::DeferredLayer`
(`lib/include/bg2e/render/deferred/DeferredLayer.hpp:75`,
`lib/src/bg2e/render/deferred/DeferredLayer.cpp`) implements both geometry
layers; `LayerType` (`Opaque` / `Transparent`,
`DeferredLayer.hpp:44-47`) selects the render queue and depth state.

`build()` (`DeferredLayer.cpp:88-218`) creates:

1. One `GBufferManager` per frame-in-flight (`:93-98`).
2. `RTAmbientOcclusion` — always created (it has an internal white fallback
   when RT is unsupported) (`:101-102`).
3. AO `TemporalAccumulator` — RT only (`:105-109`).
4. `DenoiseFilter` (LDR variant, for AO) — always (`:112-113`).
5. RT only: `RTMaterialDataBinding`, `RTReflections`, and an HDR
   `TemporalAccumulator` (`R16G16B16A16_SFLOAT`) for reflections
   (`:116-129`).
6. RT only: `RTGlobalIllumination`, an HDR GI `TemporalAccumulator`, and an
   HDR `DenoiseFilter` for GI (`:132-147`).
7. Fallback images (`:149-184`):
   - `_rtGIFallbackImage`: 1×1 black `RGBA16F` (GI disabled/unavailable).
   - `_neutralAOImage`: 4×4 white `R8_UNORM` (AO = 1 when indirect passes are
     skipped for the transparent layer).
   - `_rtReflectionFallbackImage`: 1×1 black `RGBA16F` with alpha 0 — the
     composite interprets alpha 0 as "use the prefiltered envmap".
8. Data bindings: `_frameDataBinding` (vertex stage), `_fragmentFrameDataBinding`
   (fragment stage), `_objectDataBinding`, `_environmentDataBinding`
   (`:187-190`).
9. `_useRtShadows = _engine->rayTracingSupported()` (`:193`).
10. Pipelines: G-buffer, composite, composite-RT (RT only), debug blit.
11. A default sampler for G-buffer textures (`:211-217`).

`render()` (`DeferredLayer.cpp:232-444`) executes per frame:

1. Early-out if scene, render queue or main camera are missing.
2. Selects the per-frame G-buffer via
   `_engine->currentFrameResourcesIndex()` and uses the projection override
   (jittered) if present (`:253-256`).
3. **Transparent depth copy** (transparent layer only, see §5.4) (`:260-318`).
4. **G-buffer fill pass** (`:320`, §5.3).
5. **Indirect lighting** (RT only, skipped when `_isTransparent &&
   _skipIndirectLightingForTransparent`, `:325`):
   - **RTAO mode**: `_rtAmbientOcclusion->render()` → temporal accumulation
     → denoise (`:335-358`).
   - **RTGI mode** (default, `DeferredLayer.hpp:255`): fetches the TLAS and
     object instances from `frameResources.rayTracingScene`, runs
     `_rtGlobalIllumination->render()` with the environment irradiance map
     and the reflection-light list → HDR temporal accumulation → HDR denoise
     (`:359-395`).
6. **RT reflections** (independent of the indirect mode): `_rtReflections->render()`
   → HDR temporal accumulation. The accumulated output is passed to the
   composite pass (`:398-424`). Note there is **no spatial denoise** for
   reflections — only temporal accumulation.
7. **Final stage**: full composite (§6) or a debug blit (§15) depending on
   `_debugVisualization` (`:427-443`).

---

## 5. G-buffer

### 5.1 Attachment layout

`GBufferManager` (`lib/include/bg2e/render/gbuffer/GBufferManager.hpp`,
`lib/src/bg2e/render/gbuffer/GBufferManager.cpp:30-36`) allocates **5 color
attachments + 1 depth**:

| Index | Name | Format | Contents |
|---|---|---|---|
| 0 | Albedo | `VK_FORMAT_R8G8B8A8_UNORM` | RGB = linear albedo (sRGB→linear, γ 2.2, multiplied by material albedo). **A = albedo alpha; alpha 0 marks empty pixels** |
| 1 | Normal | `VK_FORMAT_R16G16B16A16_SFLOAT` | RGB = world-space normal mapped to [0,1] (`n*0.5+0.5`). **A = light emission factor** |
| 2 | Material | `VK_FORMAT_R8G8B8A8_UNORM` | R = metallic, G = roughness (min 0.05), B = baked AO, A = sheen intensity |
| 3 | Fresnel + flags | `VK_FORMAT_R8G8B8A8_UNORM` | RGB = fresnel tint. A = material flags (bit 0 = unlit) |
| 4 | Sheen / refraction | `VK_FORMAT_R8G8B8A8_UNORM` | RGB = sheen color. **A = refraction factor** |
| — | Depth | `VK_FORMAT_D32_SFLOAT` | Scene depth |

Channel packing is written by `shaders/src/glsl/deferred_gbuffer.frag.glsl:49-73`.

### 5.2 Image creation and layout transitions

- Color images: usage `COLOR_ATTACHMENT | SAMPLED | TRANSFER_SRC |
  TRANSFER_DST`, single mip, single sample (`GBufferManager.cpp:44-88`).
- Depth: usage `DEPTH_STENCIL_ATTACHMENT | SAMPLED | TRANSFER_SRC |
  TRANSFER_DST`, aspect DEPTH.
- The manager tracks current layouts and skips redundant transitions
  (`GBufferManager.cpp:229-253`):
  - `transitionToClear()` → `GENERAL`
  - `transitionToAttachment()` → `COLOR_ATTACHMENT_OPTIMAL` /
    `DEPTH_STENCIL_ATTACHMENT_OPTIMAL`
  - `transitionToShaderRead()` → `SHADER_READ_ONLY_OPTIMAL`

### 5.3 G-buffer fill pass

`beginRender(cmd, isTransparent)` (`GBufferManager.cpp:173-227`) clears each
color image to `{0,0,0,0}` via `vkCmdClearColorImage`, transitions to
attachment layout, and begins dynamic rendering with all 5 color attachments
plus depth. The depth attachment uses `LOAD_OP_CLEAR` (clear to 1.0) for the
opaque layer and `LOAD_OP_LOAD` for the transparent layer (which pre-copied
the opaque depth).

Pipeline (`DeferredLayer::createGBufferPipeline()`, `DeferredLayer.cpp:788-836`):

- Shaders: `deferred_gbuffer.vert.spv` + `deferred_gbuffer.frag.spv`.
- Vertex input layout from `scene::Drawable` (position, normal, uv0, uv1,
  tangent).
- Descriptor sets: **set 0** = `FrameDataBinding` (view/proj UBO), **set 1**
  = `ObjectDataBinding` (model matrix + `PBRMaterialData` + 6 material
  textures: albedo, normal, metallic, roughness, AO, light emission).
- Depth state: opaque → depth test `VK_COMPARE_OP_LESS`, **writes enabled**;
  transparent → **depth test fully disabled** (`enableDepthtest(false, ...)`,
  `DeferredLayer.cpp:811-818`) — transparent ordering relies entirely on
  render-queue sorting.
- Back-face culling (CCW front), no multisampling, triangle list.

Execution (`renderGBufferPass()`, `DeferredLayer.cpp:1001-1055`): binds the
pipeline, creates the scene descriptor set, then iterates the shared
`RenderQueue<scene::Drawable>`: the opaque layer renders
`RenderQueueType::Opaque`; the transparent layer renders
`RenderQueueType::Transparent` followed by `RenderQueueType::SolidTransparent`
(sorted back-to-front by camera position). Per draw call, an object
descriptor set `{sceneDS, objectDS}` is bound.

Vertex shader (`deferred_gbuffer.vert.glsl`): transforms to world space,
builds a Gram-Schmidt re-orthogonalized TBN matrix
(`lib/normal_map.glsl:19-28`) and outputs world position, world normal, both
UV sets and the TBN.

Fragment shader (`deferred_gbuffer.frag.glsl`): samples the material
textures with per-texture UV-set selection, scale, channel selection and
inversion (`lib/uniforms.glsl:95-146`) and packs the five attachments as
described in §5.1. Normal mapping is applied in world space via the TBN.

### 5.4 Transparent layer depth handling

The transparent layer never writes depth, to preserve blending. Before its
G-buffer pass, the opaque layer's depth buffer is copied into the transparent
G-buffer's depth image with `vkCmdCopyImage` on the depth aspect
(`DeferredLayer.cpp:260-318`). Because the depth test is disabled in the
transparent pipeline, this copied depth is used only by downstream consumers
(motion vectors, debug views), not for occlusion — transparent ordering comes
from render-queue sorting.

---

## 6. Lighting composition pass

### 6.1 Pipelines and descriptor layout

Two variants of the composite pass exist, both drawing a procedural
6-vertex fullscreen quad (`deferred_composite.vert.glsl:24-42`, no vertex
inputs, no depth test):

**Non-RT** (`createCompositePipeline()`, `DeferredLayer.cpp:838-893`,
shader `deferred_composite.frag.spv`):

| Set | Binding | Content |
|---|---|---|
| 0 | 0–4 | G-buffer albedo / normal / material / fresnel+flags / sheen |
| 0 | 5 | Input image (previous layer output) |
| 0 | 6 | G-buffer depth |
| 1 | 0 | `SceneData` (view/proj, fragment stage) |
| 2 | 0–3 | Environment: irradiance cubemap, prefiltered envmap, BRDF LUT, `EnvironmentData` |
| 3 | 0 | `LightBuffer` SSBO (up to 8 `LightData`) |

**RT** (`createCompositePipelineRT()`, `DeferredLayer.cpp:895-956`, shader
`deferred_composite_rt.frag.spv`): adds binding 7 = `g_Indirect` (AO or GI)
and binding 8 = `g_RTReflection` to set 0, plus **set 4 binding 0** =
`accelerationStructureEXT` TLAS via `RayTracingSceneDataBinding`.

Push constants (`CompositePushConstants`, `DeferredLayer.hpp:258-270`):
`gamma` (2.2), `brightness`, `contrast`, `exposure`, `numLights`,
`indirectMode` (0 = RTAO, 1 = RTGI), and `inverseViewProjection` computed
from the **unjittered** camera matrices (`DeferredLayer.cpp:1149-1169`).

The RT variant is selected at draw time when `_useRtShadows &&
tlas != VK_NULL_HANDLE` (`DeferredLayer.cpp:1081-1085`). The indirect image
bound at binding 7 is: `_neutralAOImage` when indirect passes are skipped,
the HDR denoised GI image when RTGI is active, or the denoised AO image
otherwise (`:1116-1134`).

### 6.2 Non-RT composite shader

`shaders/src/glsl/deferred_composite.frag.glsl:75-127`:

1. `setupDeferredGBuffer()` (`lib/deferred_utils.glsl:70-117`) samples all
   G-buffers, decodes the normal (`*2-1`), unpacks material flags, detects
   empty pixels via `albedo.a == 0`, and reconstructs the world position
   from depth with `reconstructWorldPosition()` (handles the Vulkan NDC
   Y-flip, `deferred_utils.glsl:44-68`). `F0 = mix(vec3(0.04), albedo,
   metallic)`; roughness clamped ≥ 0.05.
2. Empty pixel → pass through the input image (previous layer) with alpha 0.
3. Unlit material → albedo through `fragmentShaderOutput()` (exposure
   tonemap → linear→sRGB → brightness/contrast,
   `lib/color_correction.glsl:90-95`).
4. **Direct lighting**: loop over the light SSBO skipping disabled lights;
   Cook-Torrance GGX BRDF for point/spot/directional lights
   (`lib/pbr.glsl:72-215`), plus a sheen term scaled by baked AO.
5. **Ambient (IBL)**: `calcAmbientLight()` (`pbr.glsl:217-258`) — irradiance
   cubemap diffuse + prefiltered envmap specular (manual two-LOD
   interpolation) scaled by the BRDF LUT and baked AO, plus sheen.
6. **Emission**: `albedo * lightEmission`.
7. **Refraction**: for translucent fragments, `applyRefraction()` offsets
   the background UV by the view-space normal, sampling the previous layer
   (`deferred_utils.glsl:139-154`).
8. **Final composition**: `compositeFinalColor()`
   (`deferred_utils.glsl:119-134`) sums ambient + direct, applies the
   exposure tonemap (`1 - exp(-c·e)`), gamma encoding and
   brightness/contrast, then blends with the previous layer:
   `mix(inputColor, outColor, albedoAlpha)` — this is the actual
   inter-layer blending mechanism.

### 6.3 RT composite shader

`shaders/src/glsl/deferred_composite_rt.frag.glsl` (requires
`GL_EXT_ray_query`) extends the non-RT shader with:

- **Inline RT shadows** in the light loop (`:211-223`) — see §7.
- **RTAO mode** (`indirectMode == 0`): `calcAmbientLightWithReflections()`
  (`:82-126`). The specular reflection is
  `mix(envReflection, rtReflection.rgb, rtReflection.a)` — the RT reflection
  replaces the prefiltered envmap proportionally to the reflection
  "certainty" stored in alpha. The whole ambient term (diffuse + specular +
  sheen) is scaled by `materialAO * RTAO`.
- **RTGI mode** (`indirectMode == 1`): `calcAmbientLightWithGI()`
  (`:131-176`). Diffuse = `Kd * giIrradiance * albedo` (the GI irradiance
  already encodes occlusion; albedo is applied at composite time for color
  bleeding). Specular still comes from the envmap/RT-reflection blend with
  the BRDF LUT. Baked AO is used only for the direct-light sheen term
  (`:207-209`).

### 6.4 Layer blending and refraction

Because each layer's composite blends `mix(inputColor, outColor,
albedoAlpha)`, transparency is resolved by alpha-blending over the previous
layer's image: skybox → opaque → transparent. The refraction factor stored
in G-buffer attachment 4 alpha perturbs the UV used to sample that
background, producing a screen-space refraction effect. The skybox's black
clear with the albedo-alpha empty-pixel convention ensures untouched pixels
pass the background through unchanged.

---

## 7. Ray-traced shadows

There is **no dedicated shadow pass**; shadows are computed inline with
`rayQueryEXT` in the RT composite fragment shader
(`deferred_composite_rt.frag.glsl:211-223`): for each non-disabled light
with `castShadows != 0`, `shadowFactor = queryShadow(tlas, worldPos, normal,
light, 32)` multiplies the full PBR radiance.

`queryShadow()` (`shaders/src/glsl/lib/ray_tracing.glsl:63-111`):

- Ray direction: `-normalize(light.direction)` for directional lights
  (tMax = 1e6); vector to the light for point/spot (tMax = distance).
- Origin offset along the normal by 0.01 to avoid self-intersection.
- **Hard shadows**: when `shadowSamples <= 1`, a single ray with
  `gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT`.
- **Soft shadows**: up to 32 rays distributed in a cone around the light
  direction; the cone angle derives from `sourceSize` (converted
  degrees→radians for directionals, `atan(sourceSize, distance)` for
  point/spot). Per-sample seeds come from integer hashes of
  `worldPos.xz`. Result: `1 - occluded/samples`, giving a visibility
  fraction (penumbra).

The same `queryShadow()` (with `maxSamples = 8`) shadows the direct lighting
computed inside the GI and reflection closest-hit shaders
(`rt_gi.rchit.glsl:112-119`, `rt_reflections.rchit.glsl:111-120`).

---

## 8. Ray-traced ambient occlusion (RTAO)

Implementation: `lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp`,
`lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp`, shader
`shaders/src/glsl/rt_ao.comp.glsl`.

- **Pass type**: a single **compute** pass using inline `rayQueryEXT`
  against the TLAS (not a `vkCmdTraceRays` pipeline). 8×8 thread groups.
- **Output**: per-frame-in-flight `VK_FORMAT_R8_UNORM` images
  (`RTAmbientOcclusion.cpp:96-108`).
- **Resolution scaling** by quality: Ultra 1.0, High 2/3, Medium 0.5, Low
  1/3 (`RTAmbientOcclusion.hpp:42-53`); default **High**. Changing quality
  waits idle and recreates the images.
- **Fallbacks**: RT unsupported → shared 4×4 white image; null TLAS →
  output cleared to white (`:63-81`, `:163-175`).

**Parameters** (defaults, `RTAmbientOcclusion.hpp:100-107`):

| Parameter | Default | Meaning |
|---|---|---|
| `sampleCount` | 8 | Hemisphere rays per pixel per frame |
| `bounceCount` | 2 | Multi-bounce occlusion iterations |
| `radius` | 0.56 | Max ray distance (world units) |
| `bias` | 0.0017 | Origin offset along the normal |
| `falloff` | 1.0 | Exponent of the distance falloff |
| `bounceAttenuation` | 0.35 | Occlusion contribution decay per bounce |

**Shader algorithm** (`rt_ao.comp.glsl`):

1. Sky pixels (`depth >= 1.0`) write AO = 1.
2. World position reconstructed from depth via `reconstructWorldPosition()`
   with the push-constant `inverseViewProjection`; normal decoded from
   [0,1].
3. Noise seed = `x*1973 ^ y*9277 ^ frameIndex*26699` — hash-based **white
   noise** (no blue noise anywhere in the pipeline).
4. For each sample, for each bounce: a cosine-weighted hemisphere direction
   is generated (`randomHemisphereDirection()`,
   `lib/deferred_utils.glsl:191-206`), and visibility is tested with
   `queryAO()` (`lib/ray_tracing.glsl:134-169`) — terminate-on-first-hit ray
   query with tMax = `radius`.
5. On hit: occlusion accumulates `pow(1 - hitDistance/radius, falloff) *
   contribution`; the origin moves to the hit point, `contribution *=
   bounceAttenuation`, and the next bounce uses the cheap normal
   approximation `-rayDir`.
6. Final `ao = 1 - clamp(occlusion / sampleCount, 0, 1)`.

The raw AO then goes through temporal accumulation (§11) and bilateral
denoising (§12) before reaching the composite.

---

## 9. Ray-traced global illumination (RTGI)

Implementation: `lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp`,
`lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp`, shaders
`shaders/src/glsl/rt_gi.{rgen,rchit,rmiss}.glsl`.

- **Pass type**: full `VK_KHR_ray_tracing` pipeline with raygen / miss /
  closest-hit groups, **max recursion depth 1**
  (`RTGlobalIllumination.cpp:143-149`). Multiple bounces are an iterative
  loop inside the raygen shader, not recursive `traceRayEXT` calls.
- **Output**: per-frame-in-flight `VK_FORMAT_R16G16B16A16_SFLOAT` storage
  images. Resolution scaled by quality: Ultra 1.0, High 2/3, Medium 0.5,
  Low 1/3 (`RTGlobalIllumination.hpp:46-56`); default **Ultra**.
- **Fallbacks**: RT unsupported → 4×4 black RGBA16F; null TLAS → output
  cleared to black.

**Settings** (`RTGISettings`, `RTGlobalIllumination.hpp:58-65`):
`enabled = true`, `sampleCount = 4`, `bounceCount = 2`, `rayBias = 0.02`,
`maxDistance = 50.0`, `quality = Ultra`.

**Descriptor sets**: set 0 = { TLAS, output image, G-buffer depth, G-buffer
normal, irradiance cubemap }; set 1 = `RTMaterialDataBinding` (material /
geometry / texture arrays, §16); set 2 = `ReflectionLightDataBinding` — note
the GI pass uses the *reflection lights* list (lights flagged
`affectsReflections`).

**Raygen shader** (`rt_gi.rgen.glsl`):

1. Sky pixels write black.
2. Path tracing loop: per sample, throughput starts at 1; per bounce, a
   cosine-weighted hemisphere direction is sampled with seed
   `x*1973 ^ y*9277 ^ (frame+1)*26699 ^ s*104729 ^ b*48611`; the ray is
   traced with origin `worldPos + normal*rayBias`, tMax = `maxDistance`.
3. On hit: accumulate `throughput * hitDirectLight` (direct lighting from
   the closest-hit shader — the color-bleeding term), then
   `throughput *= hitAlbedo` (with cosine sampling the π of the Lambert
   BRDF and the PDF cancel out) and continue from the hit point.
4. On the **last** bounce, the path is closed with exactly one terminal
   ambient sample `throughput * irradianceMap(currentNormal)` — added once
   per path so it cannot accumulate into a glow.
5. On miss: the irradiance cubemap is sampled along the escaped ray
   direction — this makes sky-exposed surfaces match the rasterized IBL
   path.
6. Output: average radiance, **alpha = 1.0** (valid-pixel marker for the
   temporal accumulator's HDR path).

**Closest-hit shader** (`rt_gi.rchit.glsl`): fetches the material via
`gl_InstanceCustomIndexEXT` into the `RTMaterialDataBinding` arrays,
interpolates UVs and geometry normals barycentrically (**normal maps are
ignored for GI bounces**), and returns **direct lighting only**: per light,
`queryShadow()` (8 samples) × `computeBasicLighting()` (pure Lambert with
inverse-square attenuation and spot cone,
`shaders/src/glsl/lib/basic_lighting.glsl:82-97`), plus albedo-tinted
emission. Ambient/irradiance is deliberately *not* added here — it would
double-count with the raygen's terminal ambient sample (documented in the
shader comments).

**Miss shader** (`rt_gi.rmiss.glsl`): zeroes the payload; the environment
contribution is handled by the raygen's irradiance sampling.

The GI output goes through HDR temporal accumulation and HDR bilateral
denoising, and is consumed by the composite as `giIrradiance` (§6.3).

---

## 10. Ray-traced reflections

Implementation: `lib/include/bg2e/render/deferred/RTReflections.hpp`,
`lib/src/bg2e/render/deferred/RTReflections.cpp`, shaders
`shaders/src/glsl/rt_reflections.{rgen,rchit,rmiss}.glsl`.

- **Pass type**: `VK_KHR_ray_tracing` pipeline (raygen/miss/closest-hit,
  max recursion 1, `RTReflections.cpp:137-142`), traced over the **full
  render extent** — reflections have no resolution-scaling option.
- **Output**: per-frame-in-flight `R16G16B16A16_SFLOAT` storage images;
  alpha carries the hit-confidence mask.

**Settings** (`RTReflections.hpp:39-46`): `enabled = true`,
`sampleCount = 6`, `maxRoughness = 0.75`, `rayBias = 0.02`,
`maxDistance = 50.0`, `roughnessSpread = 1.0`.

**Descriptor sets**: set 0 = { TLAS, output, G-buffer depth, G-buffer
normal, G-buffer material (roughness from `.g`), irradiance cubemap };
set 1 = `RTMaterialDataBinding`; set 2 = `ReflectionLightDataBinding`.

**Raygen shader** (`rt_reflections.rgen.glsl`):

1. Early-outs (write black, alpha 0): sky pixels, `roughness >
   maxRoughness`, degenerate normals.
2. Perfect mirror direction `reflect(viewDir, normal)`. **GGX importance
   sampling** (`sampleGGXLike()`) maps a GGX half-vector distribution
   (`a = max(roughness², 0.001)`, roughness scaled by `roughnessSpread`)
   around the reflection direction; sampled directions with
   `dot(dir, N) <= 0.01` are clamped back toward the mirror direction.
   Roughness ≤ 0.01 traces the exact mirror ray.
3. **Adaptive sample count**: `samples = max(1, ceil(sampleCount *
   (roughness/maxRoughness)²))` — mirror-like pixels trace 1 ray.
4. Rays: origin `worldPos + normal*rayBias`, tMin 0.001, tMax
   `maxDistance`, `gl_RayFlagsOpaqueEXT`.
5. Output: average color over **valid hits only**; **alpha = validHits /
   samples** — a continuous "reflection certainty" used by the composite to
   blend against the prefiltered envmap (`mix(envReflection,
   rtReflection.rgb, rtReflection.a)`) and stabilized by the temporal
   accumulator so intermittently-hit pixels converge instead of flickering.

**Closest-hit shader** (`rt_reflections.rchit.glsl`): material fetch and
barycentric interpolation as in GI (normal maps ignored); lighting =
`albedo * irradiance(worldNormal)` + Σ shadowed Lambert direct light over
the reflection-lights list + albedo-tinted emission.

**Miss shader**: returns black with `didHit = 0` — the sky contribution
comes from the rasterized prefiltered envmap through the certainty blend in
the composite.

---

## 11. Temporal accumulation

Implementation: `lib/include/bg2e/render/deferred/TemporalAccumulator.hpp`,
`lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`, shader
`shaders/src/glsl/temporal_accumulation.comp.glsl`.

Three instances exist per deferred layer: **AO** (scalar, default format
`R16_SFLOAT`), **reflections** (HDR `R16G16B16A16_SFLOAT`) and **GI** (HDR).
All run at **full layer extent** — lower-resolution AO/GI inputs are
bilinearly upsampled through the sampler.

**Buffers** (per frame-in-flight, `TemporalAccumulator.cpp:80-165`):

- Two ping-pong history images (`_historyImagesA/B`).
- Previous-frame depth and normal copies (formats taken from the G-buffer),
  updated each frame with `vkCmdCopyImage` after the accumulation dispatch
  (`:285-350`).

**Host-side logic** (`render()`, `:205-362`):

- **Camera motion invalidation**: if the view-projection matrix changed by
  more than ε = 0.001 since the previous frame, history is dropped and the
  accumulation counter resets (`:220-232`).
- `invalidateHistory()` is public API for explicit resets.

**Descriptor set 0**: current input, history read image, current G-buffer
depth, current G-buffer normal, output storage image, previous depth,
previous normal.

**Shader algorithm** (`temporal_accumulation.comp.glsl`):

1. **Reprojection**: world position from current depth + current inverse
   view-projection → multiplied by `previousViewProjection` → perspective
   divide → UV (with Vulkan Y-flip).
2. **History rejection**: no history flag, `previousClip.w <= 0`,
   reprojected UV outside [0,1], `|currentDepth - historyDepth| >=
   depthThreshold`, or `dot(currentNormal, historyNormal) <=
   normalThreshold`.
3. **Blending** — two modes (`AccumulationMode`, hpp):
   - **Interactive** (default): exponential moving average,
     `mix(history, current, 1 - historyWeight)` with default
     `historyWeight = 0.9`.
   - **Progressive**: true running mean, `weight = 1/(accumulatedFrames+1)`
     — converges while the camera is static.
4. The **HDR path blends RGB and alpha**, so the reflection certainty mask
   is also accumulated.
5. Sky pixels: scalar mode passes the current value through; HDR mode
   writes 0.

**Parameters** (defaults): `historyWeight = 0.9`, `depthThreshold = 0.01`,
`normalThreshold = 0.8`.

---

## 12. Denoising (bilateral filter)

Implementation: `lib/include/bg2e/render/deferred/DenoiseFilter.hpp`,
`lib/src/bg2e/render/deferred/DenoiseFilter.cpp`, shaders
`shaders/src/glsl/denoise_bilateral.comp.glsl` (LDR, AO) and
`denoise_bilateral_hdr.comp.glsl` (HDR, GI).

- Single compute pass, 8×8 groups, full layer extent.
- Variant selected at build time by `setIsHDR()`: LDR writes `R8_UNORM`
  (only R used), HDR writes `R16G16B16A16_SFLOAT`.
- **Descriptor set 0**: noisy input, G-buffer normal (guidance), G-buffer
  depth (guidance), output storage image.

**Filter math** (per tap in a `(2r+1)²` window, default `kernelRadius = 2`
→ 5×5):

- Spatial weight: Gaussian `exp(-(x²+y²) / (2r²))`.
- Depth weight: 1.0 if `|Δdepth| < depthThreshold` (0.01), else Gaussian
  with `depthSigma` (0.01).
- Normal weight: 1.0 if `dot(n0,n1) > normalThreshold` (0.8), else
  `exp(-(1 - dot) / normalSigma)` with `normalSigma` = 0.3.
- Normalized weighted sum. Sky pixels: LDR writes white (AO = 1), HDR
  writes black with alpha 1. The HDR variant filters only RGB — the
  reflection certainty channel is preserved by the temporal accumulator,
  not by this filter.

Reflections are **not** spatially denoised (temporal accumulation only).

---

## 13. Motion vector generation

Implementation: `lib/include/bg2e/render/deferred/MotionVectorGenerator.hpp`,
`lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp`, shader
`shaders/src/glsl/motion_vectors.comp.glsl`.

- **Purpose**: produce camera-motion vectors for FSR 3 (also harmless when
  the SMAA path is active).
- **Output**: per-frame-in-flight `VK_FORMAT_R16G16_SFLOAT` image at render
  resolution, storage + sampled (`MotionVectorGenerator.cpp:96-107`).
- **Inputs** (push constants): current inverse view-projection, previous
  view-projection — both **unjittered** — and the render size.

**Shader logic** (`motion_vectors.comp.glsl`):

1. Reconstruct the world position from the current depth and the current
   inverse view-projection (with the Vulkan NDC Y-flip).
2. Reproject into the previous frame; if `prevClip.w <= 0` (behind the
   previous camera) or depth ≥ 1.0 (sky), write zero motion.
3. **Motion vector = `(prevUV - currentUV) * renderSize`** — pixel units,
   pointing from the current pixel to where the surface was in the previous
   frame.

This captures **camera motion only** — there are no per-object velocity
inputs, so moving objects inherit the background motion at their depth.
FSR receives these vectors with `motionVectorScale = {1, 1}`
(`FSRPostProcessor.cpp:390`) and the jitter offset separately.

---

## 14. Final post-processing

The `deferred::FinalPostProcessor` interface
(`lib/include/bg2e/render/deferred/FinalPostProcessor.hpp:40-105`) defines:

- `prepare(projMatrix, frameCounter, renderExtent)` → returns the
  (possibly jittered) projection matrix.
- `process(cmd, frameIndex, colorInput, depthInput, motionVectors,
  colorOutput, deltaMs, near, far, fovVertical)` — executes the pass and
  guarantees `colorOutput` is left in `COLOR_ATTACHMENT_OPTIMAL`.
- Scale UI API: `processorName()`, `scaleOptions()`, `setScaleOption()`,
  `scaleOption()`, `renderScalePercent()`.

### 14.1 SMAA path

Files: `SMAAPostProcessor.{hpp,cpp}`, `SMAAProcessor.{hpp,cpp}`
(`lib/src/bg2e/render/deferred/`), shaders
`smaa_edge_detection.comp.glsl`, `smaa_blend_weight.comp.glsl`,
`smaa_neighborhood_blend.comp.glsl`, shared include
`shaders/src/glsl/lib/smaa.glsl`, LUTs in
`lib/third_party/iryku_smaa/{AreaTex.h,SearchTex.h}`.

This is **SMAA 1x** (no temporal component) implemented as **three compute
passes**, a faithful port of the reference implementation by Jimenez et al.,
configured **orthogonal-only** (no diagonal detection, no corner rounding —
≈ LOW/MEDIUM preset, `smaa_blend_weight.comp.glsl:23-25`):

- `SMAA_THRESHOLD 0.05`, `SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0`,
  `SMAA_MAX_SEARCH_STEPS 16` (`lib/smaa.glsl:22-36`).
- Luma edge detection with Rec.709 weights
  (`smaa_edge_detection.comp.glsl:47`).
- Official precomputed AreaTex (160×560, `R8G8_UNORM`) and SearchTex
  (64×16 packed, `R8_UNORM`) uploaded once and kept in
  `SHADER_READ_ONLY_OPTIMAL` (`SMAAProcessor.cpp:67-95`). A single shared
  LINEAR/CLAMP sampler is used — the blend-weight accelerated search relies
  on bilinear interpolation of the edges texture.

**Passes** (`SMAAProcessor::process()`, `SMAAProcessor.cpp:261-358`), all
8×8 compute dispatches at render resolution:

1. **Edge detection** — luma deltas vs left/top neighbors, threshold step,
   local contrast adaptation; output `R8G8_UNORM` (r = vertical edge,
   g = horizontal edge).
2. **Blend weight generation** — orthogonal searches
   (`SMAASearchXLeft/Right/YUp/Down`) with bilinear-accelerated 2-texel
   steps and SearchTex length lookups; area LUT indexing (`SMAAArea`);
   output `R8G8B8A8_UNORM` (rg = north-edge weights, ba = west-edge
   weights).
3. **Neighborhood blending** — gathers weights from the pixel and its
   right/bottom neighbors, picks the dominant axis, performs the final
   blend with two bilinear fetches.

**Upscaling**: SMAA runs entirely at render resolution. If render extent ≠
display extent, `SMAAPostProcessor::process()`
(`SMAAPostProcessor.cpp:74-120`) issues a `vkCmdBlitImage` with
`VK_FILTER_LINEAR` to the display target (plain linear upscale, or
downscale at 150 % supersampling); same-size uses a plain copy.
`prepare()` returns the projection unchanged — **no jitter**.

### 14.2 FSR 3.1 path

Files: `FSRPostProcessor.{hpp,cpp}` (`lib/src/bg2e/render/deferred/`),
Windows/Linux only (`#if !defined(__APPLE__)`).

- **AMD FidelityFX Super Resolution 3.1** upscaler through the **FidelityFX
  SDK 1.1.4 host API** (`lib/third_party/FidelityFX-SDK-1.1.4`), Vulkan
  backend (`ffx_fsr3upscaler.h`, `ffx_vk.h`).
- Context creation (`FSRPostProcessor.cpp:70-112`): scratch memory query,
  `ffxGetInterfaceVK`, `FfxFsr3UpscalerContextDescription` with
  `FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE`, max render/upscale sizes
  from the current extents. `resize()` waits idle and recreates the entire
  context (`:55-64`).
- **Shared resources** queried from FSR and allocated by the engine
  (`:138-197`): dilated depth (`R32_SFLOAT`), dilated motion vectors
  (`R16G16_SFLOAT`), reconstructed-prev-nearest-depth (`R32_UINT`), plus
  per-frame-in-flight **display-resolution output images** (swapchain
  images usually lack `STORAGE_BIT`, so FSR cannot write them directly).
  sRGB formats are mapped to their UNORM equivalents for storage; the final
  blit into an sRGB destination performs the sRGB encoding automatically.
- **Jitter** (`prepare()`, `:215-240`): Halton(2,3) offsets from
  `ffxFsr3UpscalerGetJitterOffset` with a phase count from
  `ffxFsr3UpscalerGetJitterPhaseCount` (widths only), applied to projection
  matrix column 2 in NDC units, Y negated for Vulkan's clip space.
- **Dispatch** (`process()`, `:314-433`): color, depth and motion vectors
  as compute-read inputs; per-frame output as UAV; shared resources as UAV;
  `jitterOffset` from `prepare()`; `motionVectorScale = {1,1}`; **RCAS
  sharpening enabled with sharpness 0.5**; `frameTimeDelta`, near/far and
  vertical FOV forwarded from the renderer. **`exposure`, `reactive` and
  `transparencyAndComposition` masks are not provided** (empty resources),
  and `reset` is never set — there is no history reset on camera cuts.
- Final step: NEAREST same-size blit from the FSR output to `colorOutput`
  (sRGB conversion happens here), leaving it in `COLOR_ATTACHMENT_OPTIMAL`.

### 14.3 Path selection and scale options

Selection in `RendererDeferred::build()` (`RendererDeferred.cpp:248-271`):
the motion vector generator is always built; on non-Apple platforms FSR is
attempted first, and any initialization failure (or macOS) falls back to
SMAA with a logged warning.

| Aspect | SMAA path | FSR path |
|---|---|---|
| Technique | SMAA 1x, luma edges, orthogonal only, threshold 0.05, 16 search steps | FSR 3.1 upscaler, HDR, RCAS 0.5 |
| Jitter | None | Halton(2,3) via SDK |
| Depth / motion | Unused | Depth + RG16F camera-only motion vectors |
| Reactive / T&C masks | N/A | Not provided |
| Upscale | Linear blit after SMAA at render res | FSR dispatch → per-frame display-res intermediate → NEAREST blit |
| Scale options | 25 / 50 / 75 / 100 / 150 % (default 50 %) | Quality 67 % / Balanced 59 % / Performance 50 % / Ultra 33 % (default Performance), derived from `ffxFsr3UpscalerGetUpscaleRatioFromQualityMode` (`FSRPostProcessor.cpp:444-467`) |
| Platforms | All | Windows / Linux |

Changing the FSR quality mode is heavyweight (full context recreation with
`waitIdle`); the SMAA path only recreates its per-frame images.

---

## 15. Debug visualization

`DeferredDebugVisualization` (`DeferredLayer.hpp:49-68`) selects an
alternative output for both deferred layers:

`FullComposition` (default), `GBufferAlbedo`, `GBufferNormal`,
`GBufferMaterial`, `GBufferFresnelFlags`, `GBufferSheenColor`,
`GBufferDepth`, `InputImage`, `RTAmbientOcclusion`, `DenoisedAO`,
`TemporalAccumulatedAO`, `RTReflections`, `TemporalAccumulatedReflections`,
`RTReflectionMask`, `RTGlobalIllumination`, `DenoisedGI`.

`resolveDebugSource()` (`DeferredLayer.cpp:39-86`) maps each mode to the
corresponding image; `renderDebugPass()` (`:1199-1242`) blits it through the
debug pipeline (`deferred_debug_blit.{vert,frag}.spv`) with a
`channelMode` push constant: 0 = RGB passthrough, 1/2/3 = R/G/B as gray,
**4 = alpha as gray** (used for `RTReflectionMask`), 5 = premultiplied RGB·A.

---

## 16. Ray tracing data bindings

Located under `lib/include/bg2e/render/vulkan/rt/` (implementations in
`lib/src/bg2e/render/vulkan/rt/`):

- **`RayTracingSceneDataBinding`** — single binding 0 =
  `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR`, stages
  FRAGMENT | RAYGEN | CLOSEST_HIT. Bound as set 4 in the RT composite.
  Returns `VK_NULL_HANDLE` when RT is unavailable.
- **`ReflectionLightDataBinding`** — SSBO of up to 8 `base::LightData`
  (binding 0, stage CLOSEST_HIT). Contains only lights flagged
  `affectsReflections`, so the RT hit shaders iterate the smallest set; a
  dummy disabled light keeps the descriptor valid when the list is empty.
  The GLSL `LightData` layout is in `shaders/src/glsl/lib/uniforms.glsl:70-87`.
- **`RTMaterialDataBinding`** — `MAX_OBJECTS = 256`:
  - binding 0: `RTMaterialData[]` SSBO (56 bytes: albedo color/scale,
    `indexOffset` for submesh indexing, light emission
    factor/channel/invert/scale/UV set).
  - bindings 1/2: arrays of 256 vertex-buffer and index-buffer SSBOs
    (`RTVertex` = position, normal, uv0, uv1, tangent).
  - bindings 3/4: arrays of 256 albedo and light-emission `sampler2D`.
  Missing slots are filled with dummy buffers/white textures. Hit shaders
  index these arrays with `gl_InstanceCustomIndexEXT`, and
  `gl_PrimitiveID` is corrected with `mat.indexOffset` because it excludes
  the BLAS primitive offset.

---

## 17. Known limitations and implementation notes

- **No blue noise** — all stochastic sampling uses integer-hash white noise
  seeded per pixel + frame + sample/bounce index; convergence relies on
  temporal accumulation.
- **Resolution strategy** — AO and GI render at quality-scaled resolution
  (1.0 / 0.667 / 0.5 / 0.333); reflections always full-res; temporal
  accumulation and denoise always run at full extent, bilinearly upsampling
  scaled inputs.
- **Transparent layer has no depth testing** — ordering comes from
  render-queue sorting; the copied opaque depth serves only downstream
  passes (motion vectors, debug views).
- **Camera-only motion vectors** — dynamic objects inherit background
  motion; combined with the missing FSR reactive/transparency masks, dynamic
  and transparent geometry may ghost under FSR.
- **FSR history is never reset** (`dispatch.reset` is always false) — camera
  cuts or scene switches can ghost.
- **Temporal history is dropped on any camera motion** (matrix ε = 0.001),
  so Progressive mode only converges with a static camera.
- **RT pipelines use maxRecursion = 1** — GI multi-bounce is an explicit
  loop in the raygen shader.
- **Normal maps are ignored in RT hit shaders** — GI and reflection bounces
  use interpolated geometry normals only.
- **The transparent layer can skip indirect passes**
  (`setSkipIndirectLightingForTransparent`, `DeferredLayer.hpp:179-180`),
  binding a neutral white AO instead.
- Minor code observations: a stale "3 color attachment formats" comment
  (`DeferredLayer.cpp:825`, actually 5); the G-buffer pipeline layout
  declares an unused fragment push-constant range (`:795-799`);
  `renderCompositePass` dereferences `frameResources.rayTracingScene`
  unconditionally (`:1078`); `RendererDeferred::updateLights` mixes
  `resize()` + `push_back()` producing default-constructed leading entries
  that the shader skips only if their default `type` equals disabled
  (`RendererDeferred.cpp:980-987`); intermediate images are transitioned
  from `UNDEFINED` after being written (`RendererDeferred.cpp:468-479`).

---

## 18. File and shader reference index

### C++ — renderer core

| File | Role |
|---|---|
| `lib/include/bg2e/render/RendererDeferred.hpp` / `lib/src/bg2e/render/RendererDeferred.cpp` | Renderer orchestration, layers, render scale, per-frame sequence |
| `lib/include/bg2e/render/deferred/RenderLayer.hpp` / `lib/src/bg2e/render/deferred/RenderLayer.cpp` | Layer base class |
| `lib/include/bg2e/render/deferred/SkyboxLayer.hpp` / `lib/src/bg2e/render/deferred/SkyboxLayer.cpp` | Skybox layer |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` / `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | G-buffer + indirect + composite layer |
| `lib/include/bg2e/render/gbuffer/GBufferManager.hpp` / `lib/src/bg2e/render/gbuffer/GBufferManager.cpp` | G-buffer allocation and layout transitions |

### C++ — RT and accumulation

| File | Role |
|---|---|
| `lib/.../deferred/RTAmbientOcclusion.{hpp,cpp}` | RTAO compute pass |
| `lib/.../deferred/RTGlobalIllumination.{hpp,cpp}` | RTGI path tracing pass |
| `lib/.../deferred/RTReflections.{hpp,cpp}` | RT reflections pass |
| `lib/.../deferred/TemporalAccumulator.{hpp,cpp}` | Temporal reprojection/accumulation |
| `lib/.../deferred/DenoiseFilter.{hpp,cpp}` | Bilateral denoise (LDR/HDR) |
| `lib/include/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp` | TLAS descriptor binding |
| `lib/include/bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp` | RT light SSBO binding |
| `lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp` | RT material/geometry/texture arrays |
| `lib/include/bg2e/scene/vk/DeferredLightDataBinding.hpp` | Composite light SSBO (max 8 lights) |

### C++ — post-processing

| File | Role |
|---|---|
| `lib/.../deferred/FinalPostProcessor.hpp` | Post-processor interface |
| `lib/.../deferred/MotionVectorGenerator.{hpp,cpp}` | Motion vector pass |
| `lib/.../deferred/SMAAProcessor.{hpp,cpp}` | SMAA 1x three-pass implementation |
| `lib/.../deferred/SMAAPostProcessor.{hpp,cpp}` | SMAA + blit upscaling wrapper |
| `lib/.../deferred/FSRPostProcessor.{hpp,cpp}` | FSR 3.1 integration (Win/Linux) |
| `lib/third_party/iryku_smaa/{AreaTex.h,SearchTex.h}` | SMAA lookup tables |
| `lib/third_party/FidelityFX-SDK-1.1.4` | FidelityFX SDK (FSR 3.1) |

### Shaders (`shaders/src/glsl/`)

| Shader | Pass |
|---|---|
| `deferred_gbuffer.vert.glsl` / `deferred_gbuffer.frag.glsl` | G-buffer fill |
| `deferred_composite.vert.glsl` | Fullscreen quad (shared with debug blit) |
| `deferred_composite.frag.glsl` | Lighting composite (non-RT) |
| `deferred_composite_rt.frag.glsl` | Lighting composite (RT shadows + AO/GI + reflections) |
| `deferred_debug_blit.vert.glsl` / `.frag.glsl` | Debug channel visualization |
| `rt_ao.comp.glsl` | RTAO (inline ray queries) |
| `rt_gi.rgen.glsl` / `rt_gi.rchit.glsl` / `rt_gi.rmiss.glsl` | RTGI path tracing |
| `rt_reflections.rgen.glsl` / `.rchit.glsl` / `.rmiss.glsl` | RT reflections |
| `temporal_accumulation.comp.glsl` | Temporal accumulation |
| `denoise_bilateral.comp.glsl` / `denoise_bilateral_hdr.comp.glsl` | Bilateral denoise (LDR / HDR) |
| `motion_vectors.comp.glsl` | Motion vector generation |
| `smaa_edge_detection.comp.glsl` | SMAA pass 1 |
| `smaa_blend_weight.comp.glsl` | SMAA pass 2 |
| `smaa_neighborhood_blend.comp.glsl` | SMAA pass 3 |

### Shared shader includes (`shaders/src/glsl/lib/`)

| Include | Content |
|---|---|
| `uniforms.glsl` | `PBRMaterialData`, `LightData`, light type constants, material texture samplers |
| `pbr.glsl` | Cook-Torrance GGX BRDF, IBL ambient |
| `color_correction.glsl` | Exposure tonemap, gamma, brightness/contrast |
| `deferred_utils.glsl` | G-buffer decode, world-position reconstruction, composite/refraction, hash/rand/hemisphere helpers |
| `normal_map.glsl` | TBN construction |
| `ray_tracing.glsl` | `queryShadow()` (hard/soft RT shadows), `queryAO()` |
| `basic_lighting.glsl` | Lambert direct lighting for RT hit shaders |
| `rt_material_data.glsl` | RT material/vertex structures and sampling |
| `smaa.glsl` | SMAA constants and LUT metrics |
