# Step 02: Extend ShaderLib with RT Shader Loading

## Goal

Add `rayGeneration()`, `miss()`, `closestHit()` methods to `ShaderLib`. On Metal, return `nullptr` for missing miss/closestHit files instead of throwing.

## Files to Modify

### `lib/include/bg2e/gpu/ShaderLib.hpp`

Add three new public methods after `compute()`:

```cpp
std::shared_ptr<ShaderModule> rayGeneration(
    const std::string& shaderName,
    Device* device,
    const std::string& debugName = ""
);

std::shared_ptr<ShaderModule> miss(
    const std::string& shaderName,
    Device* device,
    const std::string& debugName = ""
);

std::shared_ptr<ShaderModule> closestHit(
    const std::string& shaderName,
    Device* device,
    const std::string& debugName = ""
);
```

Add a new private method for nullable loading:

```cpp
std::shared_ptr<ShaderModule> _loadOrNull(
    const std::string& shaderName,
    const std::string& stageExt,
    ShaderStage        stage,
    const std::string& debugName,
    Device*            device);
```

### `lib/src/bg2e/gpu/ShaderLib.cpp`

**1. Add `#include <filesystem>`** at the top (already present).

**2. Implement `_loadOrNull()`**:

```cpp
std::shared_ptr<ShaderModule> ShaderLib::_loadOrNull(
    const std::string& shaderName,
    const std::string& stageExt,
    ShaderStage        stage,
    const std::string& debugName,
    Device*            device)
{
    std::string ext = (_backendType == BackendType::Vulkan) ? ".spv" : ".metallib";
    auto filePath = _basePath / (shaderName + "." + stageExt + ext);

    // On Metal, RT miss/closestHit shaders may not exist — return nullptr
    if (_backendType == BackendType::Metal)
    {
        if (!std::filesystem::exists(filePath))
        {
            return nullptr;
        }
    }

    std::string entryPoint = (_backendType == BackendType::Vulkan)
        ? "main"
        : stageExt + "Main";
    return device->createShaderModule({
        filePath.string(), entryPoint, stage, debugName
    });
}
```

**3. Implement `rayGeneration()`, `miss()`, `closestHit()`**:

```cpp
std::shared_ptr<ShaderModule> ShaderLib::rayGeneration(
    const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        "rgen",
        ShaderStage::RayGeneration,
        debugName.empty() ? shaderName + " ray generation shader" : debugName,
        device
    );
}

std::shared_ptr<ShaderModule> ShaderLib::miss(
    const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _loadOrNull(
        shaderName,
        "rmiss",
        ShaderStage::Miss,
        debugName.empty() ? shaderName + " miss shader" : debugName,
        device
    );
}

std::shared_ptr<ShaderModule> ShaderLib::closestHit(
    const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _loadOrNull(
        shaderName,
        "rchit",
        ShaderStage::ClosestHit,
        debugName.empty() ? shaderName + " closest hit shader" : debugName,
        device
    );
}
```

**Key design**: `rayGeneration()` uses `_load()` (throws on failure — the rgen shader is always required). `miss()` and `closestHit()` use `_loadOrNull()` (returns nullptr on Metal when the file doesn't exist).

## File Resolution Summary

| Method | Stage Ext | Vulkan Entry | Metal Entry | Vulkan File | Metal File |
|--------|-----------|-------------|-------------|-------------|------------|
| `rayGeneration()` | `rgen` | `main` | `rgenMain` | `.rgen.spv` | `.rgen.metallib` |
| `miss()` | `rmiss` | `main` | `rmissMain` | `.rmiss.spv` | `.rmiss.metallib` (nullable) |
| `closestHit()` | `rchit` | `main` | `rchitMain` | `.rchit.spv` | `.rchit.metallib` (nullable) |

## Integration Points

- The CMake `compile_shaders_shaderlib()` function already handles `.rgen`, `.rmiss`, `.rchit` extensions with correct entry points.
- Shader modules created by these methods are standard `ShaderModule` objects — they work with `PipelineLayout`, `ResourceSet`, etc.
- The Metal null-safety is critical: Metal shaders handle miss/hit behavior internally in the compute kernel.

## Verification

Engine compiles. ShaderLib can load RT shader modules. Metal gracefully returns nullptr for missing miss/closestHit files.
