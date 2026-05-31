# Step 9: Public API + Documentation

## Objective

Expose RT reflection settings on `RendererDeferred` (matching the pattern used for AO settings) and update the documentation.

## Files to Modify

### `lib/include/bg2e/render/RendererDeferred.hpp`

Add public methods following the existing AO settings pattern:

```cpp
// RT Reflections
void setRTReflectionsEnabled(bool enabled);
bool rtReflectionsEnabled() const;

void setRTReflectionSampleCount(uint32_t count);
uint32_t rtReflectionSampleCount() const;

void setRTReflectionMaxRoughness(float roughness);
float rtReflectionMaxRoughness() const;

void setRTReflectionRayBias(float bias);
float rtReflectionRayBias() const;

void setRTReflectionMaxDistance(float distance);
float rtReflectionMaxDistance() const;

void setRTReflectionRoughnessSpread(float spread);
float rtReflectionRoughnessSpread() const;
```

### `lib/src/bg2e/render/RendererDeferred.cpp`

Implement the methods by delegating to `_opaqueLayer` (same pattern as AO):

```cpp
void RendererDeferred::setRTReflectionsEnabled(bool enabled) {
    _opaqueLayer->setRTReflectionsEnabled(enabled);
}

bool RendererDeferred::rtReflectionsEnabled() const {
    return _opaqueLayer->rtReflectionsEnabled();
}

void RendererDeferred::setRTReflectionSampleCount(uint32_t count) {
    _opaqueLayer->setRTReflectionSampleCount(count);
}

uint32_t RendererDeferred::rtReflectionSampleCount() const {
    return _opaqueLayer->rtReflectionSampleCount();
}

// ... same pattern for other settings ...
```

### `docs/deferred_rendering_system.md`

Update the documentation to reflect the new subsystem:

**Section 1 (Architecture):** Add `RTReflections` to the class table.

**Section 3 (Pipeline):** Update the per-frame pipeline to include RT reflections step.

**Section 7 (Composite):** `deferred_composite_rt.frag.glsl` is the single unified RT composite shader with 9 bindings (AO at 7, reflections at 8). Standard and RT are the only two composite variants.

**Section 11 (Debug Visualization):** Add the three new debug modes.

**Section 12 (Shaders):** Add the three new RT shaders to the table.

**Add new section:** RT Reflections — similar structure to Section 4 (RT AO).

### `docs/rt-reflections/`

The plan documents in `.opencode/plans/rt-reflections/` serve as the implementation reference. After implementation, they can be moved to `docs/rt-reflections/` for permanent documentation.

## Verification

After this step:
- `RendererDeferred` exposes all reflection settings
- Settings are propagated to the opaque layer
- Documentation accurately describes the new system
- The complete implementation is accessible from the public API
