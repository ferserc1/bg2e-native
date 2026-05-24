# Phase 4: DeferredLayer Header Modifications

## File

`lib/include/bg2e/render/deferred/DeferredLayer.hpp` (modify)

## Changes

### 1. Add Include (after line 22)

```cpp
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
```

### 2. Add Enum Value (after line 51, `InputImage`)

Current:
```cpp
InputImage,

// Future extra passes:
// ExtraPassRTAO,

MaxLayer
```

Modified:
```cpp
InputImage,
RTAmbientOcclusion,

MaxLayer
```

### 3. Add Member Variable (after line 122, `_isTransparent`)

```cpp
std::unique_ptr<RTAmbientOcclusion> _rtAmbientOcclusion;
```

## Summary of Modifications

| Location | Line | Change |
|----------|------|--------|
| Includes | after 22 | Add `#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>` |
| `DeferredDebugVisualization` enum | after 51 | Add `RTAmbientOcclusion,` value |
| Member variables | after 122 | Add `std::unique_ptr<RTAmbientOcclusion> _rtAmbientOcclusion;` |

## Context for Each Change

### Include Context (around line 22)

```cpp
#include <bg2e/render/deferred/RenderLayer.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>  // ← NEW
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
```

### Enum Context (around line 48-57)

```cpp
enum class DeferredDebugVisualization {
    FullComposition = 0,
    GBufferAlbedo,
    GBufferNormal,
    GBufferMaterial,
    GBufferFresnelFlags,
    GBufferSheenColor,
    GBufferDepth,
    InputImage,
    RTAmbientOcclusion,  // ← NEW

    MaxLayer
};
```

### Member Context (around line 120-125)

```cpp
bool _isTransparent = false;
std::unique_ptr<RTAmbientOcclusion> _rtAmbientOcclusion;  // ← NEW
```
