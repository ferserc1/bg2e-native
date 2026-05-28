# Step 8: Debug Visualization

## Objective

Extend the debug visualization system to display raw RT reflections, accumulated reflections, and the reflection mask.

## Files to Modify

### `lib/include/bg2e/render/deferred/DeferredLayer.hpp`

The enum values were already added in Step 6:

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

### `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

**In `resolveDebugSource()`:** Add cases for the new debug modes:

```cpp
const vulkan::Image* DeferredLayer::resolveDebugSource(
    const vulkan::Image* inputImage,
    GBufferManager* gbuffer
) const {
    auto frameIndex = _engine->currentFrameResourcesIndex();

    switch (_debugVisualization) {
    // ... existing cases ...

    case DeferredDebugVisualization::RTReflections:
        if (_rtReflections) {
            return _rtReflections->reflectionImage(frameIndex).get();
        }
        return nullptr;

    case DeferredDebugVisualization::TemporalAccumulatedReflections:
        if (_temporalReflectionAccumulator) {
            return _temporalReflectionAccumulator->outputImage(frameIndex).get();
        }
        return nullptr;

    case DeferredDebugVisualization::RTReflectionMask:
        // The mask is in the alpha channel of the accumulated reflection
        // We need a special shader to visualize it — for now, show the
        // full RGBA image (the debug blit shader will show RGB, and the
        // alpha-based visualization requires a shader variant)
        if (_temporalReflectionAccumulator) {
            return _temporalReflectionAccumulator->outputImage(frameIndex).get();
        }
        return nullptr;

    default:
        return nullptr;
    }
}
```

**RTReflectionMask visualization:**

The `RTReflectionMask` debug mode should show the alpha channel as grayscale. The existing debug blit shader simply outputs `texture(srcImage, vTexcoord)` which shows RGB channels.

Options:
1. Create a separate debug shader that outputs `.aaa1` instead of `.rgba`
2. Add a push constant to the debug blit shader to select channel(s)
3. Use a swizzle in the descriptor set (not standard Vulkan)

**Recommended approach:** Add a push constant to the debug blit shader:

Modify `deferred_debug_blit.frag.glsl`:
```glsl
layout(push_constant) uniform PC {
    uint channelMode;  // 0=RGB, 1=R, 2=G, 3=B, 4=A, 5=RGB*alpha
} pc;

void main() {
    vec4 color = texture(srcImage, vTexcoord);
    switch (pc.channelMode) {
        case 0: outColor = vec4(color.rgb, 1.0); break;
        case 1: outColor = vec4(color.rrr, 1.0); break;
        case 2: outColor = vec4(color.ggg, 1.0); break;
        case 3: outColor = vec4(color.bbb, 1.0); break;
        case 4: outColor = vec4(color.aaa, 1.0); break;
        case 5: outColor = vec4(color.rgb * color.a, 1.0); break;
        default: outColor = vec4(color.rgb, 1.0); break;
    }
}
```

Modify `renderDebugPass()` to accept a channel mode:
```cpp
void renderDebugPass(
    VkCommandBuffer cmd,
    const vulkan::Image* sourceImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources,
    uint32_t channelMode = 0  // NEW parameter
);
```

The debug pipeline layout needs a push constant range:
```cpp
struct DebugPushConstants {
    uint32_t channelMode;
};
```

In the render method, select channel mode based on debug visualization:
```cpp
uint32_t channelMode = 0;  // RGB
if (_debugVisualization == DeferredDebugVisualization::RTReflectionMask) {
    channelMode = 4;  // Alpha channel
}
renderDebugPass(cmd, sourceImage, outputImage, frameResources, channelMode);
```

For `RTReflections` (raw) and `TemporalAccumulatedReflections`:
- `channelMode = 0` (RGB) shows the reflection color
- For pixels with alpha=0 (no reflection), RGB is (0,0,0) — shows as black
- For pixels with alpha=1 (valid reflection), shows the traced color

For `RTReflectionMask`:
- `channelMode = 4` shows alpha as grayscale
- White = valid reflection, black = no reflection (cubemap fallback)

## Verification

After this step:
- Debug mode `RTReflections` shows raw traced reflection RGB
- Debug mode `TemporalAccumulatedReflections` shows accumulated reflection RGB
- Debug mode `RTReflectionMask` shows alpha channel as grayscale
- Existing debug modes work identically
- The debug blit shader handles the new push constant gracefully (defaults to RGB mode)
