# Step 06: Integrate in bg2e_composer

## Files to Modify

- `apps/bg2e_composer/src/AppDelegate.hpp`
- `apps/bg2e_composer/src/AppDelegate.cpp`

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
    // bg2e_composer uses dynamic_cast since it inherits through a non-templated path
    auto* deferredRenderer = dynamic_cast<bg2e::render::RendererDeferred*>(rendererBase());
    _renderPrefs = std::make_unique<bg2e::render::RenderSettingsPreferences>(deferredRenderer);
    _renderPrefs->load();

    // Register timer: persist every 60 seconds, also persist on exit
    bg2e::app::MainLoop::current()->timeout().add([this]() -> bool {
        _renderPrefs->persist();
        return true;
    }, 60000, true);  // executeOnExit = true

    // ... existing workspace setup (unchanged) ...
    _renderSettingsWindow.init(deferredRenderer, _renderPrefs.get());
    // ... rest of init ...
}
```

### Remove `saveSettings()` and `restoreSettings()`

Delete the entire implementations of both methods (approximately lines 153-281).

## Integration Points

- `rendererBase()` returns `Renderer*`, so `dynamic_cast<RendererDeferred*>` is required
- Timer fires `persist()` every 60 seconds and on exit
- `UIRenderSettingsWindow` receives the deferred renderer and prefs directly
- No changes needed in ToolBar.cpp — same as model_edit
