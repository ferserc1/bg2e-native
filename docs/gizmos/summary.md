# Gizmo System - Implementation Summary

## Objective

Replace the ad-hoc light gizmo workaround in `StageScene::instantUpdateLightMesh()` with a proper `GizmoComponent` that lives in the `bg2e::manipulation` namespace. The component will automatically detect and display gizmos for Camera, Light (Point, Spot, Directional), and Environment nodes.

## Key Design Decisions

- **Inherits from `DrawableComponent`** — gizmos are drawables, but with special lifecycle
- **No serialization** — `serialize()`/`deserialize()` are no-ops; gizmos are never saved
- **No factory registration** — no `BG2E_SCENE_REGISTER_COMPONENT` macro; not available in the component factory
- **Lazy update via `safeUpdateScene`** — gizmo mesh swaps happen deferred to avoid GPU buffer conflicts
- **Static mesh cache** — shared `Drawable` instances across all `GizmoComponent` instances to reduce memory usage
- **Priority system** — Camera > Light > Environment; only one gizmo per node

## Gizmo Types Enum

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

## Files to Create

| File | Purpose |
|------|---------|
| `lib/include/bg2e/manipulation/GizmoComponent.hpp` | Header with enum, class declaration |
| `lib/src/bg2e/manipulation/GizmoComponent.cpp` | Implementation |

## Files to Modify

| File | Change |
|------|--------|
| `lib/include/bg2e/manipulation/all.hpp` | Add `#include <bg2e/manipulation/GizmoComponent.hpp>` |

## Integration

After this component is in place, `StageScene::instantUpdateLightMesh()` can be replaced with:

```cpp
node->addComponent(new bg2e::manipulation::GizmoComponent(_engine));
```

The component handles all gizmo detection, loading, and updating autonomously.

## Next Steps (not in this plan)

- Create a dedicated `GizmoRenderer` for rendering gizmo drawables
- Migrate `StageScene` to use `GizmoComponent` instead of manual gizmo management
