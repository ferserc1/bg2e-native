# Step 3: Modify RayTracingScene to Store Material Instances

## Purpose

Currently, `RayTracingScene::update()` creates a `CollectRayTracingInstancesVisitor`, collects instances, and builds the TLAS - but the `materialInstances()` collected by the visitor are discarded when the visitor goes out of scope. We need to store them so they're available at render time for `RTMaterialDataBinding`.

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
std::vector<RTMaterialInstance> _materialInstances;
```

### Add accessor

In the `public:` section, add:

```cpp
[[nodiscard]] const std::vector<RTMaterialInstance>& materialInstances() const { return _materialInstances; }
```

## Changes to RayTracingScene.cpp

### In `update()` method

After the visitor collects instances (line ~51), and before the visitor goes out of scope, store the material instances:

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
+   _materialInstances = visitor.materialInstances();  // Store material instances

    if (instances.empty())
    {
        cleanup();
        return true;
    }

    // ... rest of TLAS build unchanged ...
```

### In `cleanup()` method

Add clearing of material instances:

```cpp
void RayTracingScene::cleanup()
{
    // ... existing cleanup code ...

+   _materialInstances.clear();
}
```

## Verification

After this change, `frameResources.rayTracingScene->materialInstances()` returns the material instances collected during the current frame's TLAS build. This data is available during `DeferredLayer::render()` and can be passed to `RTMaterialDataBinding::newDescriptorSet()`.

## Notes

- The material instances are stored as a copy (not a reference) because the visitor is a local variable that gets destroyed after `update()`.
- The vector is cleared in `cleanup()` to free memory when the TLAS is destroyed.
- `RTMaterialData.h` is already included by `CollectRayTracingInstancesVisitor.hpp`, so the type is available. Adding the include to `RayTracingScene.hpp` ensures the header is self-contained.
