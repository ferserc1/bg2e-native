# Gizmo System - Detailed Implementation Steps

## Step 1: Create `GizmoComponent.hpp`

**File:** `lib/include/bg2e/manipulation/GizmoComponent.hpp`

### 1.1 Header guard and includes

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/base/Light.hpp>

#include <memory>
#include <unordered_map>
```

### 1.2 Define the GizmoType enum

Inside `namespace bg2e::manipulation`:

```cpp
enum class GizmoType {
    None,
    Camera,
    PointLight,
    SpotLight,
    DirectionalLight,
    Environment
};
```

Priority order for detection (highest to lowest):
1. **Camera** — if node has `CameraComponent`
2. **Light** — if node has `LightComponent`, sub-typed by `Light::LightType`:
   - `Light::TypeOmni` → `PointLight`
   - `Light::TypeSpot` → `SpotLight`
   - `Light::TypeDirectional` → `DirectionalLight`
3. **Environment** — if node has `EnvironmentComponent`

### 1.3 Class declaration

```cpp
class BG2E_API GizmoComponent : public scene::DrawableComponent {
public:
    BG2E_COMPONENT_TYPE_NAME("Gizmo");

    GizmoComponent(render::Engine* engine);
    virtual ~GizmoComponent() = default;

    // No serialization — gizmos are runtime-only
    void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&, render::Engine&) override {}
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&) override { return nullptr; }

    void update(float delta) override;

    GizmoType currentGizmoType() const { return _currentGizmoType; }

    static void cleanupStatic();

