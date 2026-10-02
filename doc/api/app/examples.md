# Application Examples

All engine symbols in these examples belong to `bg2e::app`.

## Persist a background frame limit

```cpp
#include <bg2e/app/all.hpp>
#include <cmath>

void loadSchedulingPreferences()
{
    using namespace bg2e::app;

    auto* loop = MainLoop::current();
    auto& prefs = PreferencesStore::instance().preferences("app");

    const double fps = prefs.get("backgroundMaxFrameRate", 1.0);
    if (std::isfinite(fps) && fps > 0.0) {
        loop->setBackgroundMaxFrameRate(fps);
    }

    loop->setBackgroundFrameRateLimitEnabled(
        prefs.get("backgroundFrameRateLimitEnabled", false));
}

void saveSchedulingPreferences()
{
    using namespace bg2e::app;

    auto* loop = MainLoop::current();
    auto& prefs = PreferencesStore::instance().preferences("app");
    prefs.set("backgroundMaxFrameRate", loop->backgroundMaxFrameRate());
    prefs.set("backgroundFrameRateLimitEnabled",
              loop->backgroundFrameRateLimitEnabled());
    prefs.save();
}
```

## Wake frame-driven dispatch from a worker

```cpp
void publishRequestFromWorker()
{
    // Store the request in the application's thread-safe queue first.
    if (auto* loop = bg2e::app::MainLoop::current()) {
        loop->requestFrame();
    }
}
```

The next polling iteration renders even if a long fractional-FPS deadline is
pending. The regular background cadence resumes afterward.

## Confirm application exit

```cpp
using namespace bg2e::app;

MainLoop::current()->setOnExitFunction([] {
    const int result = MessageBox::showWarning(
        "Exit",
        "Close the application?",
        {
            { 0, "Cancel", MessageBox::Esc },
            { 1, "Close", MessageBox::Return }
        }
    );
    return result == 1;
});
```

## Maintain a recent-scene list

```cpp
using namespace bg2e::app;

auto& history = FileHistory::get();
history.registerType("Scene", "glTF scene", { "gltf", "glb" });

auto selected = FileDialog::getOpenFilePath(history.filtersFor("Scene"));
if (!selected.empty()) {
    history.add("Scene", selected);
}
```

