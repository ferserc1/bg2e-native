# Step 6: Render Flow Integration

## Objective

Integrate `RTReflections` and a second `TemporalAccumulator` instance into the `DeferredLayer` render flow, inserting them after AO denoise and before composite.

## Files to Modify

### `lib/include/bg2e/render/deferred/DeferredLayer.hpp`

**Add include:**
```cpp
#include <bg2e/render/deferred/RTReflections.hpp>
```

**Add member variables:**
```cpp
// After the existing _denoiseFilter member (line 178):
std::unique_ptr<RTReflections> _rtReflections;
std::unique_ptr<TemporalAccumulator> _temporalReflectionAccumulator;
```

**Add debug visualization enum values:**
```cpp
enum class DeferredDebugVisualization {
    FullComposition = 0,
    GBufferAlbedo,
    GBufferNormal,
    GBufferMaterial,
    GBufferFresnelFlags,
    GBufferSheenColor,
    GBufferDepth,
    InputImage,
    RTAmbientOcclusion,
    DenoisedAO,
    TemporalAccumulatedAO,
    RTReflections,                    // NEW
    TemporalAccumulatedReflections,   // NEW
    RTReflectionMask,                 // NEW

    MaxLayer
};
```

**Add public methods for reflection settings:**
```cpp
void setRTReflectionsEnabled(bool enabled);
bool rtReflectionsEnabled() const;

void setRTReflectionSampleCount(uint32_t count);
uint32_t rtReflectionSampleCount() const;

void setRTReflectionMaxRoughness(float r);
float rtReflectionMaxRoughness() const;

void setRTReflectionRayBias(float b);
float rtReflectionRayBias() const;

void setRTReflectionMaxDistance(float d);
float rtReflectionMaxDistance() const;

void setRTReflectionRoughnessSpread(float s);
float rtReflectionRoughnessSpread() const;
```

### `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

**In `build()`:** After creating `_denoiseFilter`, create the RT reflections subsystem:

```cpp
// After _denoiseFilter build:
if (_engine->rayTracingSupported()) {
    _rtReflections = std::make_unique<RTReflections>(_engine);
    _rtReflections->build(gbuffer, extent);

    _temporalReflectionAccumulator = std::make_unique<TemporalAccumulator>(_engine);
    _temporalReflectionAccumulator->setFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    _temporalReflectionAccumulator->setIsHDR(true);
    _temporalReflectionAccumulator->build(gbuffer, extent);
}
```

**In `resize()`:** Propagate resize to new subsystems:

```cpp
if (_rtReflections) {
    _rtReflections->resize(newExtent);
}
if (_temporalReflectionAccumulator) {
    _temporalReflectionAccumulator->resize(newExtent);
}
```

**In `render()`:** Modify the opaque layer render flow. The current flow is:

```
G-buffer → RTAO → Temporal AO → Denoise AO → Composite
```

Insert RT reflections after denoise:

```
G-buffer → RTAO → Temporal AO → Denoise AO → RT Reflections → Temporal Reflections → Composite
```

In the render method, after the denoise pass and before the composite/debug pass:

```cpp
// After denoise pass, before composite:
const vulkan::Image* reflectionInputForComposite = nullptr;

if (_rtReflections && _rtReflections->rtSupported()) {
    auto frameIndex = _engine->currentFrameResourcesIndex();
    auto tlas = frameResources.rayTracingScene ?
        frameResources.rayTracingScene->tlas() : VK_NULL_HANDLE;

    if (tlas != VK_NULL_HANDLE && _rtReflections->settings().enabled) {
        // RT Reflections pass
        _rtReflections->render(
            cmd, currentFrame, frameResources, gbuffer,
            invVP, cameraWorldPos, tlas
        );

        // Reflection temporal accumulation
        auto rawReflectionImage = _rtReflections->reflectionImage(frameIndex);
        _temporalReflectionAccumulator->render(
            cmd, currentFrame, frameResources, gbuffer,
            invVP, viewMatrix, projMatrix,
            rawReflectionImage.get()
        );

        reflectionInputForComposite = _temporalReflectionAccumulator->outputImage(frameIndex);
    }
}
```

**In `renderCompositePass()`:** Pass the reflection image as an additional parameter:

```cpp
void renderCompositePass(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image* inputImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const vulkan::Image* reflectionImage  // NEW parameter, nullable
);
```

**In `resolveDebugSource()`:** Add cases for new debug modes:

```cpp
case DeferredDebugVisualization::RTReflections:
    return _rtReflections ? _rtReflections->reflectionImage(frameIndex).get() : nullptr;

case DeferredDebugVisualization::TemporalAccumulatedReflections:
    return _temporalReflectionAccumulator ?
        _temporalReflectionAccumulator->outputImage(frameIndex).get() : nullptr;

case DeferredDebugVisualization::RTReflectionMask:
    // Show alpha channel as grayscale — handled via a specific shader variant
    return _temporalReflectionAccumulator ?
        _temporalReflectionAccumulator->outputImage(frameIndex).get() : nullptr;
```

**In `cleanup()`:** Cleanup new subsystems:

```cpp
if (_temporalReflectionAccumulator) {
    _temporalReflectionAccumulator->cleanup();
}
if (_rtReflections) {
    _rtReflections->cleanup();
}
```

## Initialization Order

The `build()` method creates subsystems in this order:
1. G-buffers (existing)
2. RT AO (existing)
3. Temporal AO accumulator (existing)
4. Denoise filter (existing)
5. **RT Reflections** (new)
6. **Temporal reflection accumulator** (new)
7. Pipelines (existing, modified in step 7)

## Frame-in-Flight Safety

All new resources follow the existing pattern:
- `_rtReflections->reflectionImage(frameIndex)` — per swapchain image
- `_temporalReflectionAccumulator->outputImage(frameIndex)` — per swapchain image
- Descriptor sets allocated per frame via `frameResources.newDescriptorSet()`

## TLAS Access

The TLAS is accessed via `frameResources.rayTracingScene->tlas()`. This is the same pattern used by the existing RT shadows in the composite pass. If the ray tracing scene is null (no scene loaded), reflections are skipped.

## Verification

After this step:
- The engine compiles with the new subsystems
- RT reflections are created and rendered (producing output images)
- Temporal accumulation runs on reflection data
- The composite pass receives reflection data (integration in step 7)
- Debug visualization shows raw and accumulated reflection images
- Existing AO and denoise passes work identically
- If RT is not supported, the new subsystems are null and skipped
