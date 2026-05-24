# Phase 3: Class Implementation — `RTAmbientOcclusion.cpp`

## File

`lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp` (create)

## Includes

```cpp
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <cmath>
```

## Method: Constructor

```cpp
RTAmbientOcclusion::RTAmbientOcclusion(Engine* engine)
    : _engine{engine}
{
}
```

## Method: Destructor

```cpp
RTAmbientOcclusion::~RTAmbientOcclusion()
{
    cleanup();
}
```

## Method: `build(VkExtent2D extent)`

1. Store `_extent = extent`
2. Create sampler:
   ```cpp
   vulkan::factory::Sampler samplerFactory(_engine);
   _sampler = samplerFactory.build();
   ```
3. Check RT support:
   ```cpp
   if (!_engine->rayTracingSupported()) {
       createWhiteFallback();
       _rtSupported = false;
       return;
   }
   _rtSupported = true;
   createAOResources(extent);
   createPipeline();
   ```
4. Register sampler cleanup via `_engine->cleanupManager().push()`

## Method: `createWhiteFallback()`

1. Allocate 16 bytes of 0xFF (4x4 white pixels):
   ```cpp
   uint8_t whiteData[16];
   memset(whiteData, 0xFF, sizeof(whiteData));
   ```
2. Create image:
   ```cpp
   auto img = std::shared_ptr<vulkan::Image>(
       vulkan::Image::createAllocatedImage(
           _engine, "RT AO fallback", whiteData,
           VkExtent2D{4, 4}, 1, VK_FORMAT_R8_UNORM,
           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
       )
   );
   ```
3. Set all frame entries to same image:
   ```cpp
   _aoImages.resize(_engine->numImages());
   for (auto& ao : _aoImages) {
       ao = img;
   }
   ```

**Important**: The fallback image is shared across all frame indices. It is never resized or destroyed during the lifetime of the class (only in `cleanup()`).

## Method: `createAOResources(VkExtent2D extent)`

1. Cleanup existing images (except shared fallback):
   ```cpp
   cleanupImages();
   ```
2. Create one image per frame resource:
   ```cpp
   _aoImages.resize(_engine->numImages());
   for (uint32_t i = 0; i < _aoImages.size(); i++) {
       _aoImages[i] = std::shared_ptr<vulkan::Image>(
           vulkan::Image::createAllocatedImage(
               _engine,
               "RT AO image " + std::to_string(i),
               VK_FORMAT_R8_UNORM,
               extent,
               VK_IMAGE_USAGE_STORAGE_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               VK_IMAGE_ASPECT_COLOR_BIT,
               1, false, 0, VK_SAMPLE_COUNT_1_BIT
           )
       );
   }
   ```

**Usage flags explained**:
- `STORAGE_IMAGE`: compute shader writes to it
- `SAMPLED_BIT`: composite fragment shader reads from it
- `TRANSFER_DST_BIT`: allows `vkCmdClearColorImage` for white-clear fallback path

## Method: `createPipeline()`

### Step 1: Descriptor Set Layout

```cpp
vulkan::factory::DescriptorSetLayout dsLayoutFactory;
dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);      // g_Normal
dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);      // g_Depth
dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR); // TLAS
dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);              // aoOutput
_dsLayout = dsLayoutFactory.build(
    _engine->device().handle(),
    VK_SHADER_STAGE_COMPUTE_BIT
);
```

### Step 2: Pipeline Layout

```cpp
vulkan::factory::PipelineLayout layoutFactory(_engine);
layoutFactory.addDescriptorSetLayout(_dsLayout);
layoutFactory.addPushConstantRange(
    0,
    sizeof(AOPushConstants),
    VK_SHADER_STAGE_COMPUTE_BIT
);
_pipelineLayout = layoutFactory.build();
```

### Step 3: Compute Pipeline

```cpp
vulkan::factory::ComputePipeline plFactory(_engine);
plFactory.setShader("rt_ao.comp.spv");
_pipeline = plFactory.build(_pipelineLayout);
```

### Step 4: Register Cleanup

```cpp
_engine->cleanupManager().push([&](VkDevice dev) {
    vkDestroyPipeline(dev, _pipeline, nullptr);
    _pipeline = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    _pipelineLayout = VK_NULL_HANDLE;
    vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
    _dsLayout = VK_NULL_HANDLE;
});
```

