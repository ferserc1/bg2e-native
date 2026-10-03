# alphaCutoff — Alpha Test Support Implementation Plan

## Problem statement

The engine has no alpha testing (alpha cutout) support. Materials with
alpha-textured foliage, fences, decals, etc. cannot discard fragments below an
opacity threshold:

- `bg2e::base::MaterialAttributes` has no `alphaCutoff` property.
- The glTF loader explicitly does not support `alphaMode: MASK`
  (`lib/src/bg2e/db/scene_gltf.cpp:293-301`), even though cgltf already parses
  `alpha_cutoff` (default 0.5).
- bg2-scene (third-party `.bg2` format) already defines
  `Bg2Material::alphaCutoff = 0.5f`, but the engine never maps it.
- No shader performs `discard`.
- The whole ray tracing path assumes fully opaque geometry:
  - BLAS geometry is always built with `VK_GEOMETRY_OPAQUE_BIT_KHR`
    (`RTAccelerationStructureSize.cpp:80`).
  - Every ray uses `gl_RayFlagsOpaqueEXT` (inline ray queries in
    `lib/ray_tracing.glsl`, `traceRayEXT` in both `.rgen` shaders), so any-hit
    processing is impossible.
  - No any-hit shader exists and `factory::RayTracingPipeline` has no any-hit
    support.
  - The shadow ray queries (`deferred_composite_rt.frag.glsl`,
    `rt_ao.comp.glsl`) have no access to material data, so they cannot resolve
    per-texel opacity.

Goal: add a material property `alphaCutoff` (threshold for alpha testing),
serialized with that exact name for backward compatibility, applied in the
deferred renderer (rasterization + all ray traced effects: shadows,
reflections, GI, AO) and in both opaque and transparent layers. The forward
renderer (`RendererBasicForward`) is intentionally left unchanged (it is
scheduled for removal).

## Decisions (agreed with the user)

| Decision | Value |
|---|---|
| Default value | `0.5` (matches glTF / bg2-scene defaults) |
| Serialization name | `"alphaCutoff"` (JSON, backward compatible) |
| G-buffer storage | No new channel: `discard` in the G-buffer fragment shader + material flag bit 1 in the fresnel+flags alpha bitfield |
| Transparent materials | Alpha test **also applies** to transparent materials |
| BLAS opaque bit | Removed **globally** (BLAS are shared between materials; per-material flags are impossible) |
| Ray flags optimization | **Discarded** — scene-level `gl_RayFlagsOpaqueEXT` switching is pointless because virtually every scene contains alpha-tested elements |
| RT scope | Full: shadows (composite + hit shaders), reflections, GI, AO |
| glTF loader + MaterialEditor UI | Included |

## Proposed solution

### Rasterization path

The alpha test is resolved in the G-buffer fill fragment shader
(`deferred_gbuffer.frag.glsl`) with `discard`. Discarded fragments write
nothing (neither color nor depth), so the composite passes and all downstream
consumers (motion vectors, debug views) need no changes. No G-buffer channel
is consumed: sheen alpha already carries `refractionFactor` and normal alpha
carries light emission. Bit 1 of the existing material-flags bitfield
(fresnel+flags alpha) records that the material is alpha-tested, and bit 2
records transparency (used to decide whether the surviving albedo alpha is
forced to 1.0).

Cutout **opaque** materials write `g_Albedo.a = 1.0` after passing the test,
so the composite's `mix(inputColor, outColor, albedoAlpha)` blends fully.
Cutout **transparent** materials keep their sampled alpha so blending still
works.

### Ray tracing path

```
                         ┌────────────────────────────────────────┐
                         │   BLAS built WITHOUT OPAQUE bit        │
                         │   (all hits arrive as candidates)      │
                         └────────────────────────────────────────┘
                                          │
        ┌─────────────────────────────────┼──────────────────────────────────┐
        │                                 │                                  │
 traceRayEXT (primary rays)      rayQueryEXT (shadow/AO rays)        forward RT shadows
 rt_gi.rgen / rt_reflections.rgen composite_rt / rchits / rt_ao      (unchanged)
        │                                 │                          gl_RayFlagsOpaqueEXT
        ▼                                 ▼                          commits immediately
 NO OPAQUE ray flag ──► any-hit    manual candidate loop:
 rt_alpha_test.rahit               while (rayQueryProceedEXT)
 alpha test +                      if candidate && alphaPass(rq)
 ignoreIntersectionEXT                rayQueryConfirmIntersectionEXT
```

