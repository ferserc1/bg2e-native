# RT Reflections Material Data Binding - Implementation Plan

## Overview

Create a `RTMaterialDataBinding` class that builds a per-frame Vulkan descriptor set containing material data (albedo color, albedo scale) and mesh buffers (vertex + index) for all ray-tracing visible objects. This descriptor set is consumed by the closest hit shader (`rt_reflections.rchit.glsl`) to reconstruct the final albedo color of reflected surfaces.

## Architecture

### Key Design Decisions

1. **MAX_OBJECTS = 256**: A global `maxRayTracingObjects` attribute on `Engine` controls the maximum number of objects for ray tracing. This caps TLAS instances, materials, vertex buffers, index buffers, and albedo textures at the same value.

2. **Unified object index**: The material index, vertex buffer index, index buffer index, and albedo texture index all equal the object index. This is guaranteed by `CollectRayTracingInstancesVisitor` which assigns sequential indices. No need to pass separate indices.

3. **Fallback texture**: If an object has no albedo texture, `MaterialBase` automatically returns a global white texture fallback. We can assume every object always has a valid albedo texture.

4. **Array descriptor bindings**: Instead of 3*MAX_OBJECTS+1 individual bindings (769 for MAX=256), we use 4 array bindings:
   - binding 0: SSBO, count=1 (material data array)
   - binding 1: SSBO, count=MAX_OBJECTS (vertex buffer array)
   - binding 2: SSBO, count=MAX_OBJECTS (index buffer array)
   - binding 3: CIS, count=MAX_OBJECTS (albedo texture array)

5. **Non-uniform descriptor indexing**: The shader uses `GL_EXT_nonuniform_qualifier` to dynamically index into the buffer/texture arrays using `gl_InstanceCustomIndexEXT`.

### Data Flow

```
CollectRayTracingInstancesVisitor
  ├── For each RT-visible object i (capped at engine->maxRayTracingObjects()):
  │   ├── objectInstances[i].materialData = { albedo color, albedoScale }
  │   ├── objectInstances[i].vertexBuffer = renderMesh->vertexBuffer()
  │   ├── objectInstances[i].indexBuffer = renderMesh->indexBuffer()
  │   └── objectInstances[i].albedoTexture = renderMat->albedoTexture()
  ├── Sets instanceCustomIndex = i (object index)
  └── Capped at engine->maxRayTracingObjects() (default 256)
                     │
                     ▼
RayTracingScene
  ├── Stores TLAS (with instanceCustomIndex per instance)
  ├── Stores _objectInstances (NEW: material data + vertex/index buffers + textures)
  └── Accessible via FrameResources::rayTracingScene
                     │
                     ▼
RTMaterialDataBinding (NEW)
  ├── createLayout(): descriptor set layout with 4 array bindings
  ├── initFrameResources(): pool size ratios for SSBO + CIS
  └── newDescriptorSet(): creates material SSBO + binds vertex/index/texture arrays
                     │
                     ▼
RTReflections
  ├── Pipeline layout: set 0 (existing) + set 1 (material data binding)
  └── render(): creates material descriptor set, binds it as set 1
                     │
                     ▼
Shader (rt_reflections.rchit.glsl)
  ├── gl_InstanceCustomIndexEXT → object/material index
  ├── vb[matIdx].vertices[...] → vertex data
  ├── ib[matIdx].indices[...] → index data
  ├── albedoTex[matIdx] → sample texture at interpolated UV
  └── materials[matIdx].albedo → albedo color multiplier
```

### Descriptor Set Layout (set 1)

| Binding | Type | Descriptor Count | Content |
|---------|------|-----------------|---------|
| 0 | SSBO | 1 | `RTMaterialData[]` (albedo + albedoScale per object) |
| 1 | SSBO | MAX_OBJECTS (256) | Vertex buffer array |
| 2 | SSBO | MAX_OBJECTS (256) | Index buffer array |
| 3 | CIS | MAX_OBJECTS (256) | Albedo texture array |

### Engine Global Attribute

```cpp
// In Engine class
inline void setMaxRayTracingObjects(uint32_t max) { _maxRayTracingObjects = max; }
inline uint32_t maxRayTracingObjects() const { return _maxRayTracingObjects; }
uint32_t _maxRayTracingObjects = 256;
```

All ray tracing systems respect this limit:
- `CollectRayTracingInstancesVisitor` stops collecting at `maxRayTracingObjects`
- `RTMaterialDataBinding` uses `MAX_OBJECTS = 256` for array binding counts
- TLAS instances, materials, buffers, and textures are all capped at the same value

## Files to Create

| File | Description |
|------|-------------|
| `lib/include/bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp` | Header for the new data binding class |
| `lib/src/bg2e/render/vulkan/rt/RTMaterialDataBinding.cpp` | Implementation of the data binding class |
| `shaders/src/lib/rt_material_data.glsl` | GLSL include with RTMaterialData and RTVertex structs |

## Files to Modify

| File | Changes |
|------|---------|
| `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h` | Add `RTObjectInstance` struct (material data + vertex/index buffer + texture) |
| `lib/include/bg2e/render/Engine.hpp` | Add `maxRayTracingObjects()` getter and `_maxRayTracingObjects` member |
| `lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp` | Add `_objectInstances` member + accessor |
| `lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp` | Check `engine->maxRayTracingObjects()` cap, store full object instances |
| `lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp` | Add `_objectInstances` member + accessor |
| `lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp` | Store object instances from visitor |
| `lib/include/bg2e/render/deferred/RTReflections.hpp` | Add `_materialDataBinding` member + setter |
| `lib/src/bg2e/render/deferred/RTReflections.cpp` | Integrate material data binding into pipeline and render |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | Add `_rtMaterialDataBinding` member |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Create binding, pass to RTReflections, pass instances at render |
| `shaders/src/rt_reflections.rchit.glsl` | Use material data, vertex/index buffers, albedo texture with array bindings |

## Implementation Steps

1. **Step 1**: Create `RTMaterialDataBinding.hpp` - class header with 4 array bindings
2. **Step 2**: Create `RTMaterialDataBinding.cpp` - layout creation with array bindings, descriptor set updates
3. **Step 3**: Modify `Engine` - add `maxRayTracingObjects()` global attribute
4. **Step 4**: Modify `RTMaterialData.h` - add `RTObjectInstance` struct
5. **Step 5**: Modify `CollectRayTracingInstancesVisitor` - cap at maxRayTracingObjects, store full object instances
6. **Step 6**: Modify `RayTracingScene` - store object instances from visitor
7. **Step 7**: Modify `RTReflections` - integrate material data binding with array descriptors
8. **Step 8**: Modify `DeferredLayer` - create binding, wire everything together
9. **Step 9**: Create `rt_material_data.glsl` - GLSL structs
10. **Step 10**: Modify `rt_reflections.rchit.glsl` - use array bindings in shader