# Step 05: Integrate in model_edit

## Files to Modify

- `apps/model_edit/src/AppDelegate.hpp`
- `apps/model_edit/src/AppDelegate.cpp`

## AppDelegate.hpp Changes

```cpp
// BEFORE:
#include "UIRenderSettingsWindow.hpp"

// AFTER:
#include <bg2e/render/RenderSettingsPreferences.hpp>
#include <bg2e/ui/UIRenderSettingsWindow.hpp>
```

Remove method declarations:
```cpp
// REMOVE these:
void saveSettings();
void restoreSettings();
```

Add member:
```cpp
protected:
    // ... existing members ...
    std::unique_ptr<bg2e::render::RenderSettingsPreferences> _renderPrefs;
```

## AppDelegate.cpp Changes

### `initWorkspace()` — Replace settings restoration

```cpp
// BEFORE:
void AppDelegate::initWorkspace()
{
    restoreSettings();
    // ... rest of init ...
    _renderSettingsWindow.init(this);
    // ...
}

// AFTER:
void AppDelegate::initWorkspace()
{
    // Initialize preferences and load saved settings
    _renderPrefs = std::make_unique<bg2e::render::RenderSettingsPreferences>(renderer());
    _renderPrefs->load();

    // Register timer: persist every 60 seconds, also persist on exit
    bg2e::app::MainLoop::current()->timeout().add([this]() -> bool {
        _renderPrefs->persist();
        return true;
    }, 60000, true);  // executeOnExit = true

    // ... existing workspace setup (unchanged) ...
    _renderSettingsWindow.init(renderer(), _renderPrefs.get());
    // ... rest of init ...
}
```

### Remove `saveSettings()` and `restoreSettings()`

Delete the entire implementations of both methods (approximately lines 148-267).

## Integration Points

- `renderer()` returns `RendererDeferred*` directly (template specialization from `DefaultRenderLoopDelegate<RendererDeferred>`)
- Timer fires `persist()` every 60 seconds and on exit
- `UIRenderSettingsWindow` now receives the renderer and prefs directly
- No changes needed in ToolBar.cpp — the exit handler shows a confirmation dialog, and the `executeOnExit` timer handles persistence before resource cleanup