Both mechanisms share the same per-material data and helper functions:
`RTMaterialDataBinding` (materials SSBO + vertex/index buffer arrays + albedo
texture array) is extended to the shaders that lack it today:

| Shader | Material binding | Mechanism |
|---|---|---|
| `rt_gi.rchit` / `rt_reflections.rchit` | set 1 (already exists) | alpha-test variants of `queryShadow()` |
| `rt_gi.rgen` / `rt_reflections.rgen` | — (primary rays) | shared `rt_alpha_test.rahit.glsl` any-hit shader; drop `gl_RayFlagsOpaqueEXT` |
| `deferred_composite_rt.frag` | **new set 5** | alpha-test variant of `queryShadow()` |
| `rt_ao.comp` | **new set 1** | alpha-test variant of `queryAO()` |

The GLSL material bindings move into `lib/rt_material_data.glsl` behind a
`RT_MATERIAL_SET` macro so each shader can place them on a different set.
New helpers in that header compute the barycentric UV of a candidate/primitive
hit and evaluate `texture(albedoTex[i], uv).a * mat.albedo.a >= mat.alphaCutoff`.

Transparent materials are excluded from the TLAS by
`CollectRayTracingInstancesVisitor` (they are not ray traced at all), so in RT
the alpha test only covers opaque cutout materials — exactly the ones that
produce wrong hard shadows today.

## Files to create/modify

| File | Action | Description |
|---|---|---|
| `lib/include/bg2e/base/MaterialAttributes.hpp` | Modify | `_alphaCutoff = 0.5f` + getter/setter |
| `lib/src/bg2e/utils/MaterialSerializer.cpp` | Modify | Parse + serialize `"alphaCutoff"` |
| `lib/src/bg2e/db/scene_gltf.cpp` | Modify | Map `alphaMode MASK` → `alphaCutoff` |
| `lib/src/bg2e/ui/MaterialEditor.cpp` | Modify | Alpha cutoff slider |
| `lib/include/bg2e/render/uniforms/materials.hpp` | Modify | `padding` → `alphaCutoff`; new flag bits; `operator=` |
| `shaders/src/glsl/lib/uniforms.glsl` | Modify | GLSL mirror + flag constants |
| `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h` | Modify | `padding` → `alphaCutoff` |
| `shaders/src/glsl/lib/rt_material_data.glsl` | Modify | Struct mirror + shared bindings + alpha-test helpers |
| `lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp` | Modify | Populate `materialData.alphaCutoff` |
| `shaders/src/glsl/deferred_gbuffer.frag.glsl` | Modify | `discard` + alpha write + flags packing |
| `shaders/src/glsl/lib/deferred_utils.glsl` | Modify | Decode `alphaTest` flag |
| `lib/src/bg2e/render/vulkan/rt/RTAccelerationStructureSize.cpp` | Modify | Remove `VK_GEOMETRY_OPAQUE_BIT_KHR` |
| `lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp` | Modify | `setAnyHitShader()` API |
| `lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp` | Modify | Any-hit stage + triangle hit group |
| `shaders/src/glsl/rt_alpha_test.rahit.glsl` | **Create** | Shared any-hit alpha test shader |
| `lib/src/bg2e/render/deferred/RTReflections.cpp` | Modify | Wire any-hit + stage flags |
| `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp` | Modify | Wire any-hit + stage flags |
| `shaders/src/glsl/rt_gi.rgen.glsl` | Modify | Drop `gl_RayFlagsOpaqueEXT` |
| `shaders/src/glsl/rt_reflections.rgen.glsl` | Modify | Drop `gl_RayFlagsOpaqueEXT` |
| `shaders/src/glsl/rt_gi.rchit.glsl` | Modify | Bindings via header + cutout `queryShadow` |
| `shaders/src/glsl/rt_reflections.rchit.glsl` | Modify | Bindings via header + cutout `queryShadow` |
| `shaders/src/glsl/deferred_composite_rt.frag.glsl` | Modify | Set 5 materials + cutout `queryShadow` |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | Modify | (minor) comment updates |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Modify | Material binding stages + set 5 in composite RT + AO injection |
| `lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp` | Modify | `setMaterialDataBinding()` |
| `lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp` | Modify | Bind material DS (set 1) |
| `shaders/src/glsl/rt_ao.comp.glsl` | Modify | Materials set 1 + cutout `queryAO` |
| `doc/deferred_render_model.md` | Modify | Document the new behavior |

