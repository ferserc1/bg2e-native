# Step 4: RTReflections Class — Revised

## Objective

Create the `RTReflections` class that owns the ray tracing pipeline, SBT, per-frame reflection output images, descriptor set layout, sampler, and dispatch logic for reflection rays.

This revised version aligns SBT ownership with the revised `RayTracingPipeline::SBTData`, where the SBT buffer is returned as `std::unique_ptr<vulkan::Buffer>`.

## Files to Create

### `lib/include/bg2e/render/deferred/RTReflections.hpp`

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/factory/RayTracingPipeline.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>

#include <memory>
#include <vector>

namespace bg2e::render::deferred {

struct RTReflectionSettings {
    bool enabled = true;
    uint32_t sampleCount = 4;
    float maxRoughness = 0.35f;
    float rayBias = 0.02f;
    float maxDistance = 50.0f;
    float roughnessSpread = 1.0f;
};

class BG2E_API RTReflections {
public:
    explicit RTReflections(Engine* engine);
    ~RTReflections();

    void build(const GBufferManager* gbuffer, VkExtent2D extent);
    void resize(VkExtent2D extent);

    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const GBufferManager* gbuffer,
        const glm::mat4& inverseViewProjection,
        const glm::vec3& cameraPosition,
        VkAccelerationStructureKHR tlas
    );

    void cleanup();

    std::shared_ptr<vulkan::Image> reflectionImage(uint32_t frameIndex) const;
    VkSampler sampler() const { return _sampler; }
    bool rtSupported() const;

    void setSettings(const RTReflectionSettings& settings) { _settings = settings; }
    const RTReflectionSettings& settings() const { return _settings; }

    void setEnabled(bool enabled) { _settings.enabled = enabled; }
    void setSampleCount(uint32_t count) { _settings.sampleCount = count; }
    void setMaxRoughness(float roughness) { _settings.maxRoughness = roughness; }
    void setRayBias(float bias) { _settings.rayBias = bias; }
    void setMaxDistance(float distance) { _settings.maxDistance = distance; }
    void setRoughnessSpread(float spread) { _settings.roughnessSpread = spread; }

private:
    Engine* _engine = nullptr;
    RTReflectionSettings _settings;
    VkExtent2D _extent{};

    std::vector<std::shared_ptr<vulkan::Image>> _reflectionImages;
    std::shared_ptr<vulkan::Image> _fallbackImage;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    std::unique_ptr<vulkan::Buffer> _sbtBuffer;
    VkStridedDeviceAddressRegionKHR _raygenRegion = {};
    VkStridedDeviceAddressRegionKHR _missRegion = {};
    VkStridedDeviceAddressRegionKHR _hitRegion = {};
    VkStridedDeviceAddressRegionKHR _callableRegion = {};

    struct ReflectionPushConstants {
        glm::mat4 inverseViewProjection;
        glm::vec3 cameraPosition;
        float maxRoughness;
        glm::vec2 outputSize;
        uint32_t sampleCount;
        uint32_t frameIndex;
        float rayBias;
        float maxDistance;
        float roughnessSpread;
        uint32_t padding0;
    };

    void createPipeline();
    void createReflectionResources();
    void createFallbackImage();
    void cleanupImages();
};

}
```

## Implementation Details

### Constructor / Destructor

- Store the `Engine*`.
- Destructor calls `cleanup()` or follows the engine's existing cleanup manager pattern.

### `build(gbuffer, extent)`

Implementation flow:

```cpp
_extent = extent;
createFallbackImage();

if (!_engine->rayTracingSupported()) {
    return;
}

