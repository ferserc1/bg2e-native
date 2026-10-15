# M01-10 actual deferred graph

## Public objects and lifecycle

`draw/RendererDeferred.hpp` implements Renderer. Constructor accepts a deployed shader root (empty uses PlatformTools::shaderPath) and optional acquired-slot count (zero derives WindowSurface::inFlightFrames). Initialize with resolved output/internal extents, output format and depth D16_UNORM or D32_SFLOAT. Normalized/floating output formats are accepted subject to device capabilities; integer/stencil outputs are rejected. All targets are single sample.

The public stages are `pipeline::GBuffer`, `DeferredLayer`, `FinalLayer`, and `environment::SkyboxRenderer`/`SkyboxPass`. Missing stages are defaulted; RendererConfiguration.layers replaces stages by exact shaderInterface identity, not vector position. Renderer stores their ordered selections, validates declared extents/formats/semantics/usages, initializes them, and validates their actual contracts. Unknown/duplicate interfaces and incompatible replacements throw. Passes are shared-owned and belong to one active renderer lifecycle. Lifecycle failures require cleanup before reuse; initialized selections are retained early enough for partial cleanup. Extent changes rebuild slot targets at a stopped boundary; format changes use the delegate's full cleanup/initialize lifecycle. Pre-initialized injected passes declare their formats through constructor arguments or their own contract.

`Renderer::prepareFrame()` is a compatible default no-op returning whether a scene frame is required. DefaultRenderLoopDelegate invokes it before acquisition on ordinary presentations. RendererDeferred generates/synchronizes environment snapshots there, or at scene attachment/initialization while producers are stopped and before recording begins. It never generates during update/record. The existing first resolved surface lifecycle may initialize/attach after acquiring the first drawable, before command recording and after draining users. Ordinary environment changes use the pre-acquire hook. Failed HDR replacement retains usable prior resources and sets environmentError(); a later successful synchronize clears it. A supplied RendererConfiguration.environment is validated/fixed and bypasses generation. Animation-driven environment edits are observed on the next prepare hook. Direct callers must invoke this safe hook themselves.

Update runs the shared scene UpdateVisitor once with sceneDeltaSeconds, selects the first enabled camera (or explicit setCamera matrices, or identity), extracts/sorts a draw queue, prepares the four bindings once for the acquired presentation, and updates passes. Scene attachment/resize updates camera viewport to internal extent. useSceneCamera resets explicit camera selection. record requires that acquired slot's update and no active scope. UI/settings callers request a scene refresh when changing noncontinuous scene/settings. This step adds no input or manipulation policy.

Each acquired slot owns six color targets, working depth, preserved opaque depth, two HDR colors and a queue snapshot. UBOs and object sets use the same acquired slots. Shared geometry/textures are not duplicated into geometry rings. Fullscreen draws retain distinct image descriptor sets for each recorded invocation; a transparent draw never overwrites an earlier invocation's descriptors. Slot sets/snapshots are retired only after the caller actually acquires/waits that slot. All images/resources and the Engine must survive completion. Explicit cleanup waits and is required while Engine is alive; normal update/record does not waitIdle, upload, submit or generate environments. DefaultRenderLoopDelegate already owns the drained cleanup lifecycle.

## Public building blocks

- `RenderTarget::create` validates a sampled color/depth attachment plus exact transfer usages, creates one level/sample, and returns shared GPU ownership. clear and copy record generic commands; the caller retains images and owns command submission.
- `FullscreenPass` loads an explicit fragment library plus draw/raster/fullscreen vertex, accepts an explicit layout/output format/slot count, binds ordered images/samplers plus caller-selected sets, and records a procedural triangle. beginFrame/draw retire only a completed acquired slot's previous sets. Inputs are borrowed through submission completion; keep their owners. Its nearest sampler avoids format filtering assumptions; the final shader explicitly reconstructs bilinear samples.
- `RasterFrame` appends optional borrowed queue/item/binding pointers to PassContext. A null item selects opaque geometry; a non-null item selects one transparent submesh. Application passes can consume this public data and use Renderer::recordLayer validation. RasterFrame is never retained by a pass.
- No compute stage is required by this raster graph. Existing gpu::ComputePipeline/CommandBuffer compute primitives remain available; future processors add reusable public compute orchestration when actually needed rather than idle renderer placeholders.

