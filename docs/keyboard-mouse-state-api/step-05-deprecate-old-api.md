# Step 05: Deprecate Old InputManager Mouse API

## File to Modify

- `lib/include/bg2e/app/InputManager.hpp`

## Changes

Add `[[deprecated]]` attributes to the `MouseButtonsStatus` struct and `getMouseStatus()` method.

**Before** (lines 34-42):
```cpp
struct MouseButtonsStatus {
    bool left;
    bool middle;
    bool rigth;

    uint32_t x;
    uint32_t y;
};
static MouseButtonsStatus getMouseStatus();
```

**After**:
```cpp
struct [[deprecated("Use bg2e::app::Mouse instead")]] MouseButtonsStatus {
    bool left;
    bool middle;
    bool rigth;

    uint32_t x;
    uint32_t y;
};
[[deprecated("Use bg2e::app::Mouse instead")]]
static MouseButtonsStatus getMouseStatus();
```

The deprecation message directs users to the new `Mouse` class.

## Notes

- The implementation in `InputManager.cpp` is **not removed** — existing third-party code or examples that reference `InputManager::getMouseStatus()` will continue to compile with a deprecation warning.
- The `normalizedCursorPosition()` static method in `InputManager` is **not deprecated** — it serves a different purpose (viewport-normalized coordinates) and has no equivalent in `Mouse`.
- The `MouseButtonsStatus` typo (`rigth`) is preserved for backward compatibility.

## Integration Points

- After `OrbitCameraComponent` is migrated (Step 04), there are zero internal callers of `getMouseStatus()`.
- Examples and apps that reference `InputManager::getMouseStatus()` directly will get deprecation warnings but continue to compile.
