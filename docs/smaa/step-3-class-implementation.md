# Step 3: Class Implementation — `SMAAProcessor.cpp`

## File

`lib/src/bg2e/render/deferred/SMAAProcessor.cpp` (create)

## Purpose

Implement the `SMAAProcessor` class that manages SMAA compute pipelines, LUT textures, per-frame resources, and the three-pass execution.

## Includes

```cpp
#include <bg2e/render/deferred/SMAAProcessor.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/GPUProcess.hpp>
#include <cmath>
```

## Key Implementation Details

### Constructor / Destructor

```cpp
SMAAProcessor::SMAAProcessor(Engine* engine)
    : _engine{engine}
{
}

SMAAProcessor::~SMAAProcessor()
{
    cleanup();
}
```

### `build(VkExtent2D extent, VkFormat outputFormat)`

1. Store `_extent = extent` and `_outputFormat = outputFormat`
2. Create `_sampler` using `vulkan::factory::Sampler`:
   - `VK_FILTER_LINEAR` (mag and min)
   - `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` (all axes)
3. Register sampler cleanup via `_engine->cleanupManager().push()`
4. Call `createLUTTextures()` — allocates `_areaTexture` (256×256 RG8) and `_searchTexture` (64×64 R8)
5. Call `generateLUTs()` — uses `GPUProcess::executeShader()` to fill the LUT textures
6. Call `createPipelines()` — builds all descriptor set layouts, pipeline layouts, and compute pipelines
7. Call `createFrameImages(extent)` — allocates per-frame `edgesImage`, `blendWeightsImage`, `outputImage`

### `createLUTTextures()`

```cpp
void SMAAProcessor::createLUTTextures()
{
    _areaTexture = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "SMAA area texture",
            VK_FORMAT_R8G8_UNORM,
            {256, 256},
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 0, VK_SAMPLE_COUNT_1_BIT
        )
    );

    _searchTexture = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "SMAA search texture",
            VK_FORMAT_R8_UNORM,
            {64, 64},
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 0, VK_SAMPLE_COUNT_1_BIT
        )
    );
}
```

### `generateLUTs()`

Uses `GPUProcess` (immediate submit) to generate the LUT textures. This runs only once.

```cpp
void SMAAProcessor::generateLUTs()
{
    // Generate area texture
    {
        GPUProcess proc(_engine);
        proc.addBinding(0, _areaTexture.get(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        proc.executeShader("smaa_area_generate.comp.spv", {256, 256});
    }

    // Generate search texture
    {
        GPUProcess proc(_engine);
        proc.addBinding(0, _searchTexture.get(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        proc.executeShader("smaa_search_generate.comp.spv", {64, 64});
    }

    _lutsGenerated = true;
}
```

**Important**: `GPUProcess::executeShader()` calls `immediateSubmit`, which blocks and creates/destroys the pipeline per call. This is acceptable for one-time LUT generation. The LUT textures remain in `SHADER_READ_ONLY_OPTIMAL` layout after generation.

### `createPipelines()`

Create three separate descriptor set layouts (one per pass), a shared pipeline layout, and three compute pipelines.

