# bg2e::app Quick Start Guide

These recipes use `bg2e::app` APIs only. Rendering, scene, and UI delegate
implementations are intentionally outside their scope.

---

## Table of Contents

1. [Include the module](#include-the-module)
2. [Configure a window](#configure-a-window)
3. [Limit background rendering](#limit-background-rendering)
4. [Change the limiter at runtime](#change-the-limiter-at-runtime)
5. [Request a prompt frame](#request-a-prompt-frame)
6. [Queue safe main-thread work](#queue-safe-main-thread-work)
7. [Read and write preferences](#read-and-write-preferences)
8. [Handle input](#handle-input)
9. [Register a shortcut](#register-a-shortcut)
10. [Open native dialogs](#open-native-dialogs)

## Include the module

```cpp
#include <bg2e/app/all.hpp>
```

## Configure a window

```cpp
bg2e::app::MainLoop loop("org.example.editor");
loop.initWindowConfig(
    bg2e::app::WindowConfig::maximized("Example Editor", true));
```

The final `true` persists the normal window size in the application preference
directory.

## Limit background rendering

```cpp
loop.setBackgroundMaxFrameRate(1.0);
loop.setBackgroundFrameRateLimitEnabled(true);
```

Foreground rendering remains unrestricted. After the window loses input focus,
complete frames are limited to 1 FPS. Events, timers, and queued work continue
to be serviced between frames.

Fractional values are useful for tools that should become almost idle:

```cpp
loop.setBackgroundMaxFrameRate(0.5);  // one frame every two seconds
loop.setBackgroundMaxFrameRate(0.1);  // one frame every ten seconds
```

## Change the limiter at runtime

```cpp
auto* loop = bg2e::app::MainLoop::current();
if (loop) {
    loop->setBackgroundMaxFrameRate(5.0);
    loop->setBackgroundFrameRateLimitEnabled(true);
}
```

Both setters are immediately visible through their corresponding getters.
Rates must be finite and greater than zero.

## Request a prompt frame

If background work creates a request that is normally consumed during an
application frame, request one explicitly:

```cpp
void onBackgroundRequestReceived()
{
    if (auto* loop = bg2e::app::MainLoop::current()) {
        loop->requestFrame();
    }
}
```

`requestFrame()` is thread-safe. It bypasses one pending background deadline;
it does not disable the limiter.

## Queue safe main-thread work

```cpp
bg2e::app::MainLoop::current()->safeUpdateScene([] {
    // Apply work that must run between frames on the main thread.
});
```

The queue automatically requests a prompt frame.

## Read and write preferences

```cpp
auto& prefs = bg2e::app::PreferencesStore::instance().preferences("editor");

bool autosave = prefs.get("autosave", true);
double interval = prefs.get("autosaveInterval", 60.0);

prefs.set("autosave", autosave);
prefs.set("autosaveInterval", interval);
bg2e::app::PreferencesStore::instance().saveAll();
```

## Handle input

```cpp
class Input final : public bg2e::app::InputDelegate {
public:
    void keyDown(const bg2e::app::KeyEvent& event) override
    {
        if (event.key() == bg2e::app::KeyEvent::KeyEscape) {
            bg2e::app::MainLoop::current()->exit();
        }
    }

    void fileDropped(const std::filesystem::path& path) override
    {
        lastDroppedFile = path;
    }

    std::filesystem::path lastDroppedFile;
};
```

## Register a shortcut

```cpp
bg2e::app::Shortcuts::ShortcutData saveShortcut;
saveShortcut.key = bg2e::app::KeyEvent::KeyS;
saveShortcut.ctrlModifier = true;
saveShortcut.handler = [] {
    // Save the current document.
};

bg2e::app::MainLoop::shortcuts().addShortcutMapper(saveShortcut);
```

See [reference.md](reference.md#shortcuts) for the exact modifier fields and
registration overloads.

## Open native dialogs

```cpp
bg2e::app::FileDialog::FileFilters filters {
    { "glTF scene", "gltf,glb" }
};

auto path = bg2e::app::FileDialog::getOpenFilePath(filters);
if (path.empty()) {
    bg2e::app::MessageBox::showInfo("Open", "No file was selected.");
}
```
