# Step 01: Extend ShaderStage and BufferUsage Enums

## Goal

Add the three required ray tracing shader stages to `ShaderStage` and add `ShaderBindingTable` to `BufferUsage`. Update all validation and conversion functions.

## Files to Modify

### `lib/include/bg2e/gpu/Common.hpp`

**1. Extend `ShaderStage` enum** (line 116-120):

```cpp
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    RayGeneration,
    Miss,
    ClosestHit
};
```

**2. Add `ShaderBindingTable` to `BufferUsage`** (after line 506):

```cpp
enum class BufferUsage : uint32_t {
    None                            = 0,
    Vertex                          = 1 << 0,
    Index                           = 1 << 1,
    Uniform                         = 1 << 2,
    Storage                         = 1 << 3,
    TransferSrc                     = 1 << 4,
    TransferDst                     = 1 << 5,
    AccelerationStructureBuildInput = 1 << 6,
    ShaderDeviceAddress             = 1 << 7,
    ShaderBindingTable              = 1 << 8
};
```

**3. Update `validatePushConstantRanges()`** — add cases for the three new stages:

```cpp
case ShaderStage::RayGeneration:
    if (hasRayGeneration)
    {
        throw std::runtime_error(
            "PipelineLayout validation failed: more than one push constant range "
            "declared for RayGeneration stage.");
    }
    hasRayGeneration = true;
    break;
case ShaderStage::Miss:
    if (hasMiss)
    {
        throw std::runtime_error(
            "PipelineLayout validation failed: more than one push constant range "
            "declared for Miss stage.");
    }
    hasMiss = true;
    break;
case ShaderStage::ClosestHit:
    if (hasClosestHit)
    {
        throw std::runtime_error(
            "PipelineLayout validation failed: more than one push constant range "
            "declared for ClosestHit stage.");
    }
    hasClosestHit = true;
    break;
```

**4. Update `validateMetalBufferBindings()`** — add cases for RT stages (Metal RT = compute, same rules as Compute):

```cpp
case ShaderStage::RayGeneration:
case ShaderStage::Miss:
case ShaderStage::ClosestHit:
    if (rb.binding.metal < 1)
    {
        throw std::runtime_error(
            "PipelineLayout validation failed: Metal ray tracing buffer resource binding "
            "uses index " + std::to_string(rb.binding.metal) + ", but ray-tracing-stage "
            "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
            "reserved for push constants.");
    }
    break;
```

### `lib/src/bg2e/gpu/vk/PipelineLayout.cpp`

**5. Update `shaderStageToVkFlags()`** (line 29-38):

```cpp
VkShaderStageFlags shaderStageToVkFlags(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:      return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:    return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:     return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderStage::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case ShaderStage::Miss:        return VK_SHADER_STAGE_MISS_BIT_KHR;
        case ShaderStage::ClosestHit:  return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    }
    return VK_SHADER_STAGE_VERTEX_BIT;
}
```

### `lib/include/bg2e/gpu/vk/common.hpp`

**6. Add RT Vulkan header include** if not already present. The file already includes `<vulkan/vulkan.h>` which provides all RT types. Verify `VK_SHADER_STAGE_RAYGEN_BIT_KHR` etc. are available.

## Integration Points

- All existing code continues to work unchanged — the new enum values don't affect existing switch statements (they fall through to defaults).
- The `shaderStageToVkFlags()` function is used by `PipelineLayout` creation and `CommandBuffer::pushConstants()`.
- The Metal validation function now correctly validates RT buffer bindings.

## Verification

Engine compiles with no functional changes. All existing examples continue to work.
