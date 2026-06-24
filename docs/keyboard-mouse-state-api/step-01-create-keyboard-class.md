# Step 01: Create Keyboard Class

## Files to Create

- `lib/include/bg2e/app/Keyboard.hpp`
- `lib/src/bg2e/app/Keyboard.cpp`

## Interface

```cpp
// lib/include/bg2e/app/Keyboard.hpp
#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace app {

class BG2E_API Keyboard {
public:
    // Any shift key held (left or right)
    static bool shiftPressed();

    // Any control key held (left or right)
    static bool controlPressed();

    // Any alt key held (left or right)
    static bool altPressed();

    // Any super key held (left or right)
    static bool superPressed();

    // Individual modifier keys
    static bool leftShiftPressed();
    static bool rightShiftPressed();
    static bool leftControlPressed();
    static bool rightControlPressed();
    static bool leftAltPressed();
    static bool rightAltPressed();
    static bool leftSuperPressed();
    static bool rightSuperPressed();
};

}
}
```

## Implementation Details

Each method calls `SDL_GetModState()` and checks the corresponding `KMOD_*` flag. The SDL functions are:

- `SDL_GetModState()` returns `Uint16` with bitmask of currently held modifier keys
- `KMOD_SHIFT` = `KMOD_LSHIFT | KMOD_RSHIFT`
- `KMOD_CTRL` = `KMOD_LCTRL | KMOD_RCTRL`
- `KMOD_ALT` = `KMOD_LALT | KMOD_RALT`
- `KMOD_GUI` = `KMOD_LGUI | KMOD_RGUI` (Super/Windows/Cmd key)

```cpp
// lib/src/bg2e/app/Keyboard.cpp
#include <bg2e/app/Keyboard.hpp>
#include <SDL2/SDL.h>

namespace bg2e {
namespace app {

bool Keyboard::shiftPressed() {
    return (SDL_GetModState() & KMOD_SHIFT) != 0;
}

bool Keyboard::controlPressed() {
    return (SDL_GetModState() & KMOD_CTRL) != 0;
}

bool Keyboard::altPressed() {
    return (SDL_GetModState() & KMOD_ALT) != 0;
}

bool Keyboard::superPressed() {
    return (SDL_GetModState() & KMOD_GUI) != 0;
}

bool Keyboard::leftShiftPressed() {
    return (SDL_GetModState() & KMOD_LSHIFT) != 0;
}

bool Keyboard::rightShiftPressed() {
    return (SDL_GetModState() & KMOD_RSHIFT) != 0;
}

bool Keyboard::leftControlPressed() {
    return (SDL_GetModState() & KMOD_LCTRL) != 0;
}

bool Keyboard::rightControlPressed() {
    return (SDL_GetModState() & KMOD_RCTRL) != 0;
}

bool Keyboard::leftAltPressed() {
    return (SDL_GetModState() & KMOD_LALT) != 0;
}

bool Keyboard::rightAltPressed() {
    return (SDL_GetModState() & KMOD_RALT) != 0;
}

bool Keyboard::leftSuperPressed() {
    return (SDL_GetModState() & KMOD_LGUI) != 0;
}

bool Keyboard::rightSuperPressed() {
    return (SDL_GetModState() & KMOD_RGUI) != 0;
}

}
}
```

## Integration Points

- This is a standalone class with no dependencies on other engine code.
- Will be included in `all.hpp` in Step 03.
- No existing code is modified in this step.