```cpp
void SMAAProcessor::createPipelines()
{
    // === Descriptor Set Layouts ===

    // Edge Detection: binding 0 = sampler, binding 1 = storage image
    vulkan::factory::DescriptorSetLayout edgeDSFactory;
    edgeDSFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    edgeDSFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _edgeDSLayout = edgeDSFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    // Blend Weight: bindings 0,1,2 = sampler, binding 3 = storage image
    vulkan::factory::DescriptorSetLayout blendDSFactory;
    blendDSFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    blendDSFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    blendDSFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    blendDSFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _blendWeightDSLayout = blendDSFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    // Neighborhood Blend: bindings 0,1 = sampler, binding 2 = storage image
    vulkan::factory::DescriptorSetLayout nblendDSFactory;
    nblendDSFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    nblendDSFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    nblendDSFactory.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _neighborhoodBlendDSLayout = nblendDSFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    // === Pipeline Layout (shared) ===
    // We need three separate pipeline layouts because each pass uses
    // a different descriptor set layout. Create one per pass.

    // Edge detection pipeline layout
    vulkan::factory::PipelineLayout edgeLayoutFactory(_engine);
    edgeLayoutFactory.addDescriptorSetLayout(_edgeDSLayout);
    edgeLayoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipelineLayout edgePipelineLayout = edgeLayoutFactory.build("SMAA::EdgeDetection::PipelineLayout");

    // Blend weight pipeline layout
    vulkan::factory::PipelineLayout blendLayoutFactory(_engine);
    blendLayoutFactory.addDescriptorSetLayout(_blendWeightDSLayout);
    blendLayoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipelineLayout blendPipelineLayout = blendLayoutFactory.build("SMAA::BlendWeight::PipelineLayout");

    // Neighborhood blend pipeline layout
    vulkan::factory::PipelineLayout nblendLayoutFactory(_engine);
    nblendLayoutFactory.addDescriptorSetLayout(_neighborhoodBlendDSLayout);
    nblendLayoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipelineLayout nblendPipelineLayout = nblendLayoutFactory.build("SMAA::NeighborhoodBlend::PipelineLayout");

    // === Pipelines ===
    vulkan::factory::ComputePipeline plFactory(_engine);

    plFactory.setShader("smaa_edge_detection.comp.spv");
    _edgeDetectionPipeline = plFactory.build(edgePipelineLayout,
        "SMAA::EdgeDetection::Pipeline");

    plFactory.setShader("smaa_blend_weight.comp.spv");
    _blendWeightPipeline = plFactory.build(blendPipelineLayout,
        "SMAA::BlendWeight::Pipeline");

    plFactory.setShader("smaa_neighborhood_blend.comp.spv");
    _neighborhoodBlendPipeline = plFactory.build(nblendPipelineLayout,
        "SMAA::NeighborhoodBlend::Pipeline");

    // Store pipeline layouts for use in process()
    // (need to store all three layouts, not just one)
    // → add member variables: _edgePipelineLayout, _blendPipelineLayout, _nblendPipelineLayout

    // Register cleanup
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _edgeDetectionPipeline, nullptr);
        vkDestroyPipeline(dev, _blendWeightPipeline, nullptr);
        vkDestroyPipeline(dev, _neighborhoodBlendPipeline, nullptr);
        vkDestroyPipelineLayout(dev, edgePipelineLayout, nullptr);
        vkDestroyPipelineLayout(dev, blendPipelineLayout, nullptr);
        vkDestroyPipelineLayout(dev, nblendPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _edgeDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _blendWeightDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _neighborhoodBlendDSLayout, nullptr);
    });
}
```

**Note**: The class header should be updated to store three separate pipeline layouts:

```cpp
VkPipelineLayout _edgePipelineLayout = VK_NULL_HANDLE;
VkPipelineLayout _blendPipelineLayout = VK_NULL_HANDLE;
VkPipelineLayout _nblendPipelineLayout = VK_NULL_HANDLE;
```

Remove the single `_pipelineLayout` from the header and use the three separate ones.

### `createFrameImages(VkExtent2D extent)`

```cpp
void SMAAProcessor::createFrameImages(VkExtent2D extent)
{
    cleanupFrameImages();

    uint32_t numFrames = _engine->numImages();
    _frames.resize(numFrames);

    for (uint32_t i = 0; i < numFrames; i++)
    {
        _frames[i].edgesImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "SMAA edges image " + std::to_string(i),
                VK_FORMAT_R8G8_UNORM,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );

        _frames[i].blendWeightsImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "SMAA blend weights image " + std::to_string(i),
                VK_FORMAT_R8G8B8A8_UNORM,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );

        _frames[i].outputImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "SMAA output image " + std::to_string(i),
                _outputFormat,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}
```

### `resize(VkExtent2D newExtent)`

```cpp
void SMAAProcessor::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createFrameImages(newExtent);
    // LUT textures and pipelines are unchanged
}
```

### `process(VkCommandBuffer cmd, uint32_t frameIndex, vulkan::Image* inputImage)`

This is the main method. It records three compute dispatches into the command buffer.

