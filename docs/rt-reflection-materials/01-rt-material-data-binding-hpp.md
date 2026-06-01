# Step 1: Create RTMaterialDataBinding.hpp

## File Path

`lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp`

## Purpose

Define the `RTMaterialDataBinding` class that manages a per-frame descriptor set containing material data for ray tracing closest hit shader.

## Dependencies

- `bg2e/render/vulkan/PipelineDataBinding.hpp` - base class
- `bg2e/render/vulkan/FrameResources.hpp` - frame resource management
- `bg2e/render/vulkan/rt/RTMaterialData.h` - `RTMaterialData`, `RTObjectInstance`, and `MAX_OBJECTS` structs
- `bg2e/render/vulkan/factory/DescriptorSetLayout.hpp` - for creating the descriptor set layout with array bindings
- `bg2e/render/Texture.hpp` - for white texture fallback
- `bg2e/render/Engine.hpp` - for accessing the engine and its maxRayTracingObjects

## Class Design

```cpp
#pragma once

#include <bg2e/render/vulkan/PipelineDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialData.h>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

class BG2E_API RTMaterialDataBinding : public PipelineDataBinding {
public:
    // Maximum number of objects supported per frame.
    // This matches the global engine->maxRayTracingObjects() default of 256.
    // The descriptor set layout is pre-allocated with 4 array bindings:
    //   binding 0: SSBO (RTObjectInstance data array) - count 1
    //   binding 1: SSBO (vertex buffer array) - count MAX_OBJECTS
    //   binding 2: SSBO (index buffer array) - count MAX_OBJECTS
    //   binding 3: CIS (albedo texture array) - count MAX_OBJECTS
    static constexpr uint32_t MAX_OBJECTS = 256;

    RTMaterialDataBinding(bg2e::render::Engine* engine);

    // Request descriptor pool capacity. Called once during DeferredLayer::initFrameResources().
    // Requests: 1 SSBO for material data array + MAX_OBJECTS SSBOs for vertex array
    //          + MAX_OBJECTS SSBOs for index array + MAX_OBJECTS CIS for texture array
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override;

    // Create the descriptor set layout. Called once during DeferredLayer::build().
    // Uses factory::DescriptorSetLayout with 4 array bindings:
    //   binding 0: SSBO, count=1 (RTMaterialData array)
    //   binding 1: SSBO, count=MAX_OBJECTS (vertex buffer array)
    //   binding 2: SSBO, count=MAX_OBJECTS (index buffer array)
    //   binding 3: CIS, count=MAX_OBJECTS (albedo texture array)
    VkDescriptorSetLayout createLayout(
        VkShaderStageFlags shaderStages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    ) override;

    // Create a descriptor set for the given object instances.
    // Called once per frame during RTReflections::render().
    //
    // Steps:
    // 1. Extract RTMaterialData[] from objectInstances, create SSBO via macros::createBuffer(vector)
    // 2. Allocate descriptor set via frameResources.newDescriptorSet(_layout)
    // 3. Bind material data SSBO at binding 0
    // 4. For each slot i < MAX_OBJECTS:
    //    - If i < objectInstances.size(): bind vertex buffer at binding 1,
    //      index buffer at binding 2, albedo texture at binding 3
    //    - If i >= objectInstances.size(): bind white texture fallback + dummy buffers
    // 5. All three bindings (1, 2, 3) use writeDescriptorSet with descriptorCount = MAX_OBJECTS
    //    to create array bindings, with actual values at index i and dummy/fallback elsewhere
    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources& frameResources,
        const std::vector<RTObjectInstance>& objectInstances
    );
};

}
}
}
}
```

## Notes

- The class follows the same pattern as `RayTracingSceneDataBinding` and `ObjectDataBinding`
- `MAX_OBJECTS = 256` gives us 4 bindings total instead of 769 individual bindings
- The class does NOT check `_engine->rayTracingSupported()` - that's DeferredLayer's responsibility
- `newDescriptorSet` takes the object instances collected by `CollectRayTracingInstancesVisitor` (now stored in `RayTracingScene`)
- All indices (material, vertex buffer, index buffer, texture) share the same object index, so we use array bindings indexed by `gl_InstanceCustomIndexEXT` in the shader