# Step 3: Modify Engine to Add maxRayTracingObjects

## Purpose

Add a global `maxRayTracingObjects` attribute to the `Engine` class. This controls the maximum number of objects that the ray tracing system will process, capping TLAS instances, materials, vertex buffers, index buffers, and albedo textures. All ray tracing systems check this value to ensure they don't exceed the limit.

## Files to Modify

- `lib/include/bg2e/render/Engine.hpp`

## Changes to Engine.hpp

### Add member variable

In the `private:` section, add:

```cpp
uint32_t _maxRayTracingObjects = 256;
```

### Add getters/setters

Add these methods in the `public:` section:

```cpp
inline void setMaxRayTracingObjects(uint32_t max) { _maxRayTracingObjects = max; }
inline uint32_t maxRayTracingObjects() const { return _maxRayTracingObjects; }
```

### Full diff context

```cpp
class BG2E_API Engine {
public:
    // ... existing methods ...

    inline bool rayTracingSupported() const { return _physicalDevice.properties()->rayTracingSupported(); }

    // NEW: Get/set maximum number of objects for ray tracing
    inline void setMaxRayTracingObjects(uint32_t max) { _maxRayTracingObjects = max; }
    inline uint32_t maxRayTracingObjects() const { return _maxRayTracingObjects; }

    // ... existing methods ...
};

// ... existing includes ...
```

## Verification

After this change:
- `engine->maxRayTracingObjects()` returns the global limit (default 256)
- `engine->setMaxRayTracingObjects(n)` changes the limit
- `CollectRayTracingInstancesVisitor` checks this limit before collecting more objects
- `RTMaterialDataBinding::MAX_OBJECTS` is set to 256 to match

## Notes

- The default value is 256, which gives us 4 array bindings with count=256 each
- This is a reasonable limit for most scenes. If a scene has more than 256 RT-visible objects, only the first 256 (in traversal order) will be included in the TLAS and material data binding
- The limit should be set early in application initialization, before the first frame renders