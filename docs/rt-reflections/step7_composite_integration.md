# Step 7: Composite Integration — Revised

## Objective

Modify the deferred composite pass to accept the accumulated RT reflection texture and blend it with the existing cubemap/prefiltered environment reflection.

This revised version uses a real third composite variant instead of mixing two incompatible strategies.

## Composite Variants

The renderer should have three explicit composite variants:

```text
Standard composite:
  Set 0 bindings 0-6
  No RT AO/reflection texture

RT composite:
  Set 0 bindings 0-7
  Includes AO texture at binding 7

RT reflections composite:
  Set 0 bindings 0-8
  Includes AO texture at binding 7
  Includes RT reflection texture at binding 8
```

Do not add `binding = 8` to the existing `deferred_composite_rt.frag.glsl` unless its descriptor set layout is also changed everywhere. The safer approach is a separate shader and pipeline variant.

## Files to Create

### `shaders/src/deferred_composite_rt_reflections.frag.glsl`

Create this by copying:

```text
shaders/src/deferred_composite_rt.frag.glsl
```

Then add:

```glsl
layout(set = 0, binding = 8) uniform sampler2D g_RTReflection;
```

Find the existing environment reflection computation, where the prefiltered environment map is sampled using reflection vector `R` and roughness-derived LOD.

Replace the direct use of the environment reflection with a final reflection value:

```glsl
vec3 envReflection = textureLod(prefilteredEnvMap, R, lod).rgb;
vec4 rtReflection = texture(g_RTReflection, vTexcoord);

// First implementation: simple alpha-mask blend.
// alpha = 1 -> use RT reflection
// alpha = 0 -> use cubemap reflection
vec3 finalReflection = mix(envReflection, rtReflection.rgb, rtReflection.a);
```

Use `finalReflection` everywhere the shader previously used `envReflection` or the direct prefiltered envmap result for specular/environment reflection.

This first blend is intentionally simple. Do not add roughness remapping, Fresnel weighting, confidence weighting, spatial denoise or advanced BRDF correction in this step.

## Files to Modify

### `lib/include/bg2e/render/deferred/DeferredLayer.hpp`

Add members:

```cpp
VkPipeline _compositePipelineRTReflections = VK_NULL_HANDLE;
VkPipelineLayout _compositePipelineRTReflectionsLayout = VK_NULL_HANDLE;
VkDescriptorSetLayout _compositeGBufferRTReflectionsDSLayout = VK_NULL_HANDLE;
```

Add method declaration:

```cpp
void createCompositePipelineRTReflections();
```

If using an internal enum for composite selection, use:

```cpp
enum class CompositeVariant {
    Standard,
    RT,
    RTReflections
};
```

Avoid naming the second variant `RTShadows`; the existing RT composite path represents the RT-enhanced composite path, not necessarily only shadows.

### `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

## Descriptor Set Layout

Create a new descriptor set layout for the RT reflections composite variant:

```cpp
void DeferredLayer::createCompositePipelineRTReflections() {
    factory::DescriptorSetLayout dsLayoutFactory;

    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Albedo
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Normal
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Material
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // FresnelFlags
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // SheenColor
    dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // InputImage
    dsLayoutFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Depth
    dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // AO
    dsLayoutFactory.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // RT Reflections

    _compositeGBufferRTReflectionsDSLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    // Build _compositePipelineRTReflectionsLayout with the same set layout sequence
    // as the existing RT composite pipeline, but using the 9-binding set 0 layout.
    // Set 0: G-buffer + AO + RT reflections
    // Set 1: Fragment frame data
    // Set 2: Environment data
    // Set 3: Light data
    // Set 4: Ray tracing scene data / TLAS

    // Build _compositePipelineRTReflections using:
    // vertex shader:   deferred_composite.vert.spv
    // fragment shader: deferred_composite_rt_reflections.frag.spv
}
```

The exact factory calls should follow the existing `createCompositePipeline()` and `createCompositePipelineRT()` style.

## Pipeline Creation

Build the third pipeline with:

```text
Vertex shader:   deferred_composite.vert.spv
Fragment shader: deferred_composite_rt_reflections.frag.spv
```

The pipeline layout must include:

```text
Set 0: G-buffer + input + depth + AO + RT reflections
Set 1: Fragment frame data
Set 2: Environment data
Set 3: Light data
Set 4: Ray tracing scene data / TLAS
```

## `renderCompositePass()` Signature

Extend the composite pass to accept an optional reflection image:

```cpp
void renderCompositePass(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image* inputImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const vulkan::Image* reflectionImage
);
```

`reflectionImage` may be `nullptr`.

## Variant Selection

Use explicit selection:

```cpp
bool useRT = _useRtShadows && tlas != VK_NULL_HANDLE;
bool useReflections = useRT && reflectionImage != nullptr;

VkPipeline activePipeline = VK_NULL_HANDLE;
VkPipelineLayout activeLayout = VK_NULL_HANDLE;
VkDescriptorSetLayout activeGBufferLayout = VK_NULL_HANDLE;

if (useReflections) {
    activePipeline = _compositePipelineRTReflections;
    activeLayout = _compositePipelineRTReflectionsLayout;
    activeGBufferLayout = _compositeGBufferRTReflectionsDSLayout;
}
else if (useRT) {
    activePipeline = _compositePipelineRT;
    activeLayout = _compositePipelineRTLayout;
    activeGBufferLayout = _compositeGBufferRTDSLayout;
}
else {
    activePipeline = _compositePipeline;
    activeLayout = _compositePipelineLayout;
    activeGBufferLayout = _compositeGBufferDSLayout;
}
```

## Descriptor Set Updates

For the standard variant, write bindings 0-6.

For the RT variant, write bindings 0-7, where binding 7 is AO.

For the RT reflections variant, write bindings 0-8:

```cpp
// Existing bindings
addImage(0, albedo);
addImage(1, normal);
addImage(2, material);
addImage(3, fresnelFlags);
addImage(4, sheenColor);
addImage(5, inputImage);
addImage(6, depth);

// RT AO
addImage(7, denoisedAO);

// RT reflections
addImage(8, reflectionImage);
```

The reflection image must be in:

```cpp
VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
```

before the composite pass samples it.

## Render Flow Input

From Step 6, the reflection image passed into composite should be the accumulated reflection image, not the raw reflection output:

```cpp
const vulkan::Image* reflectionInputForComposite = nullptr;

if (_temporalReflectionAccumulator) {
    reflectionInputForComposite =
        _temporalReflectionAccumulator->outputImage(frameIndex).get();
}
```

Then call:

```cpp
renderCompositePass(
    cmd,
    currentFrame,
    inputImage,
    outputImage,
    frameResources,
    viewMatrix,
    projMatrix,
    reflectionInputForComposite
);
```

## Disabled / Unavailable Reflections

If reflections are disabled, unsupported, or TLAS is null:

- `reflectionImage` should be `nullptr`
- select the existing standard or RT composite path
- do not bind binding 8
- do not use the RT reflections shader variant

This preserves existing behavior.

## Verification

After this step:

- The composite pass has three clean variants.
- The existing standard composite remains unchanged.
- The existing RT composite remains unchanged.
- The new RT reflections composite uses a 9-binding set 0 descriptor layout.
- `deferred_composite_rt_reflections.frag.glsl` samples `g_RTReflection` at binding 8.
- Low-roughness surfaces can show accumulated RT reflections.
- Pixels with reflection alpha 0 fall back to cubemap reflection.
- No descriptor layout mismatch occurs.
