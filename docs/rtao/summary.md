# RTAmbientOcclusion — Implementation Plan Summary

## Objective

Create a `bg2e::render::deferred::RTAmbientOcclusion` class that generates an ambient occlusion image using a compute shader with ray queries against the TLAS. The result is a single-channel (`R8_UNORM`) image that gets multiplied into the material AO during the deferred composite pass.

## Architecture

```
RTAmbientOcclusion
├── Engine* _engine
├── vector<shared_ptr<Image>> _aoImages   (one per frame resource)
├── VkPipeline _pipeline                  (persistent compute pipeline)
├── VkPipelineLayout _pipelineLayout
├── VkDescriptorSetLayout _dsLayout
├── VkSampler _sampler
├── bool _rtSupported
├── build(extent)
├── resize(newExtent)
├── render(cmd, currentFrame, frameResources, gbuffer, inverseViewProjection)
├── cleanup()
├── aoImage(frameIndex) → shared_ptr<Image>
└── sampler() → VkSampler
```

## Push Constants Struct

```cpp
struct AOPushConstants {
    glm::mat4 inverseViewProjection;  // 64 bytes
    int sampleCount;                   // 4 bytes
    glm::vec3 padding;                 // 12 bytes
};  // Total: 80 bytes (multiple of 16)
```

## Compute Pipeline Layout

```
set=0:
  binding 0: COMBINED_IMAGE_SAMPLER    g_Normal
  binding 1: COMBINED_IMAGE_SAMPLER    g_Depth
  binding 2: ACCELERATION_STRUCTURE    tlas
  binding 3: STORAGE_IMAGE (r8)        aoOutput

push constants (80 bytes):
  mat4  inverseViewProjection
  int   sampleCount
  vec3  padding
```

## Composite Pipeline Layout (modified)

```
set=0:
  binding 0: COMBINED_IMAGE_SAMPLER  g_Albedo
  binding 1: COMBINED_IMAGE_SAMPLER  g_Normal
  binding 2: COMBINED_IMAGE_SAMPLER  g_Material
  binding 3: COMBINED_IMAGE_SAMPLER  g_FresnelFlags
  binding 4: COMBINED_IMAGE_SAMPLER  g_SheenColor
  binding 5: COMBINED_IMAGE_SAMPLER  g_InputImage
  binding 6: COMBINED_IMAGE_SAMPLER  g_Depth
  binding 7: COMBINED_IMAGE_SAMPLER  g_AO     ← NEW
```

## Execution Flow

```
DeferredLayer::render()
  ├─ renderGBufferPass()             → writes G-buffers, transitions to SHADER_READ_ONLY
  ├─ rtAmbientOcclusion.render()     → reads g_Normal + g_Depth + TLAS, writes AO image
  │   └─ AO image → SHADER_READ_ONLY
  └─ renderCompositePass()           → reads G-buffers + AO (binding 7), writes final color
```

## Fallback Behavior

| Scenario | `_rtSupported` | AO Image | `render()` |
|----------|---------------|----------|------------|
| No RT support | false | 4x4 white (shared) | no-op |
| RT, no TLAS | true | Full-size white (cleared each frame) | clears to white, returns |
| RT + TLAS | true | Full-size (compute output) | dispatches compute |

## Phases

| Phase | Description | Files |
|-------|-------------|-------|
| 1 | Compute shader | `shaders/src/rt_ao.comp.glsl` (create) |
| 2 | Class header | `lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp` (create) |
| 3 | Class implementation | `lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp` (create) |
| 4 | DeferredLayer header | `lib/include/bg2e/render/deferred/DeferredLayer.hpp` (modify) |
| 5 | DeferredLayer implementation | `lib/src/bg2e/render/deferred/DeferredLayer.cpp` (modify) |
| 6 | Composite shaders | `shaders/src/deferred_composite.frag.glsl` + `deferred_composite_rt.frag.glsl` (modify) |
