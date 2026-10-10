# UV lighting entry points

**Headers:** `<bg2e/render/deferred/RTAmbientOcclusion.hpp>`,
`<bg2e/render/deferred/RTGlobalIllumination.hpp>`
**Namespace:** `bg2e::render::deferred`

These are the atlas-facing methods of existing lighting classes. Normal baker
clients use context factories; contexts configure these passes and bindings.
The UV methods are independent of the classes' screen-space build/render paths.

## RTAmbientOcclusion

```cpp
void setMaterialDataBinding(vulkan::rt::RTMaterialDataBinding* binding);
void buildUv();
void renderUv(VkCommandBuffer cmd, uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::rt::RayTracingScene& rayTracingScene,
    vulkan::Image& aoOutput, uint32_t samplesPerPixel, float maxRayDistance);
void clearUvNeutral(VkCommandBuffer cmd, vulkan::Image& aoOutput);
void cleanupUv();
```

`buildUv()` requires ray tracing and a material binding; creates a separate
compute pipeline. `renderUv()` records full-resolution visibility, using frame
index for random sampling. It requires a depthless surface with at least four
attachments, RGBA32F position, RGBA16F normal and R32_UINT mask at index 3;
matching output extent and R8_UNORM output; a nonempty TLAS; positive samples
at most INT_MAX and finite positive distance. Invalid inputs throw, and calling
before build throws `std::logic_error`. Baker contexts supply a fixed 0.1 m
range. UV origin bias/falloff use the pass defaults (0.0017 m and 1.0), not
`LightmapSettings::giRayBias`. Screen AO quality/bounce settings do not change
this UV shader's sampling loop. `clearUvNeutral()` initializes visibility one;
`cleanupUv()` releases the UV resources. Outputs finish in shader-read layout.

## RTGlobalIllumination

```cpp
void setMaterialDataBinding(vulkan::rt::RTMaterialDataBinding* binding);
void setReflectionLightDataBinding(vulkan::rt::ReflectionLightDataBinding* binding);
void buildUv();
void renderUv(VkCommandBuffer cmd, uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::rt::RayTracingScene& rayTracingScene,
    vulkan::Image& giOutput, vulkan::Image* irradianceMap,
    VkSampler irradianceSampler, const std::vector<base::LightData>& giLights,
    const LightmapSettings& settings);
void clearUv(VkCommandBuffer cmd, vulkan::Image& giOutput);
void cleanupUv();
```

`buildUv()` requires ray tracing and both bindings, creates the UV ray-tracing
pipeline and SBT. `renderUv()` records one full-atlas trace. Surface requirements
match AO; output is RGBA16F with matching extent. A nonempty TLAS, IBL image
and non-null sampler are required. Samples/bounces must be positive, samples
at most INT_MAX and RTGI range finite/positive. Settings come from the baker,
not live `RTGISettings`; UV execution disables blue noise, enables shadows and
caps GI shadow samples at 32. The caller supplies already filtered lights.
`clearUv()` initializes zero GI. Outputs finish in shader-read layout.

Both passes allocate descriptors and frame-owned transient bindings; do not
clear allocators or release those resources before recorded work has completed.
`cleanupUv()` is distinct from the ordinary screen-path cleanup methods.

Sources: [AO header](../../../lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp),
[AO implementation](../../../lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp),
[GI header](../../../lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp),
[GI implementation](../../../lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp).
See [lighting architecture](../../architecture/lightmaps/lighting_passes.md).
