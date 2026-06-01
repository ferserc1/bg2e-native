# Step 8: Modify DeferredLayer to Wire Everything Together

## Purpose

Create the `RTMaterialDataBinding` instance, pass it to `RTReflections`, and provide object instances at render time by reading them from `RayTracingScene`.

## Files to Modify

- `lib/include/bg2e/render/deferred/DeferredLayer.hpp`
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

## Changes to DeferredLayer.hpp

### Add include

```cpp
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
```

### Add member variable

In the `protected:` section, near the other unique_ptrs:

```cpp
std::unique_ptr<vulkan::rt::RTMaterialDataBinding> _rtMaterialDataBinding;
```

## Changes to DeferredLayer.cpp

### In build() method

Before the existing RT reflections creation block (around line 108-116), create the material data binding and pass it to RTReflections:

```cpp
if (_engine->rayTracingSupported())
{
+   // Create RT material data binding first (needed for pipeline layout)
+   _rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);
+
    _rtReflections = std::make_unique<RTReflections>(_engine);
+   _rtReflections->setMaterialDataBinding(_rtMaterialDataBinding.get());
    _rtReflections->build(_gbuffers[0].get(), extent);

    _temporalReflectionAccumulator = std::make_unique<TemporalAccumulator>(_engine);
    _temporalReflectionAccumulator->setFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    _temporalReflectionAccumulator->setIsHDR(true);
    _temporalReflectionAccumulator->build(_gbuffers[0].get(), extent);
}
```

**IMPORTANT**: The `setMaterialDataBinding()` call MUST happen BEFORE `_rtReflections->build()`, because `build()` calls `createPipeline()` which needs the material data binding to create the pipeline layout. So the correct order is:

```cpp
if (_engine->rayTracingSupported())
{
+   _rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);
+
    _rtReflections = std::make_unique<RTReflections>(_engine);
+   _rtReflections->setMaterialDataBinding(_rtMaterialDataBinding.get());
    _rtReflections->build(_gbuffers[0].get(), extent);

    _temporalReflectionAccumulator = std::make_unique<TemporalAccumulator>(_engine);
    // ...
}
```

### In initFrameResources() method

Add the material data binding's frame resource initialization:

```cpp
void DeferredLayer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    _frameDataBinding->initFrameResources(allocator);
    _fragmentFrameDataBinding->initFrameResources(allocator);
    _objectDataBinding->initFrameResources(allocator);
    _environmentDataBinding->initFrameResources(allocator);
+   if (_rtMaterialDataBinding)
+   {
+       _rtMaterialDataBinding->initFrameResources(allocator);
+   }
}
```

### In render() method - RT reflections section

Modify the RT reflections render call (around line 298-319) to pass object instances:

```cpp
// RT Reflections pass (after denoise, before composite)
if (_rtReflections && _rtReflections->rtSupported())
{
    auto tlas = frameResources.rayTracingScene ?
        frameResources.rayTracingScene->tlas() : VK_NULL_HANDLE;

    if (tlas != VK_NULL_HANDLE && _rtReflections->settings().enabled)
    {
+       // Get object instances from the ray tracing scene
+       const auto& objectInstances = frameResources.rayTracingScene->objectInstances();

        // RT Reflections pass
        _rtReflections->render(
            cmd, currentFrame, frameResources, gbuffer,
-           invVP, cameraWorldPos, tlas
+           invVP, cameraWorldPos, tlas, objectInstances
        );

        // Reflection temporal accumulation
        auto rawReflectionImage = _rtReflections->reflectionImage(frameResourcesIndex);
        _temporalReflectionAccumulator->render(
            cmd, currentFrame, frameResources, gbuffer,
            rawReflectionImage.get(), invVP, viewMat, projMat
        );

        reflectionInputForComposite = _temporalReflectionAccumulator->outputImage(frameResourcesIndex).get();
    }
}
```

### In cleanup() method

Add cleanup for the material data binding:

```cpp
void DeferredLayer::cleanup()
{
    // ... existing cleanup ...

+   if (_rtMaterialDataBinding)
+   {
+       _rtMaterialDataBinding->cleanup();
+   }

    // ... rest of existing cleanup ...
}
```

## Verification Checklist

- [ ] `_rtMaterialDataBinding` is created before `_rtReflections->build()` is called
- [ ] `setMaterialDataBinding()` is called before `build()`
- [ ] `initFrameResources()` includes the material data binding
- [ ] `render()` passes object instances from `frameResources.rayTracingScene->objectInstances()`
- [ ] `cleanup()` calls `_rtMaterialDataBinding->cleanup()`

## Notes

- The null check on `_rtMaterialDataBinding` in `initFrameResources` and `cleanup` handles the case where ray tracing is not supported.
- `RayTracingScene::objectInstances()` returns a const reference - no copy is made at the call site.
- The object instances are populated during the TLAS update, which happens before `DeferredLayer::render()` in the frame lifecycle.
- The object instances are capped at `engine->maxRayTracingObjects()` (default 256) by the visitor.