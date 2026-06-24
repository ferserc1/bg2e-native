# Keyboard & Mouse State API

## Problem Statement

The `bg2e::app` namespace has no API to query the current state of keyboard modifier keys (Shift, Ctrl, Alt, Super). The existing `KeyEvent` class discards modifier information from SDL events, and `InputDelegate` callbacks pass no modifier state. The only modifier tracking exists privately inside `Shortcuts` (which also has a fall-through bug).

For mouse state, `InputManager::getMouseStatus()` exists as a static method returning a `MouseButtonsStatus` struct (which contains a typo: `rigth` instead of `right`). This struct-based API is less ergonomic than individual query methods and couples mouse state to `InputManager`.

Both gaps prevent components and application code from cleanly querying input state.

## Proposed Solution

Create two new stateless wrapper classes with static methods:

- **`bg2e::app::Keyboard`** — wraps `SDL_GetModState()` to query modifier key state
- **`bg2e::app::Mouse`** — wraps `SDL_GetMouseState()` to query mouse button state

Both classes use direct SDL calls (no singleton, no MainLoop integration needed). Migrate existing consumers of `InputManager::getMouseStatus()` to the new `Mouse` class. Deprecate the old `MouseButtonsStatus` struct and `getMouseStatus()` method.

```
  ┌─────────────────────────────────────────────────────┐
  │                  Application Code                   │
  │         (Components, Delegates, Examples)            │
  └──────────┬────────────────────────┬─────────────────┘
             │                        │
    ┌────────▼─────────┐   ┌─────────▼────────┐
    │  Keyboard (new)  │   │   Mouse (new)    │
    │  static methods  │   │  static methods  │
    │  wrapping SDL    │   │  wrapping SDL    │
    └────────┬─────────┘   └─────────┬────────┘
             │                        │
    ┌────────▼─────────┐   ┌─────────▼────────┐
    │ SDL_GetModState()│   │SDL_GetMouseState()│
    └──────────────────┘   └──────────────────┘

  ┌─────────────────────────────────────────────────────┐
  │              InputManager (legacy)                   │
  │  MouseButtonsStatus / getMouseStatus() [deprecated]  │
  │  Event dispatch + ImGui capture (unchanged)          │
  └─────────────────────────────────────────────────────┘
```

## Files to Create/Modify

| # | File | Action | Description |
|---|------|--------|-------------|
| 1 | `lib/include/bg2e/app/Keyboard.hpp` | **Create** | `Keyboard` class with static `bool` methods for modifier keys |
| 2 | `lib/src/bg2e/app/Keyboard.cpp` | **Create** | Implementation calling `SDL_GetModState()` |
| 3 | `lib/include/bg2e/app/Mouse.hpp` | **Create** | `Mouse` class with static `bool` methods for buttons + position |
| 4 | `lib/src/bg2e/app/Mouse.cpp` | **Create** | Implementation calling `SDL_GetMouseState()` |
| 5 | `lib/include/bg2e/app/all.hpp` | **Edit** | Add includes for Keyboard.hpp and Mouse.hpp |
| 6 | `lib/include/bg2e/scene/OrbitCameraComponent.hpp` | **Edit** | Change `matchMouseState()` to use `Mouse` instead of `MouseButtonsStatus` |
| 7 | `lib/src/bg2e/scene/OrbitCameraComponent.cpp` | **Edit** | Replace `InputManager::getMouseStatus()` with `Mouse` calls |
| 8 | `lib/include/bg2e/app/InputManager.hpp` | **Edit** | Add `[[deprecated]]` to `MouseButtonsStatus` and `getMouseStatus()` |
| 9 | `lib/src/bg2e/app/Shortcuts.cpp` | **Edit** | Fix fall-through bug (missing `break` at line 88) |

## Implementation Steps

- [Step 01](step-01-create-keyboard-class.md) — Create `Keyboard` class
- [Step 02](step-02-create-mouse-class.md) — Create `Mouse` class
- [Step 03](step-03-update-all-hpp.md) — Update `all.hpp` includes
- [Step 04](step-04-migrate-orbit-camera.md) — Migrate `OrbitCameraComponent`
- [Step 05](step-05-deprecate-old-api.md) — Deprecate `InputManager` mouse API
- [Step 06](step-06-fix-shortcuts-bug.md) — Fix `Shortcuts` fall-through bug

## Notes

- **No CMake changes needed** — the build system auto-discovers files in `lib/include/**` and `lib/src/**`.
- **Thread safety** — `SDL_GetModState()` and `SDL_GetMouseState()` are thread-safe when called from the same thread that pumps SDL events. Since all engine code runs on the main thread, this is safe.
- **No singleton pattern** — both classes are stateless wrappers around SDL OS-level polling functions. No singleton lifecycle management needed.
- **Backward compatibility** — `InputManager::getMouseStatus()` is deprecated but not removed. Existing code continues to compile with a deprecation warning.
