# Plan Status

## Step 01 completed: Create Keyboard class
Date: 2026-06-24
Changes:
- lib/include/bg2e/app/Keyboard.hpp: Created Keyboard class with 12 static methods for modifier key queries (shiftPressed, controlPressed, altPressed, superPressed + individual left/right variants)
- lib/src/bg2e/app/Keyboard.cpp: Implemented all methods wrapping SDL_GetModState() with KMOD_* flags

## Step 02 completed: Create Mouse class
Date: 2026-06-24
Changes:
- lib/include/bg2e/app/Mouse.hpp: Created Mouse class with static methods for mouse button queries (leftButtonPressed, middleButtonPressed, rightButtonPressed) and position (x, y)
- lib/src/bg2e/app/Mouse.cpp: Implemented all methods wrapping SDL_GetMouseState() with SDL_BUTTON_* masks

## Step 03 completed: Update all.hpp includes
Date: 2026-06-24
Changes:
- lib/include/bg2e/app/all.hpp: Added includes for Keyboard.hpp and Mouse.hpp in alphabetical order

## Step 04 completed: Migrate OrbitCameraComponent
Date: 2026-06-24
Changes:
- lib/include/bg2e/scene/OrbitCameraComponent.hpp: Changed include from InputManager.hpp to Mouse.hpp, updated matchMouseState() signature to take only MouseButtons param
- lib/src/bg2e/scene/OrbitCameraComponent.cpp: Changed include from InputManager.hpp to Mouse.hpp, replaced getOrbitAction() implementation to query Mouse directly instead of InputManager::getMouseStatus()

## Step 05 completed: Deprecate InputManager mouse API
Date: 2026-06-24
Changes:
- lib/include/bg2e/app/InputManager.hpp: Added [[deprecated("Use bg2e::app::Mouse instead")] attributes to MouseButtonsStatus struct and getMouseStatus() static method

## Step 06 completed: Fix Shortcuts fall-through bug
Date: 2026-06-24
Changes:
- lib/src/bg2e/app/Shortcuts.cpp: Added missing `break` statement after `_shiftModifier = false;` in Shortcuts::keyUp() to prevent fall-through to alt modifier cases