private:
    render::Engine* _engine;
    GizmoType _currentGizmoType = GizmoType::None;

    GizmoType resolveGizmoType() const;
    void loadGizmo(GizmoType type);
    void unloadGizmo();

    // Static mesh cache: shared across all GizmoComponent instances
    static std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> _gizmoCache;
    static std::shared_ptr<scene::Drawable> getCachedGizmo(GizmoType type, render::Engine* engine);
};
```

Key points:
- **No `BG2E_SCENE_REGISTER_COMPONENT`** — the macro is intentionally omitted
- Constructor takes `render::Engine*` because `load()` requires it
- `BG2E_COMPONENT_TYPE_NAME("Gizmo")` is needed for `typeName()` but won't be registered in the factory

---

## Step 2: Create `GizmoComponent.cpp`

**File:** `lib/src/bg2e/manipulation/GizmoComponent.cpp`

### 2.1 Includes

```cpp
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/app/MainLoop.hpp>
```

### 2.2 Static member initialization

```cpp
namespace bg2e::manipulation {

std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> GizmoComponent::_gizmoCache;
```

### 2.3 Constructor

```cpp
GizmoComponent::GizmoComponent(render::Engine* engine)
    : _engine { engine }
{
}
```

### 2.4 `resolveGizmoType()` — priority-based detection

```cpp
GizmoType GizmoComponent::resolveGizmoType() const
{
    auto node = ownerNode();
    if (!node) return GizmoType::None;

    // Priority 1: Camera
    if (node->camera()) return GizmoType::Camera;

    // Priority 2: Light (sub-typed)
    auto lightComp = node->light();
    if (lightComp) {
        switch (lightComp->light().type()) {
            case base::Light::TypeOmni:       return GizmoType::PointLight;
            case base::Light::TypeSpot:        return GizmoType::SpotLight;
            case base::Light::TypeDirectional: return GizmoType::DirectionalLight;
            default: break;
        }
    }

    // Priority 3: Environment
    if (node->environment()) return GizmoType::Environment;

    return GizmoType::None;
}
```

### 2.5 `getCachedGizmo()` — static mesh cache with lazy loading

```cpp
std::shared_ptr<scene::Drawable> GizmoComponent::getCachedGizmo(GizmoType type, render::Engine* engine)
{
    auto it = _gizmoCache.find(type);
    if (it != _gizmoCache.end()) return it->second;

    std::shared_ptr<geo::Mesh> mesh;
    auto assetsPath = base::PlatformTools::assetPath();

    switch (type) {
        case GizmoType::DirectionalLight: {
            auto meshData = db::loadMeshBg2(assetsPath, "dir-light-gizmo.bg2");
            mesh = meshData->mesh;
            break;
        }
        case GizmoType::SpotLight: {
            auto meshData = db::loadMeshBg2(assetsPath, "spot-light-gizmo.bg2");
            mesh = meshData->mesh;
            break;
        }
        case GizmoType::PointLight: {
            mesh = std::shared_ptr<geo::Mesh>(geo::createSphere(0.5f, 10, 10));
            break;
        }
        case GizmoType::Camera:
        case GizmoType::Environment:
            // TODO: load camera/environment gizmo meshes when assets are available
            return nullptr;
        default:
            return nullptr;
    }

    auto drawable = std::make_shared<scene::Drawable>();
    drawable->setMesh(mesh);
    drawable->load(*engine);
    drawable->material(0).setIsUnlit(true);
    drawable->setRayTracingEnabled(false);
    drawable->updateMaterials();

    _gizmoCache[type] = drawable;
    return drawable;
}
```

### 2.6 `loadGizmo()` — safe gizmo swap

```cpp
void GizmoComponent::loadGizmo(GizmoType type)
{
    auto cachedDrawable = getCachedGizmo(type, _engine);
    if (cachedDrawable) {
        setDrawable(cachedDrawable);
    }
}
```

### 2.7 `unloadGizmo()`

```cpp
void GizmoComponent::unloadGizmo()
{
    setDrawable(nullptr);
}
```

### 2.8 `update()` — lazy gizmo detection and deferred swap

```cpp
void GizmoComponent::update(float)
{
    auto newType = resolveGizmoType();
    if (newType == _currentGizmoType) return;

    _currentGizmoType = newType;

    // Defer the actual mesh swap to avoid destroying buffers mid-render
    app::MainLoop::current()->safeUpdateScene([this, newType]() {
        if (newType == GizmoType::None) {
            unloadGizmo();
        } else {
            loadGizmo(newType);
        }
    });
}
```

### 2.9 `cleanupStatic()` — release cached meshes

```cpp
void GizmoComponent::cleanupStatic()
{
    _gizmoCache.clear();
}
```

### 2.10 Close namespace

```cpp
}
```

**Note:** No `BG2E_SCENE_REGISTER_COMPONENT(GizmoComponent)` line at the end.

---

## Step 3: Update `manipulation/all.hpp`

**File:** `lib/include/bg2e/manipulation/all.hpp`

Add the include for the new component:

```cpp
#include <bg2e/manipulation/GizmoComponent.hpp>
```

This ensures that any file including `<bg2e/manipulation/all.hpp>` gets the gizmo component.

---

## Step 4: Verification

### 4.1 Build check

The new files will be automatically picked up by CMake because the build system globs all `.hpp` files in `lib/include/` and all `.cpp` files in `lib/src/`.

### 4.2 Manual test

To verify the component works, temporarily add it to a light node in `StageScene`:

```cpp
auto gizmo = new bg2e::manipulation::GizmoComponent(_engine);
node->addComponent(gizmo);
```

The gizmo should automatically detect the light type and display the correct mesh.

### 4.3 Cleanup verification

Call `GizmoComponent::cleanupStatic()` during application shutdown to verify no resource leaks.

---

## Architecture Notes

### Why inherit from DrawableComponent instead of Component?

- `DrawableComponent` already has the `drawable()` / `setDrawable()` interface that renderers expect
- The gizmo renderer (future step) can iterate nodes with `DrawableComponent` and distinguish gizmos by checking `dynamic_cast<GizmoComponent*>`
- Avoids duplicating drawable management logic

### Why static mesh cache?

- Multiple light nodes of the same type share the same mesh geometry
- Loading `.bg2` files and uploading to GPU is expensive — do it once per type
- `cleanupStatic()` allows the renderer to explicitly release GPU resources when shutting down

### Why lazy update with `safeUpdateScene`?

- Components can be modified during rendering (e.g., user changes light type in UI)
- Directly destroying GPU buffers mid-render causes validation errors or crashes
- `safeUpdateScene` defers the swap to frame start when GPU is idle
- The `_currentGizmoType` variable prevents redundant deferred calls
