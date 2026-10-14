# Materials, textures, environments, and shader contracts

[Overview](index.md) · [Scene boundary](03-scene-boundary.md) · [Graphics tools](10-selection-ui-and-tools.md)

## 1. Three material layers

The current system already has a useful CPU definition in [`base::MaterialAttributes`](../../lib/include/bg2e/base/MaterialAttributes.hpp). Production [`MaterialBase`](../../lib/include/bg2e/render/MaterialBase.hpp) adds uploaded textures; [`scene::vk::ObjectDataBinding`](../../lib/src/bg2e/scene/vk/ObjectDataBinding.cpp) packs attributes and image/sampler descriptors for shaders.

Preserve this conceptual separation more clearly:

| Layer | Proposed owner | Responsibility |
|---|---|---|
| Editable material definition | `base` and scene submesh data | Values, texture descriptions, UV/channel semantics and serialization. |
| Loaded material resources | `draw` | Device textures/samplers, missing-resource fallbacks and update revisions. |
| Shader binding/packed representation | Renderer/pass internals in `draw` | Object transform, material struct, resource-set schema and per-slot lifetime. |

Scene should not expose loaded `draw::MaterialResource` simply because it exposes editable submesh attributes. Pair resources during draw extraction/realization.

## 2. Material parity surface

The port must account for more than albedo/normal/metalness/roughness:

- Albedo including alpha and sRGB/linear interpretation.
- Metalness, roughness, normal, AO and light-emission textures, plus scalar factors.
- Texture channels, inversion, scale and UV set where supported by existing packing/shaders.
- Two UV sets and tangents in standard geometry.
- Alpha cutoff with matching raster and supported RT hit behavior.
- Transparent/solid flags and screen-space refraction factor.
- Unlit flag, Fresnel tint, sheen color/intensity and emission.
- AO texture metadata that distinguishes ordinary AO from baked RGB light multipliers.
- Baked-map modulation of indirect lighting while preserving direct contribution and emission semantics.

The production raster path and RT hit-material path do not expose identical channel support. Document that actual difference rather than making the new framework silently expand traced-hit material semantics.

Mutable material references and the existing updated flags are a challenge for dual backends. A flag consumed/cleared by one realization can leave another stale if shared carelessly. Prefer definition revisions observed independently by each device realization, explicit edit notifications, or a transaction/update API. Retain legacy flag behavior inside compatibility wrappers.

## 3. Texture realization and cache scope

[`render::Texture`](../../lib/include/bg2e/render/Texture.hpp) wraps a base texture description, Vulkan image, native sampler and ownership rules. It has static black/white/normal defaults. [`TextureCache`](../../lib/src/bg2e/utils/TextureCache.cpp) is a global singleton returning production GPU textures; its key includes path and sampler/color settings but not engine/device identity.

**Proposed:** device-scoped texture/image/sampler services in `draw`, with CPU decoding separate. Cache immutable image content independently from sampler descriptions where useful. Include color interpretation, mip requirements and device generation in realization identity. Do not reuse production GPU textures in Metal or a different experimental Vulkan device.

Make ownership explicit when wrapping an image: owned resource, shared resource, or borrowed view. Current pointer overloads sometimes construct owning shared pointers from raw pointers; carrying that interface forward without clear semantics risks double destruction and accidental device lifetime extension.

A material reload can replace resource references while old frames continue using previous textures. Publish the new revision and retire old resources after completion. Default textures need the same device-scoped lifetime, even though their contents are constant.

## 4. Image upload and mipmaps

Experimental Image supports RGBA8 uploads and `uploadImage(base::Image*)`, including inspected half-float conversion for RGBA16F. CubeMap supports mip chains and face/mip rendering. That provides a basis for environment processing.

It is not proof of complete production Texture parity. The inspected public command API has no general texture-mipmap-generation operation, and base image upload methods target the base image path. Review ordinary 2D mip allocation/upload/generation, supported source formats, float sources, sampler LOD/address behavior, and sRGB handling.

GPU should own generic image upload/copy/subresource/mipmap mechanics where needed. Draw selects whether a material texture needs mips, preserves color/normal interpretation, and invokes resource generation. Do not place file loading or material texture selection in GPU.

## 5. IBL and sky resources

Production [`EnvironmentResources`](../../lib/include/bg2e/render/EnvironmentResources.hpp) coordinates sphere-to-cubemap, irradiance convolution, specular prefilter, BRDF integration map and skybox drawing. It also manages samplers, environment texture switching and sky blur.

Suggested objects in `draw`:

- Environment resource bundle: source/cubemap, irradiance, prefiltered map, BRDF LUT and sampler handles.
- Generation passes: equirectangular conversion, diffuse convolution, specular mip generation and LUT generation.
- Environment updater/cache: identity/revision and asynchronous-safe replacement.
- Skybox pass/layer: camera/view and draw configuration.

