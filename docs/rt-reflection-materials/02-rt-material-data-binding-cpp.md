# Step 2: Create RTMaterialDataBinding.cpp

## File Path

`lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp`

## Purpose

Implement the `RTMaterialDataBinding` class: layout creation, frame resource allocation, and per-frame descriptor set creation with material data, vertex/index buffers, and albedo textures.

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
    // SSBO: 1 (material data) + 2*MAX_MATERIALS (vertex + index per material)
    // CIS:  MAX_MATERIALS (albedo texture per material)
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 + 2 * MAX_MATERIALS },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_MATERIALS }
    });
}
```

**How the allocator works**: `requirePoolSizeRatio(maxSets, ratios)` stores ratios. In `initPool()`, pool descriptor count = `ratio * maxSets`. With `maxSets=1`, the pool gets exactly `ratio` descriptors of each type. The pool auto-grows when exhausted (via `getPool()`).

### createLayout

Create the descriptor set layout with all bindings. Since `factory::DescriptorSetLayout::addBinding()` always sets `descriptorCount = 1`, we can use it for binding 0 (single SSBO). For the per-material bindings (1..3*MAX_MATERIALS), we build the `VkDescriptorSetLayout` manually.

```cpp
VkDescriptorSetLayout RTMaterialDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        // Build bindings array manually
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        // Binding 0: material data SSBO
        {
            VkDescriptorSetLayoutBinding b = {};
            b.binding = 0;
            b.descriptorCount = 1;
            b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            b.stageFlags = shaderStages;
            bindings.push_back(b);
        }

        // Per-material bindings: vertex buffer (SSBO), index buffer (SSBO), albedo texture (CIS)
        for (uint32_t i = 0; i < MAX_MATERIALS; ++i)
        {
            uint32_t base = 1 + i * 3;

            // Vertex buffer SSBO
            {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = base;
                b.descriptorCount = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.stageFlags = shaderStages;
                bindings.push_back(b);
            }

            // Index buffer SSBO
            {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = base + 1;
                b.descriptorCount = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.stageFlags = shaderStages;
                bindings.push_back(b);
            }

            // Albedo texture CIS
            {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = base + 2;
                b.descriptorCount = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.stageFlags = shaderStages;
                bindings.push_back(b);
            }
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VK_ASSERT(vkCreateDescriptorSetLayout(
            _engine->device().handle(), &layoutInfo, nullptr, &_layout
        ));
    }
    return _layout;
}
```

**Note**: We build the layout manually instead of using the factory because the factory's `addBinding()` doesn't support creating many bindings efficiently. The manual approach is clearer for 193 bindings.

### newDescriptorSet

Create the per-frame descriptor set with all material data.

```cpp
VkDescriptorSet RTMaterialDataBinding::newDescriptorSet(
    FrameResources& frameResources,
    const std::vector<RTMaterialInstance>& materialInstances
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("RTMaterialDataBinding::newDescriptorSet() - Layout not created");
    }

    // Step 1: Extract RTMaterialData array and create SSBO
    std::vector<RTMaterialData> materialData;
    materialData.reserve(materialInstances.size());
    for (const auto& inst : materialInstances)
    {
        materialData.push_back(inst.data);
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

    // Binding 0: material data SSBO
    ds->addBuffer(
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        materialBuffer, materialData.size() * sizeof(RTMaterialData), 0
    );

    // Step 4: Per-material bindings
    auto whiteTex = Texture::whiteTexture(_engine);

    for (uint32_t i = 0; i < MAX_MATERIALS; ++i)
    {
        uint32_t base = 1 + i * 3;

        if (i < materialInstances.size())
        {
            const auto& inst = materialInstances[i];

            // Vertex buffer
            if (inst.vertexBuffer)
            {
                ds->addBuffer(
                    base, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    inst.vertexBuffer->handle(),
                    inst.vertexBuffer->size(), 0
                );
            }

            // Index buffer
            if (inst.indexBuffer)
            {
                ds->addBuffer(
                    base + 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    inst.indexBuffer->handle(),
                    inst.indexBuffer->size(), 0
                );
            }

            // Albedo texture (fallback to white if null)
            if (inst.albedoTexture)
            {
                ds->addImage(
                    base + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    inst.albedoTexture->image()->imageView(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    inst.albedoTexture->sampler()
                );
            }
            else
            {
                ds->addImage(
                    base + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    whiteTex->image()->imageView(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    whiteTex->sampler()
                );
            }
        }
        else
        {
            // Unused slot: bind white texture (dummy buffers not strictly needed
            // if shader uses correct material count, but we bind them for safety)
            ds->addImage(
                base + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                whiteTex->image()->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                whiteTex->sampler()
            );
        }
    }

    ds->endUpdate();
    return ds->descriptorSet();
}
```

## Important Implementation Notes

1. **Buffer size access**: `inst.vertexBuffer->size()` and `inst.indexBuffer->size()` - verify that `Buffer` has a `size()` method. If not, the size must be tracked differently (e.g., stored in `RTMaterialInstance`). Check `lib/include/bg2e/render/vulkan/Buffer.hpp` for the available API.

2. **White texture**: `Texture::whiteTexture(_engine)` creates a 2x2 white texture on first call. It's registered with `cleanupManager` for automatic cleanup. Safe to use for fallback bindings.

3. **Empty material list**: If `materialInstances` is empty, we still create a 1-element SSBO with default data. This avoids a zero-sized buffer exception from `macros::createBuffer`.

4. **Dummy buffers for unused slots**: For unused material slots (i >= N), we only bind the white texture. The SSBO bindings (vertex/index) are left uninitialized - the shader must use `materialInstances.size()` (passed via push constant or uniform) to avoid accessing invalid slots. Alternatively, bind a small dummy buffer. The choice depends on the shader implementation.

5. **Memory**: All buffers created via `macros::createBuffer(vector)` are automatically cleaned up by `FrameResources::cleanupManager`. No manual cleanup needed.
