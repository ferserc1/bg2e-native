# Ray tracing effects, scene resources, and hardware fallback

[Overview](index.md) · [GPU prerequisites](06-gpu-primitives-and-readiness.md) · [Baking](11-offscreen-and-baking.md)

## 1. Production tracing architecture

The production engine creates a tracing-scene wrapper for each frame resource. Base renderer preparation calls its update, which returns immediately when hardware RT is unsupported. Drawable owns per-submesh BLAS resources. A graph visitor collects transformed instances and parallel material/geometry records; the frame tracing object builds its TLAS and exposes it to effects.

Sources: [`RayTracingScene.cpp`](../../lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp), [`CollectRayTracingInstancesVisitor.cpp`](../../lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp), [`RTMaterialData.h`](../../lib/include/bg2e/render/vulkan/rt/RTMaterialData.h), [`RTMaterialDataBinding.cpp`](../../lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp).

The collector respects disabled nodes/submesh visibility, composes graph and drawable/submesh transforms, checks traceability, excludes transparent materials, and limits objects to the engine's configured maximum, currently 256. Instance custom indices address the parallel object table. That mapping is a correctness invariant, not a incidental order.

**Proposed extraction:** `draw` owns a tracing scene snapshot/material table service fed by the same scene extraction as raster rendering. `gpu` owns acceleration-structure construction and primitive instance transforms/masks/IDs. BLAS may be cached by geometry revision; TLAS/material tables are view/scene execution resources with frame-safe lifetime.

The inspected production update recreates instance/TLAS/scratch storage and records a BUILD operation. Experimental `gpu::RayTracingScene` instead documents capacity-based resource reuse. That primitive reuse is helpful, but high-level dirty tracking for transform, geometry, visibility and material changes still belongs in `draw`; it should not be inferred from a method named `buildOrUpdate` alone.

## 2. Hardware and feature negotiation

Production largely uses a single `rayTracingSupported()` decision. Experimental GPU exposes a capability record with pipeline, query, acceleration and address flags. Preserve compatibility with devices that lack RT by building the raster renderer independently of optional tracing resources.

| Capability set | Required experimental behavior |
|---|---|
| No RT | Raster PBR/IBL/materials/transparency/baked lighting; Direct output and non-RT tools. No BLAS/TLAS/effect allocation requirement. |
| Acceleration/query path usable | Potential AO/shadow support when matching shader/material path is implemented. Do not imply full GI/reflection support from queries alone. |
| Tracing pipeline/equivalent path usable | Potential GI/reflections and baking, conditional on shader/binding/cutout support. |
| FSR available | Optional final processing independent of RT capability. |
| SDK/shader/effect initialization fails | Keep raster renderer operational; report effective disabled/fallback feature state. |

Partial RT support is a proposed capability refinement beyond production's mostly aggregate gate. A first implementation may conservatively gate all RT together, provided the no-RT baseline remains fully usable and the API can later expose finer support.

Do not emulate unavailable RT invisibly on CPU. Baking that inherently requires RT can report unsupported while camera rendering continues.

## 3. RTAO

[`RTAmbientOcclusion`](../../lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp) runs compute ray queries using G-buffer depth/normal reconstruction and material-aware cutout tests. It exposes quality, sample/bounce counts, radius, bias, falloff, bounce attenuation and blue-noise choice. It owns per-frame raw AO resources and also supports a UV-space bake path.

The layer feeds raw AO into scalar temporal accumulation and bilateral denoise before composition. Disabling live indirect lighting does not disable baked material multipliers or IBL. AO quality scales effect resolution independently of final output.

Preserve signal semantics: neutral AO is 1, not 0; AO is not HDR radiance. Its history format and filter must not accidentally use GI energy logic. A reusable pass can share query algorithms between camera and UV-space inputs while keeping their resource/lifecycle contracts separate.

Without RT, production's non-RT shader does not run live AO. Material AO/baked maps remain relevant. A new SSAO alternative would be extra scope.

## 4. RTGI

[`RTGlobalIllumination`](../../lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp) uses raygen/miss/closest-hit and any-hit production shaders. It consumes G-buffer reconstructed positions, camera, TLAS, object/material tables, irradiance environment and the reduced light set. It exposes enable/quality, samples, bounces, ray bias, maximum distance, shadow samples and blue noise, plus a UV-space tracing variant.

The deferred layer has an `IndirectLightingMode` choosing RTAO or RTGI. They are not simply two always-on additive effects. GI uses HDR temporal accumulation and HDR bilateral denoise. Preserve the relationship between GI, baked AO, IBL and direct lighting so the new composition does not double-count energy or apply an AO scalar to the wrong signal.

The existing hit-material record is narrower than the full raster material: albedo/scale, index offset, emission channel/inversion/scale/UV and alpha cutoff, with corresponding textures and geometry buffers. Preserve that actual baseline; full secondary-hit parity with every raster PBR channel is a possible improvement, not an already implemented capability.

## 5. RT reflections

[`RTReflections`](../../lib/include/bg2e/render/deferred/RTReflections.hpp) similarly traces with material hit shading, environment/light inputs and cutout handling. Settings include sample count, max roughness, ray bias/distance, roughness spread, shadow samples, quality and blue noise. Output includes radiance and a mask/confidence signal used by lighting composition and debug views.

