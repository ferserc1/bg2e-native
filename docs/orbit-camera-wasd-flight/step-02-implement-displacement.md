# Step 02: Implement Key Handlers and WASD Flight Displacement

## Files to Modify

- `lib/src/bg2e/scene/OrbitCameraComponent.cpp`

## Changes

### 1. Implement `keyDown` method

Add after the existing `mouseWheel` method (after line 350):

```cpp
void OrbitCameraComponent::keyDown(const app::KeyEvent& event)
{
    if (!_enabled) return;
    switch (event.key()) {
        case app::KeyEvent::KeyW: _keys.w = true; break;
        case app::KeyEvent::KeyA: _keys.a = true; break;
        case app::KeyEvent::KeyS: _keys.s = true; break;
        case app::KeyEvent::KeyD: _keys.d = true; break;
        case app::KeyEvent::KeyQ: _keys.q = true; break;
        case app::KeyEvent::KeyE: _keys.e = true; break;
        default: break;
    }
}
```

### 2. Implement `keyUp` method

Add immediately after `keyDown`:

```cpp
void OrbitCameraComponent::keyUp(const app::KeyEvent& event)
{
    switch (event.key()) {
        case app::KeyEvent::KeyW: _keys.w = false; break;
        case app::KeyEvent::KeyA: _keys.a = false; break;
        case app::KeyEvent::KeyS: _keys.s = false; break;
        case app::KeyEvent::KeyD: _keys.d = false; break;
        case app::KeyEvent::KeyQ: _keys.q = false; break;
        case app::KeyEvent::KeyE: _keys.e = false; break;
        default: break;
    }
}
```

Note: `keyUp` does not check `_enabled` — if a key was pressed while enabled, the release event must still clear the state even if the component was disabled in between. Otherwise the key would appear permanently stuck.

### 3. Replace the TODO block in `update()` with WASD+QE displacement

Replace lines 245-264 (the `if (_mouseButtonPressed)` block containing the commented-out TODO):

**Before:**
```cpp
if (_mouseButtonPressed)
{
    // TODO: displacement using keyboard arrows
    //            let displacement = new Vec([0,0,0]);
//            if (this._keys[SpecialKey.UP_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.backwardVector);
//            }
//            if (this._keys[SpecialKey.DOWN_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.forwardVector);
//            }
//            if (this._keys[SpecialKey.LEFT_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.leftVector);
//            }
//            if (this._keys[SpecialKey.RIGHT_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.rightVector);
//            }
//            displacement.scale(this._displacementSpeed);
//            this._center = Vec.Add(this._center, displacement);
    
}
```

**After:**
```cpp
if (_mouseButtonPressed)
{
    math::BasisVectors basis(transform->matrix(), true);

    glm::vec3 displacement(0.0f);

    if (_keys.w) displacement += basis.forward;
    if (_keys.s) displacement -= basis.forward;
    if (_keys.a) displacement -= basis.right;
    if (_keys.d) displacement += basis.right;
    if (_keys.e) displacement += glm::vec3(0.0f, 1.0f, 0.0f);
    if (_keys.q) displacement -= glm::vec3(0.0f, 1.0f, 0.0f);

    if (glm::length(displacement) > 0.0f)
    {
        displacement = glm::normalize(displacement) * _displacementSpeed;
        _center += displacement;
    }
}
```

### How the displacement works

- `math::BasisVectors basis(transform->matrix(), true)` extracts right, up, and forward vectors from the current transform matrix. With `isViewMatrix=true`, the forward vector (column 2) is negated so it points toward the camera's look direction.
- W/S move along `basis.forward` (into/away from the screen in flight mode).
- A/D move along `basis.right` (strafe left/right).
- Q/E move along world Y axis `(0, 1, 0)` — not the camera's tilted up vector — which is standard flight behavior.
- The displacement vector is normalized before scaling so diagonal movement is not faster than cardinal movement.
- `_displacementSpeed` (default `0.1f`) controls the movement rate. It is already configurable and serialized.
- `_center` is clamped to `[minX..maxX], [minY..maxY], [minZ..maxZ]` later in `update()` at lines 266-273 — no extra clamping needed.

## Integration Points

- The `if (_mouseButtonPressed)` guard ensures keyboard displacement only works when the left mouse button is held, as required.
- `BasisVectors` is already used in `mouseMove()` (line 315) — the same pattern.
- `_center` bounds clamping at lines 266-273 applies automatically after displacement.
- The transform rebuild at lines 277-285 uses the updated `_center`.

## Verification

Build the project and run the model editor (or any example using `OrbitCameraComponent`). Hold the left mouse button and press WASD/QE to verify:
- W moves forward along the camera's look direction
- S moves backward
- A strafes left, D strafes right
- E moves up (world Y), Q moves down (world Y)
- Diagonal movement is not faster than single-axis movement
- Displacement stops when the mouse button is released
- Camera center stays within configured bounds
