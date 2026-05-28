# Step 5: RGBA Temporal Accumulator Support — Revised

## Objective

Modify `TemporalAccumulator` to support both scalar AO accumulation and RGBA HDR reflection accumulation.

This revised version explicitly adds:

- configurable history/output format
- `setIsHDR(bool)` / `isHDR()` API
- safer alpha-mask handling for reflections

## Why This Is Needed

The current `TemporalAccumulator` hardcodes scalar AO-style accumulation. Reflections need HDR RGBA accumulation using:

```cpp
VK_FORMAT_R16G16B16A16_SFLOAT
```

The alpha channel stores reflection validity:

```text
A = 1.0 -> valid RT reflection
A = 0.0 -> no valid RT reflection; composite should fall back to cubemap
```

## Files to Modify

### `lib/include/bg2e/render/deferred/TemporalAccumulator.hpp`

Add public API:

```cpp
void setFormat(VkFormat format) { _format = format; }
VkFormat format() const { return _format; }

void setIsHDR(bool value) { _isHDR = value; }
bool isHDR() const { return _isHDR; }
```

Add private members:

```cpp
VkFormat _format = VK_FORMAT_R16_SFLOAT;
bool _isHDR = false;
```

Default values preserve existing AO behavior.

Update the push constants struct:

```cpp
struct AccumulatorPushConstants {
    glm::mat4 currentInverseViewProjection;
    glm::mat4 previousViewProjection;
    glm::vec2 outputSize;
    float historyWeight;
    uint32_t accumulatedFrameCount;
    uint32_t useProgressiveMode;
    uint32_t hasHistory;
    float depthThreshold;
    float normalThreshold;
    uint32_t isHDR;      // 0 = scalar AO, 1 = RGBA reflections
    uint32_t padding0;
};
```

When filling push constants:

```cpp
pc.isHDR = _isHDR ? 1u : 0u;
pc.padding0 = 0u;
```

Important: update the pipeline layout push constant size to `sizeof(AccumulatorPushConstants)`.

### `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`

In history/output image creation, replace hardcoded scalar format with `_format` only for accumulation images.

Use `_format` for:

```text
_historyImagesA
_historyImagesB
```

Do not use `_format` for:

```text
_prevDepthImages
_prevNormalImages
```

Those must keep their existing depth and normal formats.

Reflection accumulator setup should be:

```cpp
_temporalReflectionAccumulator = std::make_unique<TemporalAccumulator>(_engine);
_temporalReflectionAccumulator->setFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
_temporalReflectionAccumulator->setIsHDR(true);
_temporalReflectionAccumulator->build(gbuffer, extent);
```

AO accumulator setup can remain unchanged:

```cpp
_temporalAOAccumulator = std::make_unique<TemporalAccumulator>(_engine);
_temporalAOAccumulator->build(gbuffer, extent);
```

because the default is scalar mode.

## Shader Changes

### `shaders/src/temporal_accumulation.comp.glsl`

The shader must support both modes using `pc.isHDR`.

Current scalar AO behavior should remain unchanged:

```glsl
if (pc.isHDR == 0u) {
    float current = texture(g_CurrentInput, uv).r;
    float history = texture(g_HistoryInput, reprojectedUv).r;

    float result;
    if (validHistory) {
        result = mix(history, current, blendWeight);
    } else {
        result = current;
    }

    imageStore(outAccumulated, pixel, vec4(result, 0.0, 0.0, 0.0));
    return;
}
```

For HDR reflection mode:

```glsl
vec4 current = texture(g_CurrentInput, uv);

// If the current frame has no valid reflection, do not preserve old reflection data.
// This avoids ghost reflections when the current pixel should fall back to cubemap.
if (current.a < 0.01) {
    imageStore(outAccumulated, pixel, vec4(0.0));
    return;
}

if (!validHistory) {
    imageStore(outAccumulated, pixel, current);
    return;
}

vec4 history = texture(g_HistoryInput, reprojectedUv);

if (history.a < 0.01) {
    imageStore(outAccumulated, pixel, current);
    return;
}

vec3 resultRGB = mix(history.rgb, current.rgb, blendWeight);
imageStore(outAccumulated, pixel, vec4(resultRGB, 1.0));
```

Where `validHistory` must already include:

- `hasHistory != 0`
- valid reprojected UV
- depth threshold check
- normal threshold check
- camera/history invalidation rules

## Alpha Policy

Do not use this for reflections:

```glsl
float resultAlpha = max(history.a, current.a);
```

That can keep stale reflections alive even when the current frame no longer has a valid reflection.

Use this policy instead:

```text
current alpha == 0 -> output zero, no history preservation
current alpha == 1 and valid history alpha == 1 -> blend RGB, alpha = 1
current alpha == 1 but invalid history -> current, alpha = 1
```

This is conservative and reduces ghosting.

## Backward Compatibility

Existing AO accumulation remains compatible because:

- `_format` defaults to `VK_FORMAT_R16_SFLOAT`
- `_isHDR` defaults to `false`
- `pc.isHDR` defaults to `0`
- scalar mode writes the same data as before

## Verification

After this step:

- Existing AO temporal accumulation still works.
- Reflection accumulation can use `VK_FORMAT_R16G16B16A16_SFLOAT`.
- Reflection alpha is treated as a validity mask.
- Invalid current reflection pixels do not preserve stale history.
- The class can safely be instantiated twice: once for AO and once for reflections.