Quality determines reflection extent relative to **display viewport size**. It is not enough to resize reflections whenever G-buffer extent changes; a display-only change may still alter effect resolution. Reflection history uses HDR accumulation independently of AO/GI history. Relevant setting changes reset it in production.

When no supported result is available, preserve IBL specular/environment behavior and neutral reflection fallback. Do not substitute black radiance blindly for a confidence-weighted missing hit and accidentally remove environment reflections.

## 6. Soft shadows

Direct-light composition's RT shader calls cutout-aware shadow queries. Neutral `base::Light` contains castShadows, sourceSize and shadowSamples. Secondary GI/reflection lighting has its own shadow sample controls and a light set controlled by `affectsReflections`.

This is shader-integrated shadow evaluation, not currently a dedicated `SoftShadowRenderer` class. In `draw`, it may stay as a lighting-kernel operation, or become an explicit shadow pass if that simplifies Metal portability/performance. GPU should only expose the necessary query/tracing primitives.

Without RT, lights contribute without these shadow queries. Production has no equivalent shadow-map fallback in the inspected deferred path. Adding shadow maps could improve unsupported devices, but must be costed and reviewed independently.

## 7. Alpha-cutout parity is a blocker

Production uses two intersection approaches:

| Path | Current production implementation |
|---|---|
| AO/direct shadows and query-based secondary tests | Iterate candidate triangles, reconstruct UV/material alpha and explicitly confirm accepted intersections. |
| GI/reflection traced pipeline | Any-hit shader `rt_alpha_test.rahit.glsl` rejects below-cutoff intersections. |

Both need non-opaque geometry and consistent instance material indexing. The common material GLSL helper indexes vertex/index/texture arrays, reconstructs triangle barycentric UV0 for albedo and multiplies sampled alpha by material albedo alpha. The cutoff is shared with raster material data, but exact texture/UV behavior must be checked rather than assuming arbitrary per-channel UV parity on traced hits.

Experimental Vulkan currently marks geometry opaque and its RT pipeline description lacks any-hit. Therefore a simple port would cast solid shadows/reflections/GI from cutout holes even if raster alpha discard looks correct. This is a real primitive-contract gap plus draw shader/material work, not merely a missing effect toggle.

Metal's RT pipeline wrapper executes a compute raygen kernel and ignores separate miss/closest-hit modules. Equivalent cutout handling needs candidate/intersection logic in a Metal kernel or a reviewed generic shader/intersection approach. A plain closest intersection of opaque geometry is insufficient. No production-equivalent Metal cutout implementation is asserted by this assessment.

## 8. Scene tables and addressing

Current tables have a maximum of 256 objects. Per-object vertex/index buffer descriptor arrays plus albedo/emission texture arrays are indexed by custom instance IDs using nonuniform access. CPU RTMaterialData has a 56-byte scalar-layout assertion.

Potential experimental approaches:

1. Extend generic resource arrays and preserve the table layout for initial Vulkan parity; implement corresponding Metal resource addressing.
2. Pack geometry/material records into larger shared storage buffers and use offsets; manage textures through supported arrays/atlases or backend resource mechanisms.
3. Separate a backend-specific tracing resource table behind a draw-private service, while maintaining one logical CPU scene snapshot and result contract.

Option 1 is least algorithmic change but needs low-level binding work. Option 2 can improve scaling but changes more shader/addressing logic. Option 3 contains differences, but should not let duplicated material policy drift. The choice must be made before estimating RT ports.

Do not remove the object/light limits incidentally. Deferred and reflection-light limits are currently eight. Report capacity overflow and desired future scaling explicitly; larger scenes are an optional capability change, not automatically guaranteed by replacing TLAS wrappers.

## 9. Histories and synchronization

Each layer/effect/view needs separate histories, previous depth/normals and camera/projection identity. An output image indexed by frame slot is not itself a temporal chronology. Paused scenes, presentation-only UI frames, dropped acquisition, resize and multiple offscreen views make this distinction important.

Geometry/material edits can invalidate BLAS, instance tables and effect histories differently. Transform edits update TLAS; topology/buffer changes require BLAS rebuild; material cutout changes affect intersection behavior and history even when geometry buffers are unchanged. Publish one coherent snapshot before recording.

Retain resource sets, geometry buffers, BLAS/TLAS and textures until every submitted command using them completes. Avoid mutating persistent resource sets still in flight; update the protected slot or replace/defer the resource. `gpu` provides completion mechanisms; `draw` must assign the correct lifetimes and invalidation rules.

## 10. Backend rollout options

A Vulkan-first experimental deferred path can establish parity with the closest production implementation, followed by Metal ports. A backend-neutral design with raster parity on both first reduces architectural surprises. Either is reasonable; the review must decide whether “complete” means Vulkan parity or both backends.

In all cases no-RT raster support is an independent release gate, not postponed until the last effect works. Cross-backend visual equivalence needs tolerances because different shader execution/intersection implementations need not produce identical stochastic samples.