Reuse `gpu::CubeMap` and `CubemapRenderPass` for face/mip iteration. [`examples/gpu/10_cubemap_render_pass`](../../examples/gpu/10_cubemap_render_pass/) already contains Vulkan and Metal conversion/convolution/prefilter shader examples. They reduce discovery work, but need comparison against production sampling, roughness/mips, color handling and resource lifetime before being treated as equivalent engine services.

`scene::EnvironmentComponent` keeps declarative environment input. CPU SkyDomeTextureGenerator can move to a neutral procedural utility if desired; it does not need to become a GPU object. No-RT rendering still requires IBL, so environment generation must not be gated on acceleration support.

## 6. Shader families and destinations

Current engine shaders reside primarily in `shaders/src/glsl`, with experimental counterparts in example-local shader directories. The global Metal folder contains a compile test, not a complete deferred production-equivalent library.

| Family | Production examples | Experimental work |
|---|---|---|
| G-buffer/composite | `deferred_gbuffer`, `deferred_composite`, `deferred_composite_rt`, debug blit. | Preserve channel schema and port binding/output declarations. |
| Shared lighting/material | `lib/pbr`, `uniforms`, normal map, color correction, deferred utilities. | Reuse algorithms; review layout, matrices and color domain. |
| Environment | Sphere conversion, irradiance, prefilter, skybox, BRDF LUT. | Promote reviewed example implementations to reusable shader resources. |
| RT | AO compute, GI/reflection raygen/miss/hit, alpha any-hit, RT material helpers. | Vulkan binding port; Metal kernel/intersection equivalent and material addressing. |
| Temporal/denoise | Scalar/HDR bilateral, temporal accumulation and motion vectors. | Match packing/coordinates/history reset and signal formats. |
| Spatial AA | SMAA edge/weights/neighborhood plus lookup textures. | Preserve facility; agree public final-mode integration. |
| Picking/overlays | Pick selection, selection highlight and gizmos. | Preserve ID encoding, blend/depth behavior and coordinate conventions. |
| Baking/tools | UV surface, UV preview, accumulation, composition and dilation. | Independent shader profiles and typed output/readback. |

Forward scene shaders are excluded, but shared PBR/math/environment code used by deferred is not. Avoid deleting shared GLSL merely because forward no longer has an experimental implementation.

## 7. Binding schema and shader ABI

Production shaders use native descriptor sets and often combined samplers. GPU ResourceSet has separate image/sampler resource types and Vulkan/Metal binding indices. ShaderLib resolves compiled stage files with backend-specific entry points/conventions. A direct filename/path rename does not adapt descriptor declarations.

Define one logical resource schema per pass and explicit backend mappings. Check:

- CPU struct size, field offsets and scalar/std140/std430/Metal alignment.
- Texture/sampler separation and array addressing.
- Stage visibility and reserved Metal buffer indices.
- Vertex layout, especially standard PNUUT and RT hit reconstruction.
- Depth range, handedness, projection Y convention, jitter direction and image/readback orientation.
- Sampled versus storage-image format declarations and integer output types.
- Dispatch dimensions and workgroup sizes; inspected Metal dispatch uses a fixed default group convention, so shaders must match or the primitive contract must grow.

Production's `RTMaterialData` has a scalar-layout 56-byte assertion. Metal ABI equivalence requires deliberate packing; do not assume a same-named struct matches automatically.

Prefer private renderer packing definitions with shared metadata or assertions where useful. Do not put renderer-specific structs into GPU simply to share them across shaders.

## 8. FSR and final processing

[`FSRPostProcessor.cpp`](../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp) uses FidelityFX Vulkan interfaces and FSR3 **upscaler** contexts. It supplies color, depth and camera/depth-based motion vectors; reactive and transparency/composition resources are empty. No frame-generation integration is present in this path. Dispatch sets reset to false; context recreation/mode transitions and a richer explicit history reset API deserve review.

Preserve Quality/Balanced/Performance/UltraPerformance/NativeAA options and Direct fallback where currently supported. The production code excludes the FSR implementation on Apple. A new Metal FSR path would be an additional integration project, not existing parity.

The upscaler writes storage-capable intermediate outputs before copying/blitting to a presentation-compatible target. Draw's retained target currently lacks Storage and TransferDst usage, so an adapted final pass should sample/blend or use an explicitly reviewed target contract. Treat native SDK state mutations carefully.

SMAA exists as an implementation family but is not selected by current `FinalRenderMode`. Decide whether the new API exposes it as a selectable mode or an independent processing service; either preserves its availability without pretending it is an active production default.

## 9. Shared assets during coexistence

CPU geometry, material definitions and image sources may be shared under a revision/immutability contract. GPU textures, samplers, resource sets and native shaders belong to their device/backend. Shader algorithms can be shared, but compiled artifacts and binding ABI are backend-specific.

Keep production shader resource paths functioning until application migration. Packaging/install and standalone-project tools must eventually include the new shader libraries and optional SDK assets; that is later implementation/build-system work requiring explicit authorization, not a change made by this analysis.
