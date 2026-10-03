# Step 05 — Alpha-tested shadow rays

## Goal

Make every shadow ray in the engine's deferred path respect the alpha test:

1. **RT composite** (`deferred_composite_rt.frag.glsl`) — the per-light shadow
   rays traced inline with `rayQueryEXT` must see through cutout holes.
2. **GI / reflection hit shaders** (`rt_gi.rchit.glsl`,
   `rt_reflections.rchit.glsl`) — the `queryShadow()` calls used for direct
   lighting of bounce/reflection hits must do the same.

Depends on: steps 02 (RTMaterialData.alphaCutoff) and 04 (helpers, bindings,
BLAS change).

## Files

| File | Action |
|---|---|
| `shaders/src/glsl/deferred_composite_rt.frag.glsl` | Modify |
| `shaders/src/glsl/rt_gi.rchit.glsl` | Modify |
| `shaders/src/glsl/rt_reflections.rchit.glsl` | Modify |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Modify |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | Modify (comment) |

## 5.1 `DeferredLayer` — shared material binding layout and composite RT set 5

### a) Create the material binding layout with all consumer stages

In `DeferredLayer::build()`, immediately after
`_rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);`
(line ~124), force layout creation with the full stage mask. The layout is
cached inside the binding, so every later `createLayout()` call (GI,
reflections pipelines) reuses it:

```cpp
_rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);
// The same layout instance is shared by the GI/reflections pipelines
// (CLOSEST_HIT | ANY_HIT), the RT composite (FRAGMENT) and the AO compute
// pass (COMPUTE). Create it once with the union of all stages; subsequent
// createLayout() calls return the cached layout.
_rtMaterialDataBinding->createLayout(
    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
    VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
    VK_SHADER_STAGE_FRAGMENT_BIT |
    VK_SHADER_STAGE_COMPUTE_BIT
);
```

### b) Composite RT pipeline layout — `createCompositePipelineRT()` (line ~998)

Add the material binding as **set 5**:

```cpp
vulkan::factory::PipelineLayout layoutFactory(_engine);
layoutFactory.addDescriptorSetLayout(_compositeGBufferRTDSLayout);      // set 0
layoutFactory.addDescriptorSetLayout(_fragmentFrameDataBinding->createLayout(VK_SHADER_STAGE_FRAGMENT_BIT)); // set 1
layoutFactory.addDescriptorSetLayout(_environmentDataBinding->createLayout());  // set 2
layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout());        // set 3
layoutFactory.addDescriptorSetLayout(_rtDataBinding->createLayout());           // set 4
layoutFactory.addDescriptorSetLayout(_rtMaterialDataBinding->createLayout());   // set 5 (cached)
```

### c) Bind the material descriptor set — `renderCompositePass()` (line ~1160)

The pass already dereferences `frameResources.rayTracingScene` for the TLAS.
Inside the `if (useRT)` block, after binding the RT scene descriptor set:

```cpp
if (useRT)
{
    auto rtDS = _rtDataBinding->newDescriptorSet(frameResources, tlas);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 4, 1, &rtDS, 0, nullptr);

    const auto& objectInstances = frameResources.rayTracingScene->objectInstances();
    auto matDS = _rtMaterialDataBinding->newDescriptorSet(frameResources, objectInstances);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 5, 1, &matDS, 0, nullptr);
}
```

Note: the transparent layer also runs `renderCompositePass()` with the RT
pipeline; `_rtMaterialDataBinding` exists on both layers (created in
`build()` when RT is supported), so no extra plumbing is needed.

## 5.2 `deferred_composite_rt.frag.glsl`

At the top of the file, before including the material header, place the
bindings on set 5 (sets 0-4 are taken):

```glsl
#version 460
...
#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"
#include "lib/color_correction.glsl"
#include "lib/deferred_utils.glsl"
#include "lib/ray_tracing.glsl"

#define RT_MATERIAL_SET 5
#include "lib/rt_material_data.glsl"
```

Include-order note: `rt_material_data.glsl` needs `rayQueryEXT` (already
enabled at the top of this shader) and `LightData`/`MAX_RT_OBJECTS` (from
`uniforms.glsl` / its own header). The cutout helpers also reuse
`_shadowHash`/`_shadowRandomDirection` from `ray_tracing.glsl`, hence the
include order above.

Shadow loop (line ~213-219): switch to the cutout variant:

```glsl
float shadowFactor = 1.0;
if (LightsBuffer.lights[i].castShadows != 0) {
    shadowFactor = queryShadowCutout(tlas, gbuf.worldPos, gbuf.normal, LightsBuffer.lights[i], 32);
}
```

This shader may also use `hardShadow()` — migrate any such call to
`hardShadowCutout()` the same way.

## 5.3 `rt_gi.rchit.glsl`

After step 04 the material bindings come from the header (set 1, default).
Change the shadow query in the light loop (line ~117):

```glsl
if (light.castShadows != 0) {
    shadowFactor = queryShadowCutout(tlas, worldPos, worldNormal, light, int(pc.shadowSamples));
}
```

## 5.4 `rt_reflections.rchit.glsl`

Same change in its light loop (line ~115):

```glsl
if (light.castShadows != 0)
{
    shadowFactor = queryShadowCutout(tlas, worldPos, worldNormal, light, int(pc.shadowSamples));
}
```

## What does NOT change

- `basic_forward_rt_shadows.frag.glsl` (forward renderer): keeps calling the
  opaque `queryShadow()`. Its rays use `gl_RayFlagsOpaqueEXT`, which commits
  the first hit immediately without any-hit/candidate handling — identical
  behavior to today even though the BLAS no longer carries the opaque
  geometry bit.
- The non-RT composite (`deferred_composite.frag.glsl`): no ray tracing at
  all; cutout was already resolved by the G-buffer discard (step 03).
- Transparent materials: not present in the TLAS, so they neither cast nor
  receive alpha-tested shadow rays (unchanged from today).

## Integration points

- The material SSBO/textures are rebuilt per frame from
  `frameResources.rayTracingScene->objectInstances()` — the same vector the
  GI/reflections passes already use (`DeferredLayer.cpp:371-416`), populated
  by `RayTracingScene::update()` from the visitor (with `alphaCutoff` filled
  in step 02).
- `_rtMaterialDataBinding->initFrameResources()` is already called in
  `DeferredLayer::initFrameResources()` (line ~232).

## Verification

- Alpha-textured occluder (e.g. a fence or foliage plane) between a light and
  a surface: the shadow must show the cutout pattern, for both hard
  (`shadowSamples <= 1`) and soft shadows.
- Same check for the direct lighting visible in RT reflections and in GI
  bounces (debug views `RTReflections` / `RTGlobalIllumination`).
- Scenes without any alpha-tested material must produce pixel-identical
  shadows to before (candidate loop still commits the first candidate when
  `alphaCutoff <= 0`).
