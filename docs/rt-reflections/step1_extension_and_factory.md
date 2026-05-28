# Step 1: Extension Function + Ray Tracing Pipeline Factory — Revised

## Objective

Add the missing `vkGetRayTracingShaderGroupHandlesKHR` extension function and create a `RayTracingPipeline` factory class following the existing factory pattern.

This revised version fixes two important design details:

1. The factory stores the created `VkPipeline` internally so `createSBT()` can call `vkGetRayTracingShaderGroupHandlesKHR()` without requiring an external pipeline parameter.
2. SBT ownership is represented with the existing `vulkan::Buffer` wrapper instead of raw `VkBuffer + VmaAllocation`, so the returned SBT data matches the style expected by `RTReflections`.

## Why This Is Needed

The codebase loads `vkCreateRayTracingPipelinesKHR` and `vkCmdTraceRaysKHR`, but not `vkGetRayTracingShaderGroupHandlesKHR`, which is required to retrieve shader group handles for SBT creation.

There is also no `RayTracingPipeline` factory class. Only `GraphicsPipeline` and `ComputePipeline` exist.

## Files to Create

### `lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp`

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>

#include <memory>
#include <string>
#include <vector>

namespace bg2e::render::vulkan::factory {

class BG2E_API RayTracingPipeline {
public:
    explicit RayTracingPipeline(Engine* engine);
    ~RayTracingPipeline();

    void setRayGenShader(const std::string& fileName, const std::string& entryPoint = "main");
    void setMissShader(const std::string& fileName, const std::string& entryPoint = "main");
    void setClosestHitShader(const std::string& fileName, const std::string& entryPoint = "main");

    VkPipeline build(
        VkPipelineLayout layout,
        uint32_t maxRecursionDepth = 1,
        const std::string& name = ""
    );

    struct SBTData {
        std::unique_ptr<vulkan::Buffer> buffer;
        VkStridedDeviceAddressRegionKHR raygenRegion = {};
        VkStridedDeviceAddressRegionKHR missRegion = {};
        VkStridedDeviceAddressRegionKHR hitRegion = {};
        VkStridedDeviceAddressRegionKHR callableRegion = {};
    };

    SBTData createSBT(const std::string& name = "");

    VkPipeline pipeline() const { return _pipeline; }
    uint32_t groupCount() const { return static_cast<uint32_t>(_groups.size()); }

private:
    Engine* _engine = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;

    std::vector<VkPipelineShaderStageCreateInfo> _stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> _groups;
    std::vector<VkShaderModule> _shaderModules;

    uint32_t _raygenGroupIndex = 0;
    uint32_t _missGroupIndex = 0;
    uint32_t _hitGroupIndex = 0;

    uint32_t addShaderStage(
        VkShaderStageFlagBits stage,
        const std::string& fileName,
        const std::string& entryPoint
    );

