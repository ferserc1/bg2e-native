# Step 5: Modify CollectRayTracingInstancesVisitor

## Purpose

Update `CollectRayTracingInstancesVisitor` to:
1. Respect the global `engine->maxRayTracingObjects()` limit
2. Store `RTObjectInstance` data (material data + vertex/index buffers + albedo texture)
3. Use object index as the unified index for all components

## Files to Modify

- `lib/include/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp`
- `lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp`

## Changes to CollectRayTracingInstancesVisitor.hpp

### Add include

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialData.h>
```

### Change member variables

Replace `_materialInstances` with `_objectInstances`:

```cpp
// OLD:
// std::vector<RTMaterialInstance> _materialInstances;

// NEW:
std::vector<RTObjectInstance> _objectInstances;
```

### Update accessor

```cpp
// OLD:
// [[nodiscard]] const std::vector<RTMaterialInstance>& materialInstances() const
// {
//     return _materialInstances;
// }

// NEW:
[[nodiscard]] const std::vector<RTObjectInstance>& objectInstances() const
{
    return _objectInstances;
}
```

## Changes to CollectRayTracingInstancesVisitor.cpp

### Modify visit() method

Add the `maxRayTracingObjects` check and store `RTObjectInstance`:

```cpp
void CollectRayTracingInstancesVisitor::visit(scene::Node * node)
{
    auto transformComponent = node->transform();
    auto drawableComp = node->drawable();
    auto drw = drawableComp ? drawableComp->drawable() : nullptr;
    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }

    if (drw && _engine->rayTracingSupported() && drw->rayTracingEnabled())
    {
        uint32_t maxObjects = _engine->maxRayTracingObjects();

        for (uint32_t i = 0; i < drw->submeshesCount() && _objectInstances.size() < maxObjects; ++i)
        {
            if (drw->submeshVisibility(i))
            {
                const auto & rtMesh = drw->rayTracingMesh(i);
                if (!rtMesh)
                {
                    std::cerr << "WARN: invalid rayTracingMesh found in submesh. Check Drawable initialization ("
                        << node->name() << " - "
                        << drw->name() << ")" << std::endl;
                    continue;
                }

                // Check if we've already reached the max object limit
                if (_objectInstances.size() >= maxObjects)
                {
                    break;
                }

                auto mat = mat4ToVkTransformMatrix(_currentTransform * drw->submeshTransform(i));
                auto renderMat = drw->renderMaterial(i);
                auto renderMesh = drw->renderMesh();

                // Store the full RTObjectInstance with all components sharing the same index
                RTObjectInstance objInst {};
                objInst.materialData.albedo = renderMat->materialAttributes().albedo();
                objInst.materialData.albedoScale = renderMat->materialAttributes().albedoScale();
                objInst.materialData.padding[0] = 0;
                objInst.materialData.padding[1] = 0;
                objInst.vertexBuffer = renderMesh->vertexBuffer();
                objInst.indexBuffer = renderMesh->indexBuffer();
                objInst.albedoTexture = renderMat->albedoTexture().get();
                _objectInstances.push_back(objInst);

                // The object index is the same for all components
                uint32_t objIndex = static_cast<uint32_t>(_objectInstances.size() - 1);
                _instances.push_back({
                    .transform = mat,
                    .instanceCustomIndex = objIndex,  // Unified object index
                    .mask = 0xFF,
                    .instanceShaderBindingTableRecordOffset = 0,
                    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
                    .accelerationStructureReference = rtMesh->deviceAddress()
                });
            }
        }
    }
}
```

## Key Changes from Old Plan

1. **Cap check**: Added `&& _objectInstances.size() < maxObjects` in the loop condition AND an explicit `break` check
2. **Store RTObjectInstance**: Instead of `RTMaterialInstance`, we store `RTObjectInstance` with all components
3. **Unified index**: The object index (`_objectInstances.size() - 1`) is used as `instanceCustomIndex`, which serves as the index for material, vertex buffer, index buffer, and texture in the shader
4. **Padding initialization**: Explicitly zero the padding fields to avoid undefined behavior

## Verification Checklist

- [ ] Visitor stops collecting at `engine->maxRayTracingObjects()` (default 256)
- [ ] `objectInstances()` returns `std::vector<RTObjectInstance>`
- [ ] `instanceCustomIndex` equals the object index (array index in `_objectInstances`)
- [ ] All four components (material data, vertex buffer, index buffer, albedo texture) are stored in the same `RTObjectInstance`

## Notes

- The visitor traverses the scene graph in order, so the first 256 RT-visible objects (in traversal order) will be included
- If a scene has more than 256 RT-visible objects, only the first 256 are used
- The albedo texture is always valid because `MaterialBase` returns a white texture fallback
- Vertex and index buffers are always valid for RT-visible drawable objects