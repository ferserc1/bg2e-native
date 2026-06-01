# Step 4: Modify RTReflections to Use Material Data Binding

## Purpose

Integrate `RTMaterialDataBinding` into `RTReflections` so that:
- The pipeline layout includes the material data descriptor set layout as set 1
- The render method creates and binds the material descriptor set

## Files to Modify

- `lib/include/bg2e/render/deferred/RTReflections.hpp`
- `lib/src/bg2e/render/deferred/RTReflections.cpp`

## Changes to RTReflections.hpp

### Add includes

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialData.h>
```

### Add member variables

In the `private:` section, add:

```cpp
vulkan::rt::RTMaterialDataBinding* _materialDataBinding = nullptr;
```

### Add setter

In the `public:` section, add:

```cpp
void setMaterialDataBinding(vulkan::rt::RTMaterialDataBinding* binding) { _materialDataBinding = binding; }
```

### Modify render() signature

Add `materialInstances` parameter:

```cpp
void render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const GBufferManager* gbuffer,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition,
    VkAccelerationStructureKHR tlas,
    const std::vector<vulkan::rt::RTMaterialInstance>& materialInstances  // NEW
);
```

## Changes to RTReflections.cpp

### Modify createPipeline()

Add the material data binding layout to the pipeline layout as set 1:

```cpp
void RTReflections::createPipeline()
{
    // Existing: descriptor set layout (set 0)
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_MISS_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);

    // NEW: add material data binding layout as set 1
    if (_materialDataBinding)
    {
        layoutFactory.addDescriptorSetLayout(
            _materialDataBinding->createLayout(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        );
    }

    layoutFactory.addPushConstantRange(
        0,
        sizeof(ReflectionPushConstants),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR
    );
    _pipelineLayout = layoutFactory.build("RTReflections::PipelineLayout");

    // ... rest of pipeline creation unchanged ...
}
```

### Modify render()

Create the material descriptor set and bind it as set 1 after the main descriptor set:

```cpp
void RTReflections::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const GBufferManager* gbuffer,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition,
    VkAccelerationStructureKHR tlas,
    const std::vector<vulkan::rt::RTMaterialInstance>& materialInstances
)
{
    // ... existing early returns unchanged ...

    // ... existing: create main descriptor set (set 0) ...
    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addAccelerationStructure(0, tlas);
    // ... existing bindings 1-4 ...
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);

    // Bind set 0
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    // NEW: Create and bind material data descriptor set (set 1)
    if (_materialDataBinding && !materialInstances.empty())
    {
        auto materialDS = _materialDataBinding->newDescriptorSet(frameResources, materialInstances);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            _pipelineLayout, 1, 1, &materialDS, 0, nullptr);
    }

    // ... existing push constants and traceRays call unchanged ...
}
```

## Important Notes

1. **Set numbering**: Set 0 is the existing TLAS + output + G-buffer set. Set 1 is the new material data set. The pipeline layout must have them in this order.

2. **Null check**: `_materialDataBinding` may be null if the system doesn't support ray tracing. The existing `if (!_rtSupported)` early return in `build()` handles this, but we check anyway for safety.

3. **Empty material list**: If `materialInstances` is empty, we skip binding the material set. The shader should handle this case (e.g., use a default material).

4. **Shader stage for material layout**: We use `VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR` because only the closest hit shader accesses the material data. The existing set 0 layout uses `VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR`.

5. **Render signature change**: The `render()` method now takes an additional parameter. All callers must be updated (specifically `DeferredLayer::render()`).
