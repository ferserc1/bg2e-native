# Step 7: Composite Integration

## Objective

Modify `deferred_composite_rt.frag.glsl` to accept the accumulated RT reflection texture and blend it with the cubemap/prefiltered environment reflection. This shader is the **single unified RT composite shader** — all ray tracing effects (shadows, AO, reflections) are combined here. No separate composite shaders are created.

## Composite Variants

The renderer has exactly two composite variants:

```text
Standard composite:
  Set 0 bindings 0-6
  No RT textures
  Shader: deferred_composite.frag.spv

RT composite:
  Set 0 bindings 0-8
  Binding 7: AO texture
  Binding 8: RT reflection texture (fallback to 1x1 alpha=0 image when unavailable)
  Shader: deferred_composite_rt.frag.spv
```

## Shader Modification

### `shaders/src/deferred_composite_rt.frag.glsl`

Add binding 8 for the RT reflection texture:

```glsl
layout(set = 0, binding = 8) uniform sampler2D g_RTReflection;
```

Inline the `calcAmbientLight()` function as `calcAmbientLightWithReflections()`, adding an `rtReflection` parameter. Replace the direct environment reflection with a blended value:

```glsl
vec3 envReflection = mix(prefilteredColor1, prefilteredColor2, fract(sampleRoughness));
vec3 finalReflection = mix(envReflection, rtReflection.rgb, rtReflection.a);
vec3 specular = finalReflection * (F * envBRDF.x + envBRDF.y);
```

Use `finalReflection` everywhere the shader previously used the prefiltered envmap result for specular/environment reflection.

This first blend is intentionally simple. Do not add roughness remapping, Fresnel weighting, confidence weighting, spatial denoise or advanced BRDF correction in this step.

## Files to Modify

### `lib/include/bg2e/render/deferred/DeferredLayer.hpp`

Add a fallback image member for when RT reflections are unavailable:

```cpp
std::shared_ptr<vulkan::Image> _rtReflectionFallbackImage;
```

No new pipeline, layout, or descriptor set members are needed — the existing `_compositePipelineRT` / `_compositeGBufferRTDSLayout` are updated to support 9 bindings.

### `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

#### `createCompositePipelineRT()`

Update the descriptor set layout to include binding 8:

```cpp
dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO
dsLayoutFactory.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_RTReflection
```

#### `build()`

Create a 1x1 RGBA16F fallback image with alpha=0 for when no RT reflection is available:

```cpp
std::vector<uint8_t> blackData(4 * 4 * sizeof(uint16_t), 0);
_rtReflectionFallbackImage = std::shared_ptr<vulkan::Image>(
    vulkan::Image::createAllocatedImage(
        _engine, "RT Reflections fallback", blackData.data(),
        VkExtent2D{1, 1}, 4, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    )
);
```

#### `renderCompositePass()`

Simplify variant selection to two variants:

```cpp
bool useRT = _useRtShadows && tlas != VK_NULL_HANDLE;

VkPipeline activePipeline = useRT ? _compositePipelineRT : _compositePipeline;
VkPipelineLayout activeLayout = useRT ? _compositePipelineRTLayout : _compositePipelineLayout;
VkDescriptorSetLayout activeGBufferLayout = useRT ? _compositeGBufferRTDSLayout : _compositeGBufferDSLayout;
```

For the RT variant, always write bindings 0-8. When no reflection image is available, use the fallback:

```cpp
if (useRT) {
    addImage(7, denoisedAO);
    const vulkan::Image* reflImg = reflectionImage ? reflectionImage : _rtReflectionFallbackImage.get();
    addImage(8, reflImg);
}
```

## `renderCompositePass()` Signature

```cpp
void renderCompositePass(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image* inputImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const vulkan::Image* reflectionImage  // nullable
);
```

## Render Flow Input

From Step 6, the reflection image passed into composite should be the accumulated reflection image, not the raw reflection output:

```cpp
const vulkan::Image* reflectionInputForComposite = nullptr;

if (_temporalReflectionAccumulator) {
    reflectionInputForComposite =
        _temporalReflectionAccumulator->outputImage(frameIndex).get();
}
```

## Disabled / Unavailable Reflections

If reflections are disabled, unsupported, or TLAS is null:

- `reflectionImage` should be `nullptr`
- The fallback image (alpha=0) is bound to binding 8
- The shader samples it and `mix()` returns the cubemap reflection unchanged
- No descriptor layout mismatch occurs

## Verification

After this step:

- `deferred_composite_rt.frag.glsl` is the single unified RT composite shader with 9 bindings.
- No separate composite shader or pipeline is created for reflections.
- The standard composite remains unchanged (7 bindings).
- The RT composite handles both AO and reflections (9 bindings).
- Low-roughness surfaces can show accumulated RT reflections.
- Pixels with reflection alpha 0 fall back to cubemap reflection.
- No descriptor layout mismatch occurs.
