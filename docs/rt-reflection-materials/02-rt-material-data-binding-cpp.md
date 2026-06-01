# Step 2: Create RTMaterialDataBinding.cpp

## File Path

`lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp`

## Purpose

Implement the `RTMaterialDataBinding` class: layout creation with 4 array bindings, frame resource allocation, and per-frame descriptor set creation with material data, vertex/index buffers, and albedo textures.

## Dependencies / Includes

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/Texture.hpp>
```

## Implementation

### Constructor

```cpp
RTMaterialDataBinding::RTMaterialDataBinding(Engine* engine)
    : PipelineDataBinding(engine)
{
}
```

### initFrameResources

Request pool capacity for the descriptor types used by this binding.

```cpp
void RTMaterialDataBinding::initFrameResources(DescriptorSetAllocator* frameAllocator)
{
    // Ratio = number of descriptors of this type per set
    // SSBO: 1 (material data array) + MAX_OBJECTS (vertex array) + MAX_OBJECTS (index array)
    // CIS:  MAX_OBJECTS (albedo texture array)
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 + 2 * MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS }
    });
}
```

**How the allocator works**: `requirePoolSizeRatio(maxSets, ratios)` stores ratios. In `initPool()`, pool descriptor count = `ratio * maxSets`. With `maxSets=1`, the pool gets exactly `ratio` descriptors of each type. The pool auto-grows when exhausted (via `getPool()`).

### createLayout

Create the descriptor set layout with 4 array bindings using the `factory::DescriptorSetLayout` helper.

```cpp
VkDescriptorSetLayout RTMaterialDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        factory::DescriptorSetLayout dsLayoutFactory;
        dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // count=1 (default)
        dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS);           // count=256
        dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS);           // count=256
        dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS);   // count=256
        _layout = dsLayoutFactory.build(_engine->device().handle(), shaderStages);
    }
    return _layout;
}
```

**Note**: The `factory::DescriptorSetLayout::addBinding()` method now accepts an optional `descriptorCount` parameter (default=1). This is used for the array bindings (bindings 1-3) that need `descriptorCount = MAX_OBJECTS`.

### newDescriptorSet

Create the per-frame descriptor set with all material data and array bindings.

```cpp
VkDescriptorSet RTMaterialDataBinding::newDescriptorSet(
    FrameResources& frameResources,
    const std::vector<RTObjectInstance>& objectInstances
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("RTMaterialDataBinding::newDescriptorSet() - Layout not created");
    }

    // Step 1: Extract RTMaterialData array and create SSBO
    std::vector<RTMaterialData> materialData;
    materialData.reserve(objectInstances.size());
    for (const auto& inst : objectInstances)
    {
        materialData.push_back(inst.materialData);
    }

    // Handle empty case: create a single-element buffer with default data
    if (materialData.empty())
    {
        materialData.push_back(RTMaterialData{});
    }

    auto* materialBuffer = macros::createBuffer(
        _engine, frameResources, materialData,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        "RTMaterialDataBinding: material data SSBO"
    );

    // Step 2: Allocate descriptor set
    auto ds = frameResources.newDescriptorSet(_layout);

    // Step 3: Update descriptor set
    ds->beginUpdate();

    // Binding 0: material data SSBO (count = 1)
    ds->addBuffer(
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        materialBuffer, materialData.size() * sizeof(RTMaterialData), 0
    );

    // Step 4: Array bindings for vertex buffer, index buffer, and albedo texture
    auto whiteTex = Texture::whiteTexture(_engine);

    // Prepare arrays for bindings 1, 2, 3
    // We need to fill all MAX_OBJECTS slots, using actual data where available
    // and dummy/fallback values for unused slots

    // Vertex buffer array (binding 1)
    {
        std::vector<VkBuffer> vertexBuffers(MAX_OBJECTS, VK_NULL_HANDLE);
        std::vector<VkDeviceSize> bufferSizes(MAX_OBJECTS, 0);
        std::vector<VkDeviceSize> bufferOffsets(MAX_OBJECTS, 0);

        for (uint32_t i = 0; i < MAX_OBJECTS; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].vertexBuffer)
            {
                vertexBuffers[i] = objectInstances[i].vertexBuffer->handle();
                bufferSizes[i] = objectInstances[i].vertexBuffer->size();
            }
            else
            {
                // Dummy buffer for unused slots
                vertexBuffers[i] = _dummyBuffer->handle();
                bufferSizes[i] = 0;
            }
        }

        ds->addBufferArray(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffers, bufferSizes, bufferOffsets);
    }

    // Index buffer array (binding 2)
    {
        std::vector<VkBuffer> indexBuffers(MAX_OBJECTS, VK_NULL_HANDLE);
        std::vector<VkDeviceSize> bufferSizes(MAX_OBJECTS, 0);
        std::vector<VkDeviceSize> bufferOffsets(MAX_OBJECTS, 0);

        for (uint32_t i = 0; i < MAX_OBJECTS; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].indexBuffer)
            {
                indexBuffers[i] = objectInstances[i].indexBuffer->handle();
                bufferSizes[i] = objectInstances[i].indexBuffer->size();
            }
            else
            {
                indexBuffers[i] = _dummyBuffer->handle();
                bufferSizes[i] = 0;
            }
        }

        ds->addBufferArray(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffers, bufferSizes, bufferOffsets);
    }

    // Albedo texture array (binding 3)
    {
        std::vector<VkImageView> imageViews(MAX_OBJECTS);
        std::vector<VkSampler> samplers(MAX_OBJECTS);

        for (uint32_t i = 0; i < MAX_OBJECTS; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].albedoTexture)
            {
                imageViews[i] = objectInstances[i].albedoTexture->image()->imageView();
                samplers[i] = objectInstances[i].albedoTexture->sampler();
            }
            else
            {
                imageViews[i] = whiteTex->image()->imageView();
                samplers[i] = whiteTex->sampler();
            }
        }

        ds->addImageArray(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageViews, samplers);
    }

    ds->endUpdate();
    return ds->descriptorSet();
}
```

## Important Implementation Notes

1. **Array bindings**: The key change is using `addBufferArray` and `addImageArray` to create array descriptor writes with `descriptorCount = MAX_OBJECTS`. The shader uses `nonuniformEXT()` to index into these arrays.

2. **Dummy buffer**: For unused slots (i >= objectInstances.size()), we bind a small dummy buffer (1 byte) for vertex/index bindings to avoid null buffer issues. The albedo texture uses the white texture fallback.

3. **White texture**: `Texture::whiteTexture(_engine)` creates a 2x2 white texture on first call. It's registered with `cleanupManager` for automatic cleanup. Safe to use for fallback bindings.

4. **Empty object list**: If `objectInstances` is empty, we still create a 1-element SSBO with default data. This avoids a zero-sized buffer exception from `macros::createBuffer`.

5. **Buffer size access**: `inst.vertexBuffer->size()` and `inst.indexBuffer->size()` - verify that `Buffer` has a `size()` method. If not, the size must be tracked differently (e.g., stored in `RTObjectInstance`). Check `lib/include/bg2e/render/vulkan/Buffer.hpp` for the available API.

6. **Memory**: All buffers created via `macros::createBuffer(vector)` are automatically cleaned up by `FrameResources::cleanupManager`. No manual cleanup needed.

7. **DescriptorSet API**: The `DescriptorSet` class has `addBufferArray` and `addImageArray` methods that create a single `VkWriteDescriptorSet` with `descriptorCount = N` pointing to a contiguous array of descriptor infos. This is cleaner than using `vkUpdateDescriptorSets` directly.