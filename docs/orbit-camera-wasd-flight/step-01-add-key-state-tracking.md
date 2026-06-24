# Step 01: Add Key State Tracking to OrbitCameraComponent Header

## Files to Modify

- `lib/include/bg2e/scene/OrbitCameraComponent.hpp`

## Changes

### 1. Add `KeyState` struct and `_keys` member in the protected section

After the existing `_mouseButtonPressed` member (line 182), add:

```cpp
struct KeyState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool q = false;
    bool e = false;
};
KeyState _keys;
```

### 2. Add `keyDown`/`keyUp` override declarations in the public section

After the `mouseWheel` declaration (line 137), add:

```cpp
void keyDown(const app::KeyEvent& keyEvent) override;
void keyUp(const app::KeyEvent& keyEvent) override;
```

No new `#include` directives are needed — `KeyEvent.hpp` is already included transitively via `Component.hpp` (which `OrbitCameraComponent.hpp` includes at line 21).

## Integration Points

- The `Component` base class declares `virtual void keyDown(const app::KeyEvent&)` at `Component.hpp:72` and `virtual void keyUp(const app::KeyEvent&)` at `Component.hpp:74`.
- `InputVisitor` dispatches key events to all components on each node (`InputVisitor.cpp:55-62`).
- The new overrides will automatically receive key events from the scene graph traversal — no registration or wiring needed.

## Verification

After this step, the code should compile without errors. The `keyDown`/`keyUp` methods will be declared but not yet implemented (implementation is in step 02), so linking will fail until step 02 is complete.
