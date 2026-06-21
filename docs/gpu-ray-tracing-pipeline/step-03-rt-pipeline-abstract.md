# Step 03: Create RayTracingPipeline Abstract Class

## Goal

Define the common `gpu::RayTracingPipeline` interface and description struct, following the same pattern as `ComputePipeline`.

## Files to Create

### `lib/include/bg2e/gpu/RayTracingPipeline.hpp` (new)

```cpp
/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/ShaderModule.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>

#include <string>

namespace bg2e {
namespace gpu {

struct RayTracingPipelineDescription {
    gpu::ShaderModule*   raygenShader     = nullptr; // non-owning; stage must be RayGeneration
    gpu::ShaderModule*   missShader       = nullptr; // non-owning; stage must be Miss (nullable on Metal)
    gpu::ShaderModule*   closestHitShader = nullptr; // non-owning; stage must be ClosestHit (nullable on Metal)
    gpu::PipelineLayout* layout           = nullptr; // non-owning; caller must keep alive
    uint32_t             maxRecursionDepth = 1;      // Vulkan max ray recursion depth
    std::string          debugName;
};

class BG2E_API RayTracingPipeline : public DeviceResource {
public:
    explicit RayTracingPipeline(Device* device) : DeviceResource(device) {}
    virtual ~RayTracingPipeline() = default;
};

}
}
```

## Files to Modify

### `lib/include/bg2e/gpu/Device.hpp`

1. Add forward declaration (after `class RayTracingScene;`):
```cpp
class RayTracingPipeline;
```

2. Add include (after `#include <bg2e/gpu/ComputePipeline.hpp>`):
```cpp
#include <bg2e/gpu/RayTracingPipeline.hpp>
```

3. Add factory method (after `createRayTracingScene()`):
```cpp
virtual std::shared_ptr<RayTracingPipeline> createRayTracingPipeline(
    const RayTracingPipelineDescription& description)
{
    throw std::runtime_error("createRayTracingPipeline not implemented");
}
```

### `lib/include/bg2e/gpu/all.hpp`

Add include after `ComputePipeline.hpp`:
```cpp
#include <bg2e/gpu/RayTracingPipeline.hpp>
```

## Design Rationale

- `RayTracingPipelineDescription` mirrors `ComputePipelineDescription` and `GraphicsPipelineDescription` in using non-owning raw pointers for shader modules and layout.
- `missShader` and `closestHitShader` are nullable — on Metal they will be null and the implementation must accept this.
- `maxRecursionDepth` is Vulkan-specific but exposed in the common description for API completeness (Metal ignores it).
- The class inherits `DeviceResource` for consistent cleanup and device tracking.

## Integration Points

- `Device::createRayTracingPipeline()` follows the same factory pattern as all other resource creation methods.
- Backend implementations (Steps 05, 06) will dynamic-cast the description's pointers to backend types.

## Verification

Engine compiles. Abstract class exists but no backend implementation yet — calling `createRayTracingPipeline()` throws.
