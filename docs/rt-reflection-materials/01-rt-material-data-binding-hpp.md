# Step 1: Create RTMaterialDataBinding.hpp

## File Path

`lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp`

## Purpose

Define the `RTMaterialDataBinding` class that manages a per-frame descriptor set containing material data for ray tracing closest hit shader.

## Dependencies

- `bg2e/render/vulkan/PipelineDataBinding.hpp` - base class
- `bg2e/render/vulkan/FrameResources.hpp` - frame resource management
- `bg2e/render/vulkan/rt/RTMaterialData.h` - `RTMaterialData` and `RTMaterialInstance` structs
- `bg2e/render/Texture.hpp` - for white texture fallback

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
    // Maximum number of materials supported per frame.
    // The descriptor set layout is pre-allocated with this many material slots.
    // Each material uses 3 bindings: vertex buffer (SSBO), index buffer (SSBO),
    // albedo texture (CIS). Plus 1 SSBO for the material data array.
    static constexpr uint32_t MAX_MATERIALS = 64;

    RTMaterialDataBinding(bg2e::render::Engine* engine);

    // Request descriptor pool capacity. Called once during DeferredLayer::initFrameResources().
    // Requests: 1 + 2*MAX_MATERIALS SSBO descriptors, MAX_MATERIALS CIS descriptors.
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override;

    // Create the descriptor set layout. Called once during DeferredLayer::build().
    // Creates a layout with 3*MAX_MATERIALS + 1 bindings:
    //   binding 0: SSBO (RTMaterialData array)
    //   bindings 1..3*MAX_MATERIALS: per-material vertex/index/texture bindings
    VkDescriptorSetLayout createLayout(
        VkShaderStageFlags shaderStages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    ) override;

    // Create a descriptor set for the given material instances.
    // Called once per frame during RTReflections::render().
    //
    // Steps:
    // 1. Extract RTMaterialData[] from materialInstances, create SSBO via macros::createBuffer(vector)
    // 2. Allocate descriptor set via frameResources.newDescriptorSet(_layout)
    // 3. Bind material data SSBO at binding 0
    // 4. For each material i < materialInstances.size():
    //    - Bind vertex buffer at binding 3*i + 1
    //    - Bind index buffer at binding 3*i + 2
    //    - Bind albedo texture at binding 3*i + 3
    // 5. For unused slots (i >= materialInstances.size()):
    //    - Bind dummy buffer at bindings 3*i + 1 and 3*i + 2
    //    - Bind white texture at binding 3*i + 3
    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources& frameResources,
        const std::vector<RTMaterialInstance>& materialInstances
    );
};

}
}
}
}
```

## Notes

- The class follows the same pattern as `RayTracingSceneDataBinding` and `ObjectDataBinding`
- `MAX_MATERIALS = 64` is a reasonable limit. With 3 bindings per material + 1 global, the layout has 193 bindings total. This is within typical Vulkan device limits for ray tracing capable hardware.
- The class does NOT check `_engine->rayTracingSupported()` - that's DeferredLayer's responsibility
- `newDescriptorSet` takes the material instances collected by `CollectRayTracingInstancesVisitor` (now stored in `RayTracingScene`)
