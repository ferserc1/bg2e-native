# Phase 5: DeferredLayer Implementation Modifications

## File

`lib/src/bg2e/render/deferred/DeferredLayer.cpp` (modify)

## Changes Overview

| Location | Method | Change Description |
|----------|--------|-------------------|
| 1 | `resolveDebugSource()` | Add `RTAmbientOcclusion` case |
| 2 | `build()` | Create RTAmbientOcclusion instance |
| 3 | `render()` | Call AO render after G-buffer pass |
| 4 | `resize()` | Resize AO images |
| 5 | `cleanup()` | Cleanup AO resources |
| 6 | `createCompositePipeline()` | Add binding 7 to G-buffer descriptor set layout |
| 7 | `renderCompositePass()` | Bind AO image to descriptor set |

---

### Change 1: `resolveDebugSource()` — Add RTAmbientOcclusion Case

**Location**: inside switch statement (after line 55, `InputImage` case)

**Add case**:
```cpp
case DeferredDebugVisualization::RTAmbientOcclusion:
    return _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex()).get();
```

**Context**:
```cpp
case DeferredDebugVisualization::InputImage:
    return inputImage;
case DeferredDebugVisualization::RTAmbientOcclusion:  // ← NEW
    return _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex()).get();
default:
    return gbuffer->image(0).get();
```

---

### Change 2: `build()` — Create RTAmbientOcclusion

**Location**: after GBuffer creation loop (after line 71), before data binding creation (line 73)

**Add**:
```cpp
_rtAmbientOcclusion = std::make_unique<RTAmbientOcclusion>(_engine);
_rtAmbientOcclusion->build(extent);
```

**Context**:
```cpp
    gb->build(extent);
}

// Create AO pass
_rtAmbientOcclusion = std::make_unique<RTAmbientOcclusion>(_engine);  // ← NEW
_rtAmbientOcclusion->build(extent);                                    // ← NEW

// Create per-layer data bindings
_frameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(_engine);
```

---

### Change 3: `render()` — Call AO Render

**Location**: after `renderGBufferPass()` call (after line 202), before the `if (_debugVisualization == ...)` block

**Replace lines 202-215** with:
```cpp
renderGBufferPass(cmd, currentFrame, gbuffer, frameResources, viewMatrix, projMatrix, cameraWorldPos);

// AO pass: compute ambient occlusion from G-buffers + TLAS
{
    auto projMat = _scene->mainCamera()->projectionMatrix();
    auto viewMat = _scene->mainCamera()->viewMatrix();
    auto invVP = glm::inverse(projMat * viewMat);
    _rtAmbientOcclusion->render(cmd, currentFrame, frameResources, gbuffer, invVP);
}

if (_debugVisualization == DeferredDebugVisualization::FullComposition)
{
    renderCompositePass(cmd, currentFrame, inputImage, outputImage, frameResources, viewMatrix, projMatrix);
}
else
{
    auto* src = resolveDebugSource(inputImage, gbuffer);
    if (src)
    {
        renderDebugPass(cmd, src, outputImage, frameResources);
    }
}
```

**Key detail**: The `inverseViewProjection` is computed from the camera's projection and view matrices. This same matrix is also used by the composite shader (via push constants). We compute it here for the AO pass, and it's also computed inside `renderCompositePass` for the composite shader.

---

### Change 4: `resize()` — Resize AO Images

**Location**: after GBuffer resize loop (after line 225)

**Add**:
```cpp
_rtAmbientOcclusion->resize(newExtent);
```

**Context**:
```cpp
    for (auto& gb : _gbuffers)
    {
        gb->resize(newExtent);
    }
    _rtAmbientOcclusion->resize(newExtent);  // ← NEW
```

---

### Change 5: `cleanup()` — Cleanup AO Resources

**Location**: inside `cleanup()` method (after line 234, after `_gbuffers.clear()`)

**Add**:
```cpp
if (_rtAmbientOcclusion) _rtAmbientOcclusion->cleanup();
```

**Context**:
```cpp
    _gbuffers.clear();

    if (_rtAmbientOcclusion) _rtAmbientOcclusion->cleanup();  // ← NEW

    _frameDataBinding->cleanup();
```

---

### Change 6: `createCompositePipeline()` — Add Binding 7 to G-buffer Layout

**Location**: after line 309 (binding 6 for g_Depth)

**Add**:
```cpp
dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO
```

**Context**:
```cpp
dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_InputImage
dsLayoutFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Depth
dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO  ← NEW
_compositeGBufferDSLayout = dsLayoutFactory.build(
```

**Important**: This change affects BOTH the standard composite pipeline and the RT composite pipeline, because `_compositeGBufferDSLayout` is shared between them (see `createCompositePipelineRT()` line 363 which reuses `_compositeGBufferDSLayout`).

---

### Change 7: `renderCompositePass()` — Bind AO Image

**Location**: after the g_Depth descriptor set update (after line 542), before `gbufferDS->endUpdate()`

**Add**:
```cpp
auto aoImg = _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex());
gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    aoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
```

**Context**:
```cpp
gbufferDS->addImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);

auto aoImg = _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex());  // ← NEW
gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,                   // ← NEW
    aoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);        // ← NEW

gbufferDS->endUpdate();
```

**Layout transition**: The AO image is already in `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` at this point, because `RTAmbientOcclusion::render()` transitions it after the compute dispatch (or after the white-clear).

---

## Complete Execution Flow After Modifications

```
DeferredLayer::render(cmd, currentFrame, inputImage, outputImage, frameResources)
  │
  ├─ 1. renderGBufferPass()
  │     ├─ G-buffer images written
  │     └─ G-buffers → SHADER_READ_ONLY_OPTIMAL
  │
  ├─ 2. _rtAmbientOcclusion->render()
  │     ├─ [RT not supported] → no-op
  │     ├─ [No TLAS] → clear AO to white → SHADER_READ_ONLY_OPTIMAL
  │     └─ [TLAS exists]
  │           ├─ AO image → GENERAL
  │           ├─ Dispatch compute shader
  │           └─ AO image → SHADER_READ_ONLY_OPTIMAL
  │
  └─ 3. renderCompositePass()  OR  renderDebugPass()
        ├─ [Debug RTAmbientOcclusion] → blit AO image to output
        └─ [Full Composition]
              ├─ Bind G-buffers (set=0, bindings 0-6)
              ├─ Bind AO image (set=0, binding 7)     ← NEW
              ├─ Bind scene data (set=1)
              ├─ Bind environment (set=2)
              ├─ Bind lights (set=3)
              ├─ [RT] Bind TLAS (set=4)
              └─ Draw fullscreen quad
```