## Step files

1. [step-01-material-property-serialization.md](step-01-material-property-serialization.md) — `MaterialAttributes`, JSON serializer, glTF loader, MaterialEditor slider
2. [step-02-uniform-buffers.md](step-02-uniform-buffers.md) — `PBRMaterialData` and `RTMaterialData` (C++ and GLSL), visitor population
3. [step-03-gbuffer-alpha-test.md](step-03-gbuffer-alpha-test.md) — discard in the G-buffer pass, flags bitfield
4. [step-04-rt-infrastructure.md](step-04-rt-infrastructure.md) — BLAS opaque bit removal, `RayTracingPipeline` any-hit support, `rt_material_data.glsl` restructure, shared `.rahit`
5. [step-05-rt-shadows.md](step-05-rt-shadows.md) — alpha-tested shadow rays in the RT composite and hit shaders
6. [step-06-rt-gi-reflections.md](step-06-rt-gi-reflections.md) — any-hit wiring for GI and reflection primary rays
7. [step-07-rt-ao.md](step-07-rt-ao.md) — alpha-tested occlusion rays in RTAO
8. [step-08-documentation.md](step-08-documentation.md) — update `doc/deferred_render_model.md`

## Implementation order and dependencies

```
step-01 ──┐
step-02 ──┼──► step-03 (rasterization, independently testable)
step-04 ──┴──► step-05 ──► step-06 ──► step-07 ──► step-08
```

Step 04 must land before (or together with) 05–07: removing the BLAS opaque
bit without the any-hit/candidate handling changes the behavior of every ray
query (hits become candidates that must be confirmed). Steps 05, 06, 07 are
independent of each other once 04 is in place.

## Notes

- **Struct layout safety**: `alphaCutoff` replaces an existing `uint32_t
  padding` in both uniform structs. Sizes are unchanged (PBRMaterialData keeps
  its std140 layout; RTMaterialData stays at 56 bytes with `scalar` layout).
- **Non-RT pipeline**: fully preserved. All RT changes are behind
  `_engine->rayTracingSupported()`; the discard happens in the G-buffer pass
  which is common to both modes; the non-RT composite needs nothing.
- **Forward renderer**: untouched. `basic_forward_rt_shadows.frag.glsl` keeps
  using the opaque `queryShadow()` variant with `gl_RayFlagsOpaqueEXT`, which
  commits hits immediately without any-hit even though the BLAS no longer has
  the opaque geometry bit (ray flags override geometry flags). The new field
  in the shared `uniforms.glsl` struct is simply unused there.
- **Behavior change warning**: with default `alphaCutoff = 0.5`, existing
  materials whose albedo alpha is < 0.5 will start discarding fragments,
  including BLEND-transparent materials with alpha gradients (clipped at 0.5).
  This is the semantics explicitly requested.
- **`bg2e::gpu`**: untouched (experimental namespace, out of scope).
- **Threading**: all changes are on the render thread; descriptor sets are
  per-frame via `frameResources`, no new cross-frame state is introduced.
- **Verification**: no test framework exists; use the examples (e.g. a scene
  with an alpha-textured plane) to check shadows, reflections, GI and AO.
