# Step 06: Fix Shortcuts Fall-Through Bug

## File to Modify

- `lib/src/bg2e/app/Shortcuts.cpp`

## Bug Description

In `Shortcuts::keyUp()` (line 80-111), the `switch` statement for modifier keys is missing a `break` after the shift cases. This causes shift release to fall through and also clear the alt modifier:

```cpp
case KeyEvent::KeyLeftShift:
case KeyEvent::KeyRightShift:
    _shiftModifier = false;
    // BUG: Missing break here! Falls through to alt cases
case KeyEvent::KeyRightAlt:
case KeyEvent::KeyLeftAlt:
    _altModifier = false;
    break;
```

## Fix

Add the missing `break` statement:

```cpp
case KeyEvent::KeyLeftShift:
case KeyEvent::KeyRightAlt:
    _shiftModifier = false;
    break;
case KeyEvent::KeyRightAlt:
case KeyEvent::KeyLeftAlt:
    _altModifier = false;
    break;
```

## Impact

Without this fix, releasing Shift would incorrectly clear the Alt modifier state, which could cause shortcut combinations involving both Shift and Alt to malfunction.

## Integration Points

- This is a standalone bug fix unrelated to the Keyboard/Mouse API, but it's included in this plan because the `Shortcuts` class tracks modifier state — the same state that `Keyboard` now exposes via `SDL_GetModState()`.
- With `Keyboard::shiftPressed()` and `Keyboard::altPressed()` available, `Shortcuts` could theoretically be refactored to query `Keyboard` instead of tracking state internally. However, that refactoring is out of scope for this plan.