    static VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment);
};

}
```

## Implementation Details

### `lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp`

Implementation requirements:

- Load shader modules using the existing shader module helper used by `GraphicsPipeline` and `ComputePipeline`.
- Each `set*Shader()` call must add:
  - one `VkPipelineShaderStageCreateInfo`
  - one matching `VkRayTracingShaderGroupCreateInfoKHR`
- `build()` must store the created pipeline in `_pipeline` and return it.
- `createSBT()` must require `_pipeline != VK_NULL_HANDLE`.
- `createSBT()` must call `vulkan::getRayTracingShaderGroupHandles(...)` using the stored `_pipeline`.
- `createSBT()` must return `SBTData` with a `std::unique_ptr<vulkan::Buffer>` and the four `VkStridedDeviceAddressRegionKHR` structs.
- `_pipeline` is only stored to be used in `createSBT()`. Do not destroy the pipeline in `RayTracingPipeline`; pipeline lifetime is owned by the caller.

### Shader Groups

Expected group layout for the first implementation:

```text
Group 0: Ray generation
Group 1: Miss
Group 2: Closest hit
```

Ray generation group:

```cpp
VkRayTracingShaderGroupCreateInfoKHR group{};
group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
group.generalShader = raygenStageIndex;
group.closestHitShader = VK_SHADER_UNUSED_KHR;
group.anyHitShader = VK_SHADER_UNUSED_KHR;
group.intersectionShader = VK_SHADER_UNUSED_KHR;
```

Miss group:

```cpp
group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
group.generalShader = missStageIndex;
```

Closest-hit group:

```cpp
group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
group.generalShader = VK_SHADER_UNUSED_KHR;
group.closestHitShader = closestHitStageIndex;
group.anyHitShader = VK_SHADER_UNUSED_KHR;
group.intersectionShader = VK_SHADER_UNUSED_KHR;
```

### `build()`

`build()` must use:

```cpp
VkRayTracingPipelineCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
createInfo.stageCount = static_cast<uint32_t>(_stages.size());
createInfo.pStages = _stages.data();
createInfo.groupCount = static_cast<uint32_t>(_groups.size());
createInfo.pGroups = _groups.data();
createInfo.maxPipelineRayRecursionDepth = maxRecursionDepth;
createInfo.layout = layout;
```

Then call:

```cpp
vulkan::createRayTracingPipelines(
    device,
    VK_NULL_HANDLE,
    VK_NULL_HANDLE,
    1,
    &createInfo,
    nullptr,
    &_pipeline
);
```

Return `_pipeline`.

### `createSBT()`

`createSBT()` must:

1. Query `VkPhysicalDeviceRayTracingPipelinePropertiesKHR`.
2. Read:
   - `shaderGroupHandleSize`
   - `shaderGroupHandleAlignment`
   - `shaderGroupBaseAlignment`
3. Call:

```cpp
vulkan::getRayTracingShaderGroupHandles(
    device,
    _pipeline,
    0,
    groupCount,
    handleDataSize,
    handleData.data()
);
```

4. Create a SBT buffer with:

```cpp
VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT
```

5. Allocate it in CPU-visible memory if that matches the existing buffer upload style, or use a staging upload if the engine usually does that.
6. Copy each shader group handle into its aligned SBT record.
7. Fill:

```cpp
VkStridedDeviceAddressRegionKHR raygenRegion;
VkStridedDeviceAddressRegionKHR missRegion;
VkStridedDeviceAddressRegionKHR hitRegion;
VkStridedDeviceAddressRegionKHR callableRegion;
```

Recommended layout:

```text
raygen record: 1 record, aligned to shaderGroupBaseAlignment
miss record:   1 record, aligned to shaderGroupBaseAlignment
hit record:    1 record, aligned to shaderGroupBaseAlignment
callable:      empty
```

Use `shaderGroupBaseAlignment` for region start alignment and `shaderGroupHandleAlignment` for record stride alignment.

## Files to Modify

### `lib/include/bg2e/render/vulkan/extensions.hpp`

Add:

```cpp
extern BG2E_API PFN_vkGetRayTracingShaderGroupHandlesKHR getRayTracingShaderGroupHandles;
```

### `lib/src/bg2e/render/vulkan/extensions.cpp`

Add global variable definition:

```cpp
PFN_vkGetRayTracingShaderGroupHandlesKHR getRayTracingShaderGroupHandles;
```

In `loadDeviceExtensions()`, add:

```cpp
getRayTracingShaderGroupHandles =
    reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
        loadDeviceExtension("vkGetRayTracingShaderGroupHandlesKHR")
    );
```

## Verification

After this step:

- The engine compiles and runs identically to before.
- `vkGetRayTracingShaderGroupHandlesKHR` is loaded.
- `RayTracingPipeline` can build a RT pipeline.
- `RayTracingPipeline::createSBT()` has access to the internally stored `_pipeline`.
- SBT ownership is cleanly represented by `std::unique_ptr<vulkan::Buffer>`.
- No existing render behavior changes.
