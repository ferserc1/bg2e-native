# Step 04: Migrate OrbitCameraComponent

## Files to Modify

- `lib/include/bg2e/scene/OrbitCameraComponent.hpp`
- `lib/src/bg2e/scene/OrbitCameraComponent.cpp`

## Current Code

**Header** (`OrbitCameraComponent.hpp:194-201`):
```cpp
inline bool matchMouseState(
    const app::InputManager::MouseButtonsStatus & status,
    const MouseButtons & buttons
) {
    return  status.left == buttons.left &&
            status.middle == buttons.middle &&
            status.rigth == buttons.right;
}
```

**Source** (`OrbitCameraComponent.cpp:360-377`):
```cpp
OrbitCameraComponent::OrbitAction OrbitCameraComponent::getOrbitAction()
{
    auto mouseState = app::InputManager::getMouseStatus();

    if (matchMouseState(mouseState, _rotationButtons))
    {
        return OrbitAction::Rotate;
    }
    else if (matchMouseState(mouseState, _panButtons))
    {
        return OrbitAction::Pan;
    }
    else if (matchMouseState(mouseState, _zoomButtons))
    {
        return OrbitAction::Zoom;
    }
    return OrbitAction::None;
}
```

## New Code

**Header** — Replace `matchMouseState` with a new version using `Mouse`:

```cpp
inline bool matchMouseState(const MouseButtons & buttons) {
    return  app::Mouse::leftButtonPressed() == buttons.left &&
            app::Mouse::middleButtonPressed() == buttons.middle &&
            app::Mouse::rightButtonPressed() == buttons.right;
}
```

The method signature changes from taking a `MouseButtonsStatus` reference to taking no mouse state parameter — it queries `Mouse` directly. This is cleaner because:
1. The `MouseButtonsStatus` struct is no longer needed
2. No intermediate variable is required
3. The typo `rigth` is eliminated

**Header includes** — Replace `InputManager.hpp` with `Mouse.hpp`:

```cpp
// Current (line 23):
#include <bg2e/app/InputManager.hpp>

// New:
#include <bg2e/app/Mouse.hpp>
```

**Source** — Replace `getOrbitAction()`:

```cpp
OrbitCameraComponent::OrbitAction OrbitCameraComponent::getOrbitAction()
{
    if (matchMouseState(_rotationButtons))
    {
        return OrbitAction::Rotate;
    }
    else if (matchMouseState(_panButtons))
    {
        return OrbitAction::Pan;
    }
    else if (matchMouseState(_zoomButtons))
    {
        return OrbitAction::Zoom;
    }
    return OrbitAction::None;
}
```

The `auto mouseState` local variable is removed — `matchMouseState` now queries `Mouse` internally.

## Integration Points

- `OrbitCameraComponent` was the only consumer of `InputManager::getMouseStatus()` in the engine code.
- After this change, `InputManager::getMouseStatus()` has zero internal callers and can be deprecated in Step 05.
- The `MouseButtons` struct (line 142-147) is kept as-is since it defines the expected button configuration, not the actual state.