createPipeline();
createReflectionResources();
```

The fallback image should always exist. It is useful when RT is unsupported, disabled, or TLAS is null.

### `createPipeline()`

Create a descriptor set layout with 7 bindings:

```text
0: ACCELERATION_STRUCTURE_KHR  TLAS
1: STORAGE_IMAGE               reflection output
2: COMBINED_IMAGE_SAMPLER      G-buffer depth
3: COMBINED_IMAGE_SAMPLER      G-buffer normal
4: COMBINED_IMAGE_SAMPLER      G-buffer material
5: COMBINED_IMAGE_SAMPLER      G-buffer albedo
6: COMBINED_IMAGE_SAMPLER      G-buffer fresnel/flags
```

Create a pipeline layout with a push constant range for:

```cpp
ReflectionPushConstants
```

with stage:

```cpp
VK_SHADER_STAGE_RAYGEN_BIT_KHR
```

Then create the RT pipeline:

```cpp
vulkan::factory::RayTracingPipeline pipelineFactory(_engine);
pipelineFactory.setRayGenShader("rt_reflections.rgen.spv");
pipelineFactory.setMissShader("rt_reflections.rmiss.spv");
pipelineFactory.setClosestHitShader("rt_reflections.rchit.spv");

_pipeline = pipelineFactory.build(_pipelineLayout, 1, "RTReflections::Pipeline");
auto sbt = pipelineFactory.createSBT("RTReflections::SBT");

_sbtBuffer = std::move(sbt.buffer);
_raygenRegion = sbt.raygenRegion;
_missRegion = sbt.missRegion;
_hitRegion = sbt.hitRegion;
_callableRegion = sbt.callableRegion;
```

Important: `pipelineFactory` may be a local object only if it does not destroy `_pipeline` in its destructor. The factory should destroy only temporary shader modules, not the created pipeline. Pipeline lifetime belongs to `RTReflections`.

### `createReflectionResources()`

Create one reflection output image per frame-in-flight:

```cpp
_reflectionImages.resize(_engine->numImages());

for (uint32_t i = 0; i < _reflectionImages.size(); ++i) {
    _reflectionImages[i] = vulkan::Image::createAllocatedImage(
        _engine,
        "RT Reflections output " + std::to_string(i),
        VK_FORMAT_R16G16B16A16_SFLOAT,
        _extent,
        VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        false,
        20,
        VK_SAMPLE_COUNT_1_BIT
    );
}
```

### `render(...)`

Guard conditions:

```cpp
if (!_settings.enabled) return;
if (!_pipeline) return;
if (tlas == VK_NULL_HANDLE) return;
if (_reflectionImages.empty()) return;
```

Use the active frame-in-flight index:

```cpp
uint32_t frameIndex = _engine->currentFrameResourcesIndex();
auto output = _reflectionImages[frameIndex];
```

Render flow:

1. Transition output image to `VK_IMAGE_LAYOUT_GENERAL`.
2. Allocate descriptor set through `frameResources.newDescriptorSet(_dsLayout)`.
3. Write descriptors:
   - TLAS acceleration structure descriptor
   - storage image descriptor for output
   - sampled image descriptors for G-buffer depth, normal, material, albedo, fresnel/flags
4. Bind pipeline:

```cpp
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);
```

5. Bind descriptor set.
6. Push `ReflectionPushConstants`.
7. Dispatch:

```cpp
vulkan::cmdTraceRays(
    cmd,
    &_raygenRegion,
    &_missRegion,
    &_hitRegion,
    &_callableRegion,
    _extent.width,
    _extent.height,
    1
);
```

8. Barrier from ray tracing shader write to shader sampled read.
9. Transition output image to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.

### `reflectionImage(frameIndex)`

```cpp
if (_reflectionImages.empty() || frameIndex >= _reflectionImages.size()) {
    return _fallbackImage;
}
return _reflectionImages[frameIndex];
```

### `resize(extent)`

- Store new extent.
- Destroy old reflection images.
- Recreate fallback image.
- Recreate per-frame reflection images if RT is supported and pipeline exists.

### `cleanup()`

Destroy:

- per-frame reflection images
- fallback image
- SBT buffer
- pipeline
- pipeline layout
- descriptor set layout
- sampler

Use the existing deferred destruction pattern if the renderer requires it.

## Frame-in-Flight Requirement

All reflection output images must be replicated per frame-in-flight:

```cpp
_reflectionImages[frameIndex]
```

Never use a single global reflection output image for all frames.

## Verification

After this step:

- `RTReflections` compiles.
- It owns the RT pipeline and SBT cleanly.
- SBT ownership matches the revised `RayTracingPipeline::SBTData`.
- Reflection output images are allocated per frame-in-flight.
- The class is still unused by `DeferredLayer` until the render-flow integration step.
