# Step 02: Create Mouse Class

## Files to Create

- `lib/include/bg2e/app/Mouse.hpp`
- `lib/src/bg2e/app/Mouse.cpp`

## Interface

```cpp
// lib/include/bg2e/app/Mouse.hpp
#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace app {

class BG2E_API Mouse {
public:
    static bool leftButtonPressed();
    static bool middleButtonPressed();
    static bool rightButtonPressed();
    static int x();
    static int y();
};

}
}
```

## Implementation Details

Each method calls `SDL_GetMouseState()` and checks the corresponding `SDL_BUTTON_*` mask. The SDL API returns both button state and position in a single call:

```cpp
// lib/src/bg2e/app/Mouse.cpp
#include <bg2e/app/Mouse.hpp>
#include <SDL2/SDL.h>

namespace bg2e {
namespace app {

bool Mouse::leftButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_LMASK) != 0;
}

bool Mouse::middleButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_MMASK) != 0;
}

bool Mouse::rightButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_RMASK) != 0;
}

int Mouse::x() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return x;
}

int Mouse::y() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return y;
}

}
}
```

**Note on performance**: Each method calls `SDL_GetMouseState()` independently, which results in multiple OS calls per frame if multiple methods are called in sequence. This is acceptable because:
1. `SDL_GetMouseState()` is a lightweight OS query (no allocations, no event pumping)
2. Most call sites query one or two methods at a time
3. The alternative (caching state) would require MainLoop integration, adding complexity

## Integration Points

- This is a standalone class with no dependencies on other engine code.
- Will be included in `all.hpp` in Step 03.
- Will replace `InputManager::getMouseStatus()` usage in Step 04.
- The existing `InputManager::getMouseStatus()` struct has a typo (`rigth` instead of `right`) — the new class corrects this.
