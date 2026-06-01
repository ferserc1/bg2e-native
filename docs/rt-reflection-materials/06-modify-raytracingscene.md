# Step 6: Modify RayTracingScene to Store Object Instances

## Purpose

Currently, `RayTracingScene::update()` creates a `CollectRayTracingInstancesVisitor`, collects instances, and builds the TLAS - but the `objectInstances()` collected by the visitor are discarded when the visitor goes out of scope. We need to store them so they're available at render time for `RTMaterialDataBinding`.

## Files to Modify

- `lib/include/bg2e/render/vulkan/rt/RayTracingScene.hpp`
- `lib/src/bg2e/render/vulkan/rt/RayTracingScene.cpp`

## Changes to RayTracingScene.hpp

### Add include

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialData.h>
```

### Add member variable

In the `protected:` section, add:

```cpp
std::vector<RTObjectInstance> _objectInstances;
```

### Add accessor

In the `public:` section, add:

```cpp
[[nodiscard]] const std::vector<RTObjectInstance>& objectInstances() const { return _objectInstances; }
```

## Changes to RayTracingScene.cpp

### In `update()` method

After the visitor collects instances (line ~51), and before the visitor goes out of scope, store the object instances:

```cpp
bool RayTracingScene::update(VkCommandBuffer cmd, scene::Node* node)
{
    if (!_engine->rayTracingSupported())
    {
        return false;
    }

    CollectRayTracingInstancesVisitor visitor(_engine);
    node->accept(&visitor);

    const auto & instances = visitor.rayTracingInstances();
+   _objectInstances = visitor.objectInstances();  // Store object instances

    if (instances.empty())
    {
        cleanup();
        return true;
    }

    // ... rest of TLAS build unchanged ...
```

### In `cleanup()` method

Add clearing of object instances:

```cpp
void RayTracingScene::cleanup()
{
    // ... existing cleanup code ...

+   _objectInstances.clear();
}
```

## Verification

After this change, `frameResources.rayTracingScene->objectInstances()` returns the object instances collected during the current frame's TLAS build. This data is available during `DeferredLayer::render()` and can be passed to `RTMaterialDataBinding::newDescriptorSet()`.

## Notes

- The object instances are stored as a copy (not a reference) because the visitor is a local variable that gets destroyed after `update()`.
- The vector is cleared in `cleanup()` to free memory when the TLAS is destroyed.
- `RTMaterialData.h` is already included by `CollectRayTracingInstancesVisitor.hpp`, so the type is available. Adding the include to `RayTracingScene.hpp` ensures the header is self-contained.
- The object instances are capped at `engine->maxRayTracingObjects()` by the visitor, so `_objectInstances.size()` will never exceed 256.