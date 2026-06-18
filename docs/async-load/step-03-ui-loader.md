# Step 03 — ui::Loader: thread-safe progress window

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/ui/Loader.hpp` | create |
| `lib/src/bg2e/ui/Loader.cpp` | create |
| `lib/include/bg2e/ui/all.hpp` | modify |

## Design notes

* `Loader` is a plain C++ object — it does not inherit from `Window` because
  it is not a user-closeable, user-moveable window. It renders as a centered,
  fixed-size ImGui window with no title bar close button, no resize, and no
  move.
* All property accessors are protected by a `std::recursive_mutex` so the
  loader thread can safely call `setMessage` / `setProgress` while the render
  thread calls `draw()`.
* `recursive_mutex` is used (instead of plain `mutex`) to allow re-entrant
  locking from the same thread without deadlock, e.g. if `draw()` reads both
  `_message` and `_progress` under the same lock.
* Visibility is not a field on `Loader`; whether it appears is controlled by
  whether `UserInterface::setFrameOverride` points to it (managed by `MainLoop`
  in step 04). `draw()` always draws unconditionally when called.

## `Loader.hpp`

```cpp
#pragma once

#include <bg2e/common.hpp>

#include <string>
#include <mutex>

namespace bg2e {
namespace ui {

class BG2E_API Loader {
public:
    void setMessage(const std::string& msg);
    std::string getMessage() const;

    void setProgress(float progress);   // clamped to [0, 1]
    float getProgress() const;

    // Call once per ImGui frame while the loader is active.
    // Draws a centered non-closeable window with message + progress bar.
    void draw();

private:
    mutable std::recursive_mutex _mutex;
    std::string _message  = "Loading...";
    float       _progress = 0.f;
};

} // ui
} // bg2e
```

## `Loader.cpp`

```cpp
#include <bg2e/ui/Loader.hpp>

#ifdef BG2E_LINUX
#include <imgui/imgui.h>
#else
#include <imgui.h>
#endif

#include <algorithm>

namespace bg2e {
namespace ui {

void Loader::setMessage(const std::string& msg)
{
    std::lock_guard lock(_mutex);
    _message = msg;
}

std::string Loader::getMessage() const
{
    std::lock_guard lock(_mutex);
    return _message;
}

void Loader::setProgress(float progress)
{
    std::lock_guard lock(_mutex);
    _progress = std::clamp(progress, 0.f, 1.f);
}

float Loader::getProgress() const
{
    std::lock_guard lock(_mutex);
    return _progress;
}

void Loader::draw()
{
    std::lock_guard lock(_mutex);

    ImGuiIO& io = ImGui::GetIO();
    float winW = 420.f;
    float winH = 90.f;
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - winW) * 0.5f,
               (io.DisplaySize.y - winH) * 0.5f),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar   |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoScrollbar  |
        ImGuiWindowFlags_NoCollapse   |
        ImGuiWindowFlags_NoNav        |
        ImGuiWindowFlags_NoBringToDisplayFront;

    if (ImGui::Begin("##loader", nullptr, flags))
    {
        ImGui::TextUnformatted(_message.c_str());
        ImGui::Spacing();
        ImGui::ProgressBar(_progress, ImVec2(-1.f, 0.f));
    }
    ImGui::End();
}

} // ui
} // bg2e
```

### ImGui include path

Check the existing UI source files in `lib/src/bg2e/ui/` to confirm whether
the include is `<imgui/imgui.h>` (Linux) or `<imgui.h>` (macOS/Windows).
The pattern already used in the codebase must be followed here too.

## `all.hpp` modification

Add after the last existing `#include`:

```cpp
#include <bg2e/ui/Loader.hpp>
```

## Compile-time check

The new translation unit must compile cleanly. No existing code changes, so
no regressions are expected.

## Tests / manual verification

* Instantiate a `Loader` on the stack.
* In a temporary test frame override, call `loader.draw()`.
* Confirm the centered window appears with text and progress bar.
* Call `setProgress` and `setMessage` from a background thread while `draw()`
  runs on the main thread; confirm no data races (run under ThreadSanitizer if
  available).