```cpp
const vulkan::Image* SMAAProcessor::process(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    const vulkan::Image* inputImage)
{
    auto& frame = _frames[frameIndex];
    glm::vec2 texelSize = {
        1.0f / static_cast<float>(_extent.width),
        1.0f / static_cast<float>(_extent.height)
    };

    // === Pass 1: Edge Detection ===
    {
        // Transition edgesImage to GENERAL for writing
        vulkan::Image::cmdTransitionImage(cmd, frame.edgesImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // Allocate and update descriptor set
        auto ds = _engine->currentFrameResources().newDescriptorSet(_edgeDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.edgesImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _edgeDetectionPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _edgePipelineLayout, 0, 1, &dsHandle, 0, nullptr);

        SMAAPushConstants pc{ texelSize };
        vkCmdPushConstants(cmd, _edgePipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pc);

        uint32_t gx = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
        uint32_t gy = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
        vkCmdDispatch(cmd, gx, gy, 1);

        // Transition edgesImage to SHADER_READ_ONLY for pass 2
        vulkan::Image::cmdTransitionImage(cmd, frame.edgesImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // === Pass 2: Blend Weight Calculation ===
    {
        vulkan::Image::cmdTransitionImage(cmd, frame.blendWeightsImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        auto ds = _engine->currentFrameResources().newDescriptorSet(_blendWeightDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            frame.edgesImage.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _areaTexture.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _searchTexture.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.blendWeightsImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _blendWeightPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _blendPipelineLayout, 0, 1, &dsHandle, 0, nullptr);

        SMAAPushConstants pc{ texelSize };
        vkCmdPushConstants(cmd, _blendPipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pc);

        uint32_t gx = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
        uint32_t gy = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
        vkCmdDispatch(cmd, gx, gy, 1);

        vulkan::Image::cmdTransitionImage(cmd, frame.blendWeightsImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // === Pass 3: Neighborhood Blending ===
    {
        vulkan::Image::cmdTransitionImage(cmd, frame.outputImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        auto ds = _engine->currentFrameResources().newDescriptorSet(_neighborhoodBlendDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            frame.blendWeightsImage.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.outputImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _neighborhoodBlendPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _nblendPipelineLayout, 0, 1, &dsHandle, 0, nullptr);

        SMAAPushConstants pc{ texelSize };
        vkCmdPushConstants(cmd, _nblendPipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pc);

        uint32_t gx = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
        uint32_t gy = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
        vkCmdDispatch(cmd, gx, gy, 1);

        vulkan::Image::cmdTransitionImage(cmd, frame.outputImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return frame.outputImage.get();
}
```

**Note on descriptor sets**: The code uses `_engine->currentFrameResources().newDescriptorSet()` which allocates from the frame's descriptor allocator. This matches the pattern used by `DenoiseFilter::render()`. The descriptor sets are automatically freed when the frame is flushed.

**Note on synchronization**: Between compute passes, a pipeline barrier is needed to ensure the storage image write from one pass is visible as a sampled read in the next pass. The `cmdTransitionImage()` call handles this by using `VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT` for both src and dst stage masks (see `Image::TransitionInfo` defaults). For more precise synchronization, explicit barriers with `VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT` could be used, but the default behavior is correct.

### `cleanup()`

```cpp
void SMAAProcessor::cleanup()
{
    cleanupFrameImages();

    // Pipelines and layouts are cleaned up via cleanupManager
    // LUT textures are cleaned up via unique_ptr

    _areaTexture.reset();
    _searchTexture.reset();
}

void SMAAProcessor::cleanupFrameImages()
{
    for (auto& frame : _frames)
    {
        if (frame.edgesImage) frame.edgesImage->cleanup();
        if (frame.blendWeightsImage) frame.blendWeightsImage->cleanup();
        if (frame.outputImage) frame.outputImage->cleanup();
    }
    _frames.clear();
}
```

## Error Handling

- If `_lutsGenerated` is false when `process()` is called, generate LUTs first (fallback for deferred initialization).
- The `outputImage` format should match the expected output. If the swapchain uses `B8G8R8A8_UNORM`, consider passing the format to `build()`.

## Performance Considerations

- Each `newDescriptorSet()` call allocates from the per-frame pool, which is flushed automatically. No manual cleanup needed.
- Pipelines are created once and reused. No per-frame pipeline creation overhead.
- LUT textures are small and static. No memory pressure.
- The three dispatches are sequential on the same command buffer. No additional synchronization is needed beyond the image layout transitions.
