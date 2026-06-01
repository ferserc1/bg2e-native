# RT Reflections Material Data Binding - Implementation Plan

## Overview

Create a `RTMaterialDataBinding` class that builds a per-frame Vulkan descriptor set containing material data (albedo color, albedo scale, albedo texture) and mesh buffers (vertex + index) for all ray-tracing visible materials. This descriptor set is consumed by the closest hit shader (`rt_reflections.rchit.glsl`) to reconstruct the final albedo color of reflected surfaces.

## Architecture

### Data Flow

```
CollectRayTracingInstancesVisitor
  ├── Collects RTMaterialInstance[] (albedo color, scale, vertex buffer, index buffer, albedo texture)
  ├── Sets instanceCustomIndex = material index in TLAS instances
  └── Used by RayTracingScene::update() to build TLAS
                    │
                    ▼
RayTracingScene
  ├── Stores TLAS (with instanceCustomIndex per instance)
  ├── Stores _materialInstances (NEW: previously discarded)
  └── Accessible via FrameResources::rayTracingScene
                    │
                    ▼
RTMaterialDataBinding (NEW)
  ├── createLayout(): descriptor set layout with 3*MAX_MATERIALS+1 bindings
  ├── initFrameResources(): pool size ratios for SSBO + CIS
  └── newDescriptorSet(): creates SSBO + binds vertex/index/texture per material
                    │
                    ▼
RTReflections
  ├── Pipeline layout: set 0 (existing) + set 1 (material data binding)
  └── render(): creates material descriptor set, binds it as set 1
                    │
                    ▼
Shader (rt_reflections.rchit.glsl)
  ├── gl_InstanceCustomIndexEXT → material index
  ├── materials[matIdx] → albedo color + albedoScale
  ├── vertices[matIdx] / indices[matIdx] → UV recovery from barycentrics
  └── albedoTex[matIdx] → sample texture at interpolated UV
```

### Descriptor Set Layout (set 1)

| Binding | Type | Content |
|---------|------|---------|
| 0 | SSBO | `RTMaterialData[]` (albedo + albedoScale per material) |
| 1 | SSBO | Vertex buffer for material 0 |
| 2 | SSBO | Index buffer for material 0 |
| 3 | CIS | Albedo texture for material 0 |
| 4 | SSBO | Vertex buffer for material 1 |
| 5 | SSBO | Index buffer for material 1 |
| 6 | CIS | Albedo texture for material 1 |
| ... | ... | ... up to MAX_MATERIALS (64) |

### Key Design Decisions

1. **Pre-allocated layout**: `VkDescriptorSetLayout` is created once with `3 * MAX_MATERIALS + 1 = 193` bindings. Unused bindings are filled with white texture + small dummy buffers.

2. **Pool auto-growth**: `initFrameResources` uses `requirePoolSizeRatio(1, ...)` with ratios equal to descriptor counts per set. The allocator handles pool growth automatically.

3. **No RT support checks**: `DeferredLayer` gates RT features. The data binding class assumes ray tracing is available.

4. **Material index mapping**: `gl_InstanceCustomIndexEXT` in the shader directly indexes into the material data buffer. This is already set up in `CollectRayTracingInstancesVisitor`.

## Files to Create

| File | Description |
|------|-------------|
| `lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp` | Header for the new data binding class |
| `lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp` | Implementation of the data binding class |
| `shaders/src/lib/rt_material_data.glsl` | GLSL include with RTMaterialData and RTVertex structs |

## Files to Modify

| File | Changes |
|------|---------|
| `lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp` | Add `_materialInstances` member + accessor |
| `lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp` | Store material instances from visitor |
| `lib/include/bg2e/render/deferred/RTReflections.hpp` | Add `_materialDataBinding` member + setter |
| `lib/src/bg2e/render/deferred/RTReflections.cpp` | Integrate material data binding into pipeline and render |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | Add `_rtMaterialDataBinding` member |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Create binding, pass to RTReflections, pass instances at render |
| `shaders/src/rt_reflections.rchit.glsl` | Use material data, vertex/index buffers, albedo texture |

## Implementation Steps

1. **Step 1**: Create `RTMaterialDataBinding.hpp` - class header
2. **Step 2**: Create `RTMaterialDataBinding.cpp` - layout creation, buffer creation, descriptor set updates
3. **Step 3**: Modify `RayTracingScene` - store material instances from visitor
4. **Step 4**: Modify `RTReflections` - integrate material data binding
5. **Step 5**: Modify `DeferredLayer` - create binding, wire everything together
6. **Step 6**: Create `rt_material_data.glsl` - GLSL structs
7. **Step 7**: Modify `rt_reflections.rchit.glsl` - use material data in shader
