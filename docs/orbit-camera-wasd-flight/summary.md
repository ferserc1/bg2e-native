# OrbitCameraComponent WASD Flight-Mode Displacement

## Problem Statement

`OrbitCameraComponent` has a commented-out TODO (`OrbitCameraComponent.cpp:247-262`) for keyboard-driven camera displacement. Currently, the orbit center (`_center`) can only be moved via mouse pan. There is no way to move the camera using WASD keys in flight mode. The engine's `Component` base class already provides `keyDown`/`keyUp` virtual methods dispatched by `InputVisitor`, and `app::Mouse` provides static polling for mouse button state — so all the infrastructure exists.

## Proposed Solution

Add WASD flight-mode displacement to `OrbitCameraComponent` by:

1. Overriding `Component::keyDown`/`keyUp` to track which keys (W, A, S, D, Q, E) are currently pressed.
2. In `update()`, inside the existing `if (_mouseButtonPressed)` block, compute a displacement vector from the current key state, apply it to `_center` scaled by the existing `_displacementSpeed`, and let the existing bounds clamping take effect.

### Architecture

```
 InputVisitor
      |
      v
 Component::keyDown() / keyUp()
      |
      v
 OrbitCameraComponent
 +------------------+
 | _keys.w/a/s/d/q/e|  <-- key state tracking
 +------------------+
      |
      v  (every frame)
 OrbitCameraComponent::update()
      |
      +-- if (_mouseButtonPressed)
      |       |
      |       +-- read _keys state
      |       +-- build displacement from BasisVectors (forward, right, world up)
      |       +-- normalize + scale by _displacementSpeed
      |       +-- _center += displacement
      |
      +-- clamp _center to [minX..maxX], [minY..maxY], [minZ..maxZ]
      +-- rebuild transform matrix from _center, _rotation, _distance
```

## Files to Modify

| File | Action | Description |
|------|--------|-------------|
| `lib/include/bg2e/scene/OrbitCameraComponent.hpp` | Modify | Add `KeyState` struct, `_keys` member, declare `keyDown`/`keyUp` overrides |
| `lib/src/bg2e/scene/OrbitCameraComponent.cpp` | Modify | Implement `keyDown`/`keyUp`, replace TODO block with WASD+QE displacement logic |

## Steps

1. [step-01-add-key-state-tracking.md](step-01-add-key-state-tracking.md) — Header changes: add `KeyState` struct and override declarations
2. [step-02-implement-displacement.md](step-02-implement-displacement.md) — Source changes: implement key handlers and displacement logic in `update()`

## Notes

- **No new dependencies** — `KeyEvent.hpp` is already included transitively via `Component.hpp`.
- **No serialization changes** — `_displacementSpeed` is already serialized/deserialized.
- **Bounds clamping** is already handled at lines 266-273 of `OrbitCameraComponent.cpp`.
- **Flight mode** means W/S move along the camera's forward vector, not along a world axis. Q/E move along world Y (not the camera-tilted up vector).
- **Thread safety** — not a concern; `update()` and `keyDown`/`keyUp` run on the same main thread as the engine's event loop.