## Image contracts and ordering

| Index | Semantic | Format | Packing |
|---|---|---|---|
| 0 | albedo | RGBA8 UNORM | linear RGB, linear coverage alpha |
| 1 | normalEmission | RGBA16 float | world normal * 0.5 + 0.5, scalar emission |
| 2 | material | RGBA8 UNORM | metalness, roughness, AO, sheen intensity |
| 3 | fresnelFlags | RGBA8 UNORM | Fresnel tint, exact material flag bits / 255 |
| 4 | sheenRefraction | RGBA8 UNORM | sheen RGB, refraction factor |
| 5 | bakedMultiplier | RGBA8 UNORM | neutral white RGB; no bake lookup in M01 |
| 6 | depth | D16 or D32 | nearest surviving surface |

GBuffer contract `draw.raster.gbuffer.v1` has no sampled pass inputs and these seven internal outputs. It clears colors, clears opaque depth or loads copied depth for a selected transparent item, then writes nearest surviving geometry. Alpha cutoff and zero transparent alpha discard fragments. Opaque alpha becomes one; material scalar/UV/scales/channels/inversions/native color decoding use M01-09. Normal mapping uses the PNUUT tangent and inverse-transpose world normal. Standard meshes have a vec3 tangent; no new vec4 handedness attribute is invented.

Deferred contract `draw.raster.deferred.v1` samples the seven G-buffer images plus internal HDR background and writes distinct internal RGBA16 float `color`. Opaque G-buffer depth is copied to preserved depth. HDR A is cleared, then the sky loads A/preserved depth without writing depth. Opaque lighting samples A and writes B. Both transparent classes merge in stable far-to-near submesh order. For each submesh, working depth receives the preserved opaque depth, colors clear, the item renders to G-buffer, and a fullscreen deferred composition alternates HDR A/B. A cleared normal marks absent coverage even when copied opaque depth is finite. Hidden/occluded glass therefore returns the existing background.

This is deferred sorted-submesh transparency with nearest surface per submesh, matching the existing resource granularity; it is not per-triangle order-independent transparency. Geometry is never shaded by a forward workaround. Per-transparent-item fullscreen work is an explicit correctness-first cost. Do not combine multiple sorted items into one nearest-surface G-buffer and silently lose layers.

Lighting implements GGX direct BRDF for up to eight captured omni/directional/spot lights, degree spot angles/cutoffs, Fresnel tint, sheen, AO, emission, unlit, and screen-space refraction of the already-composited HDR background. Irradiance includes PI and is multiplied by albedo/PI; prefilter LOD is roughness * maxLOD, BRDF RG=A/B. AO is an ordinary scalar and does not activate baked-map semantics. No query shadow/AO/GI/reflection/history/denoise objects are allocated. Material flags remain M01-09; the baked channel remains white.

Final contract `draw.raster.final.v1` samples HDR color followed by all seven G-buffer images and writes `output` at output extent. All inputs are sampled internal images, preserving exact formats including configured depth. FinalLayer.settings uses a 48-byte native push block: color vec4 at 0 (exposure stops, contrast, brightness, positive gamma), outputExtent vec4 at 16, display uint4 at 32. Exposure uses 1-exp(-linear * exp2(stops)); gamma follows, then brightness before contrast. These operations occur only once after HDR alpha composition. Native sRGB attachment encoding is compensated to match UNORM display values. Both extents can differ without blit-format filtering or FSR; four nearest samples implement bilinear reconstruction.

