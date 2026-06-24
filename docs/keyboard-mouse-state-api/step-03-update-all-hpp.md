# Step 03: Update all.hpp

## File to Modify

- `lib/include/bg2e/app/all.hpp`

## Changes

Add two new includes after the existing `InputManager.hpp` include:

```cpp
// Current content (lines 25-26):
#include <bg2e/app/InputManager.hpp>
#include <bg2e/app/KeyEvent.hpp>

// New content:
#include <bg2e/app/InputManager.hpp>
#include <bg2e/app/Keyboard.hpp>
#include <bg2e/app/KeyEvent.hpp>
#include <bg2e/app/Mouse.hpp>
```

The new includes are placed alphabetically among the existing includes, maintaining the file's organizational convention.

## Integration Points

- `all.hpp` is the convenience header that includes the entire `bg2e::app` namespace.
- Adding the includes here makes `Keyboard` and `Mouse` available to any code that includes `<bg2e/app/all.hpp>`.
- No other changes needed — the CMake build system auto-discovers files in `lib/include/**`.