## Method: `render(VkCommandBuffer cmd, uint32_t currentFrame, vulkan::FrameResources& frameResources, const GBufferManager* gbuffer, const glm::mat4& inverseViewProjection)`

### Step 1: Early Exit

```cpp
if (!_rtSupported) return;
```

### Step 2: Check TLAS

```cpp
VkAccelerationStructureKHR tlas = frameResources.rayTracingScene->tlas();
auto aoImage = _aoImages[_engine->currentFrameResourcesIndex()];
```

### Step 3: Clear to White if No TLAS

```cpp
if (tlas == VK_NULL_HANDLE) {
    vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clearWhite{{1.0f, 0.0f, 0.0f, 0.0f}};
    VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, aoImage->handle(), VK_IMAGE_LAYOUT_GENERAL, &clearWhite, 1, &range);

    vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return;
}
```

### Step 4: Transition AO Image to GENERAL

```cpp
vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
```

### Step 5: Allocate and Update Descriptor Set

```cpp
auto ds = frameResources.newDescriptorSet(_dsLayout);
ds->beginUpdate();
ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);  // g_Normal
ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);  // g_Depth
ds->addAccelerationStructure(2, tlas);  // TLAS
ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    aoImage.get(), VK_IMAGE_LAYOUT_GENERAL);  // aoOutput (no sampler for storage images)
ds->endUpdate();
```

**Note**: `gbuffer->image(1)` is the Normal G-buffer (index 1). `gbuffer->depthImage()` is the depth buffer.

### Step 6: Bind Pipeline and Descriptor Set

```cpp
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
VkDescriptorSet dsHandle = ds->descriptorSet();
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);
```

### Step 7: Push Constants

```cpp
AOPushConstants pc;
pc.inverseViewProjection = inverseViewProjection;
pc.sampleCount = 1;
pc.padding = glm::vec3(0.0f);
vkCmdPushConstants(cmd, _pipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AOPushConstants), &pc);
```

### Step 8: Dispatch

```cpp
uint32_t groupX = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
uint32_t groupY = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
vkCmdDispatch(cmd, groupX, groupY, 1);
```

### Step 9: Transition AO Image to SHADER_READ_ONLY

```cpp
vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

## Method: `resize(VkExtent2D newExtent)`

```cpp
if (!_rtSupported) return;
_extent = newExtent;
createAOResources(newExtent);
```

Note: `createAOResources` calls `cleanupImages()` first.

## Method: `cleanupImages()`

```cpp
void RTAmbientOcclusion::cleanupImages()
{
    // For fallback case, all entries share the same image — only cleanup once
    // For RT case, each entry has its own image
    if (!_rtSupported) {
        if (!_aoImages.empty() && _aoImages[0]) {
            _aoImages[0]->cleanup();
        }
    } else {
        for (auto& img : _aoImages) {
            if (img) img->cleanup();
        }
    }
    _aoImages.clear();
}
```

## Method: `cleanup()`

```cpp
void RTAmbientOcclusion::cleanup()
{
    cleanupImages();
    // Pipeline, layout, dsLayout are destroyed by cleanupManager
    // Sampler is destroyed by cleanupManager
}
```

## Method: `aoImage(uint32_t frameIndex)`

```cpp
std::shared_ptr<vulkan::Image> RTAmbientOcclusion::aoImage(uint32_t frameIndex) const
{
    return _aoImages[frameIndex];
}
```

## Method: `sampler()`

```cpp
VkSampler RTAmbientOcclusion::sampler() const
{
    return _sampler;
}
```

## Method: `rtSupported()`

```cpp
bool RTAmbientOcclusion::rtSupported() const
{
    return _rtSupported;
}
```

## Reference Files

- `lib/src/bg2e/render/gbuffer/GBufferManager.cpp` — image creation pattern (lines 44-88)
- `lib/src/bg2e/render/GPUProcess.cpp` — compute pipeline creation pattern (lines 65-102)
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp` — cleanupManager pattern (lines 97-104)
- `lib/src/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.cpp` — acceleration structure descriptor set binding (lines 53-77)