Only implemented debug selections are exposed: display.x 0 final, 1 albedo, 2 normal/emission, 3 material, 4 Fresnel/flags, 5 sheen/refraction, 6 baked white, 7 depth. Debug displays the last processed G-buffer (opaque when no transparent items), with depth replicated to RGB. FinalSettings display.y/outputExtent are resolved by the pass. An injected final pass need not derive FinalLayer; renderer recording does not access its settings. finalSettings() explicitly throws if the selected pass has its own API. Reflected catalogs and stable serialization properties remain M01-12.

## Native binding table

G-buffer retains the M01-09 drawBindingLayout ABI: frame set0 Vulkan0/1 Metalbuffers2/6, object set1 Vulkan0/1 Metalbuffers3/7, material image/sampler pairs Vulkan2..7/8..13 Metaltextures/samplers0..5. Only frame/object sets are consumed by this stage.

Fullscreen deferred uses its own public deferredBindingLayout:

| Data | Vulkan set / bindings | Metal indices |
|---|---|---|
| frame stage aliases | 0 / 0 vertex, 1 fragment | buffers2 vertex, 6 fragment |
| G-buffer/background images | 1 / 0..7 | textures0..7 |
| G-buffer/background samplers | 1 / 8..15 | samplers0..7 |
| lights | 2 / 0 | fragment buffer4 |
| sky/irradiance/prefilter/BRDF | 3 / 0..3 | textures8..11 |
| environment samplers | 3 / 4..7 | samplers8..11 |
| environment parameters | 3 / 8 | fragment buffer5 |

EnvironmentDataBinding adds optional metalTextureBase (default6 preserves M01-09); the deferred renderer passes8. Do not assume drawBindingLayout's material and fullscreen indices match. Frame/light structs are unchanged. Final uses set0 images0..7/samplers8..15, native textures/samplers0..7; fragment push Metalbuffer0. Metal uses depth2d<float> for depth inputs; Vulkan uses sampled depth images. Concrete paired libraries live under draw/raster; shared draw/lib PBR math is reused. Existing recursive compilation/deployment discovers these sources without CMake edits.

## Required supporting primitive/integration

M01-03 exact copy initially covered color only. M01-10 extends CommandBuffer::validateCopy to D16/D32 depth-only images with exact matching formats/extents, transfer usages/layouts, one selected mip/layer and distinct subresources. Vulkan selects DEPTH aspect; Metal's existing same-format texture blit copies these formats. Combined depth-stencil copies, depth upload/readback and scaled depth blits remain unsupported. Backend implementation stays in gpu. RenderLoop's retained scene color now includes Sampled usage, allowing final pass validation/ShaderReadOnly before presentation copy.

## Lead verification and successor work

[Manual fixture/checks](../../../../tests/draw/deferred/README.md) include an application-owned compatible GBuffer subclass, an unlit opaque/occluded-glass/front-glass scene, output readback expectation, slots/resize/replacement/environment/depth-copy/native shader cases and no-RT operation. The agent performs only static inspection and whitespace/include/ABI/link checks; no configuration, build, shader compilation or runtime execution.

M01-11 extends actual deferred native lighting and query opacity support. M01-12 reflects live final/pass/renderer settings. M01-13 introduces upscaler injection/scale policy and minimal FSR motion inputs while preserving this ordinary reconstruction fallback. M01-15 supplies the full Application example. M02 picking adds its own ID/depth readback surface and per-submission map; current debug depth is the last layer, not a picking result. M04 injects actual compatible baked multipliers into these public stages. Public scene Update/Input/Transform and all production render implementations remain operational references.

## M01-11 extension

[Query shadows](query-shadows.md) adds an injectable compute SoftShadowPass, optional slot masks and the compatible deferred.shadows.v1 lighting variant. The unshadowed contract remains available and explicit application injection opts that graph out. FullscreenPass now optionally maps Metal input indices to keep masks12/13 distinct from environment8..11. No final color/debug/postprocessing behavior changes. The lead's irradianceImage rename and CommandBuffer include corrections were reviewed and preserved.
