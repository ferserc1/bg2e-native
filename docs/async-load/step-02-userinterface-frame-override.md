# Step 02 — UserInterface: frame-override mechanism

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/ui/UserInterface.hpp` | modify |
| `lib/src/bg2e/ui/UserInterface.cpp` | modify |

## Motivation

When an async load is running we want the ImGui frame to render **only** the
loading screen, bypassing `UserInterfaceDelegate::draw()` entirely. A single
override callback is the minimal, non-breaking way to achieve this.

## Changes

### `UserInterface.hpp`

Add to the public interface (requires `#include <functional>`, already present):

```cpp
void setFrameOverride(std::function<void()> fn);
void clearFrameOverride();
```

Add to the protected section:

```cpp
std::function<void()> _frameOverride;
```

### `UserInterface.cpp` — `setFrameOverride` / `clearFrameOverride`

```cpp
void UserInterface::setFrameOverride(std::function<void()> fn)
{
    _frameOverride = std::move(fn);
}

void UserInterface::clearFrameOverride()
{
    _frameOverride = nullptr;
}
```

### `UserInterface.cpp` — `draw()` modification

Inside `draw(VkCommandBuffer cmd, VkImageView targetImageView)`, find the call
to `_delegate->draw()` and replace it with:

```cpp
if (_frameOverride)
{
    _frameOverride();
}
else if (_delegate)
{
    _delegate->draw();
}
```

No other code in `draw()` changes.

## Compile-time check

No callers are broken: `_frameOverride` is null by default, so all existing
applications keep their current behaviour. Build must pass cleanly.

## Tests / manual verification

* All existing examples must run identically to before this change.
* A quick local test: set an override that draws `ImGui::Text("override")`,
  confirm only that text appears; call `clearFrameOverride()`, confirm normal
  delegate draw resumes.
