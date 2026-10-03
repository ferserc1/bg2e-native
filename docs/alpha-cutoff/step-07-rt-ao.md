# Step 07 — Alpha-tested occlusion rays in RTAO

## Goal

The RTAO compute pass (`rt_ao.comp.glsl`) traces occlusion rays with
`queryAO()`, which today uses `gl_RayFlagsOpaqueEXT` and has no access to
material data. Bind `RTMaterialDataBinding` to the pass and switch to the
alpha-tested `queryAOCutout()` so foliage/fence geometry occludes only
through its surviving texels.

Depends on: steps 02, 04.

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp` | Modify |
| `lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp` | Modify |
| `shaders/src/glsl/rt_ao.comp.glsl` | Modify |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Modify (one line) |

## 7.1 `RTAmbientOcclusion.hpp`

Add the binding pointer and setter (pattern copied from `RTReflections`):

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>

class BG2E_API RTAmbientOcclusion {
public:
    ...
    void setBlueNoise(const BlueNoise* blueNoise) { _blueNoise = blueNoise; }
    void setMaterialDataBinding(vulkan::rt::RTMaterialDataBinding* binding) { _materialDataBinding = binding; }
    ...
private:
    ...
    const BlueNoise* _blueNoise = nullptr;
    vulkan::rt::RTMaterialDataBinding* _materialDataBinding = nullptr;
```

(Verify the exact `_blueNoise` member declaration context and add the new
member next to it.)

## 7.2 `RTAmbientOcclusion.cpp`

### `createPipeline()` (line ~113)

Add a second descriptor set (set 1) using the shared material binding layout:

```cpp
vulkan::factory::PipelineLayout layoutFactory(_engine);
layoutFactory.addDescriptorSetLayout(_dsLayout);                        // set 0
if (_materialDataBinding)
{
    layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout());  // set 1 (cached, includes COMPUTE stage)
}
layoutFactory.addPushConstantRange(0, sizeof(AOPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
_pipelineLayout = layoutFactory.build("RTAmbientOcclusion::PipelineLayout");
```

The layout is the cached shared one created in `DeferredLayer::build()` with
`COMPUTE` in its stage mask (step 05).

### `render()` (line ~149)

The object instances are reachable through `frameResources.rayTracingScene`
(already dereferenced for the TLAS). After binding set 0:

```cpp
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
VkDescriptorSet dsHandle = ds->descriptorSet();
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

if (_materialDataBinding)
{
    const auto& objectInstances = frameResources.rayTracingScene->objectInstances();
    auto matDS = _materialDataBinding->newDescriptorSet(frameResources, objectInstances);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 1, 1, &matDS, 0, nullptr);
}
```

## 7.3 `DeferredLayer.cpp`

In `DeferredLayer::build()`, right after the AO pass creation (line ~107):

```cpp
_rtAmbientOcclusion = std::make_unique<RTAmbientOcclusion>(_engine);
_rtAmbientOcclusion->build(extent);
```

The `_rtMaterialDataBinding` is created a few lines later (inside the RT
block). Move/inject after both exist:

```cpp
if (_engine->rayTracingSupported())
{
    _rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);
    _rtMaterialDataBinding->createLayout(...);   // step 05
    _rtAmbientOcclusion->setMaterialDataBinding(_rtMaterialDataBinding.get());
    ...
}
```

(`RTAmbientOcclusion::build()` currently creates the pipeline eagerly; if the
pipeline is built before `setMaterialDataBinding()` is called, either move
the setter before `build()` or create the pipeline layout unconditionally
with the material layout when RT is supported. Prefer: call
`setMaterialDataBinding()` **before** `_rtAmbientOcclusion->build(extent)` —
i.e. construct `_rtMaterialDataBinding` first, then the AO pass.)

## 7.4 `rt_ao.comp.glsl`

Add the include (set 1 is free in this shader) after the existing includes:

```glsl
#include "lib/deferred_utils.glsl"
#include "lib/ray_tracing.glsl"
#include "lib/blue_noise.glsl"

#define RT_MATERIAL_SET 1
#include "lib/rt_material_data.glsl"
```

Switch the occlusion query (line ~103):

```glsl
float hitDistance;
if (queryAOCutout(tlas, origin, bounceNormal, rayDir, pc.radius, pc.bias, hitDistance))
{
    ...
}
```

No push constant changes are needed.

## Behavior notes

- The AO pass runs on the opaque layer's G-buffer (and optionally on the
  transparent layer); the rays only see TLAS geometry (opaque cutout
  materials), consistent with the rest of the RT path.
- The fallback paths are untouched: RT unsupported → white AO image; null
  TLAS → output cleared to white (`RTAmbientOcclusion.cpp:63-81,163-175`).
  The material descriptor set is only bound when a valid TLAS exists, which
  is the same branch that already uses `frameResources.rayTracingScene`.
- Blue noise, bounce loop, temporal accumulation and denoising are
  unaffected.

## Verification

- Alpha-textured foliage above a surface: RTAO contact shadows must respect
  the leaf holes (light leaks through the holes), instead of a solid quad
  shadow.
- `RTAmbientOcclusion` / `DenoisedAO` debug views show the corrected
  occlusion pattern.
- Opaque-only scenes: AO output must match the previous implementation
  (first candidate always commits when `alphaCutoff <= 0`).
