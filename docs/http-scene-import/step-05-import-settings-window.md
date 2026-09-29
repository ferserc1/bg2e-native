# Step 05 — ImportSettingsWindow + ToolBar menu entry

## Goal

`File > Import Settings...` opens a floating window with exactly two settings:
the TCP port (editable only while the service is stopped) and an
enabled/disabled switch. Invalid ports are rejected; an occupied port triggers
a `bg2e::app::MessageBox` alert suggesting another port. All UI goes through
the `bg2e::ui` wrapper — never ImGui directly.

## Files to create

- `apps/bg2e_composer/src/ImportSettingsWindow.hpp`
- `apps/bg2e_composer/src/ImportSettingsWindow.cpp`

## Window class

Follows the `RenderSettingsWindow` pattern (`bg2e::ui::Window` subclass,
`init()` + `setDrawFunction`, drawn from `AppDelegate::drawUI()` when open).

```cpp
#pragma once

#include <bg2e/ui/Window.hpp>
#include <string>

class ImportServer;
class ImportSettings;

class ImportSettingsWindow : public bg2e::ui::Window {
public:
    void init(ImportServer * server, ImportSettings * settings);

private:
    void drawUI();

    ImportServer * _server = nullptr;
    ImportSettings * _settings = nullptr;
    std::string _portText;      // editable copy; committed on successful enable
};
```

```cpp
void ImportSettingsWindow::init(ImportServer * server, ImportSettings * settings)
{
    _server = server;
    _settings = settings;
    _portText = std::to_string(_settings->port());
    setTitle("Import Settings");
    setSize(320, 120);
    close();                       // hidden until opened from the menu

    setDrawFunction([this]() { drawUI(); });
}

void ImportSettingsWindow::drawUI()
{
    const bool running = _server->isRunning();

    // Port: disabled while the service is running (new Value::text parameter,
    // step 01). Only digits are kept; committed when the service is enabled.
    if (!running)
    {
        bg2e::ui::Value::text("Port", _portText, 6 /* maxLength */, false, false);
    }
    else
    {
        bg2e::ui::Value::text("Port", _portText, 6, false, true /* disabled */);
    }

    bool enabled = running;
    if (bg2e::ui::Button::checkBox("Service enabled", &enabled))
    {
        if (enabled)
        {
            uint32_t port = 0;
            if (!ImportSettings::parsePort(_portText, port))
            {
                bg2e::app::MessageBox::showError("Import Service",
                    "Invalid port. Use a number between 1024 and 49151.");
            }
            else
            {
                auto error = _server->start(port);
                if (!error.empty())
                {
                    bg2e::app::MessageBox::showError("Import Service",
                        error + ". Try a different port.");
                }
                else
                {
                    _settings->setPort(port);
                    _settings->setServiceEnabled(true);
                    _settings->save();
                }
            }
        }
        else
        {
            _server->stop();
            _settings->setServiceEnabled(false);
            _settings->save();
        }
    }

    bg2e::ui::Text::separator();
    if (running)
    {
        bg2e::ui::Text::text("Listening on 127.0.0.1:" + std::to_string(_server->port()));
    }
    else
    {
        bg2e::ui::Text::text("Service stopped");
    }
}
```

Notes:

- The checkbox state mirrors `isRunning()` (the source of truth), not the raw
  preference — if startup auto-start failed, the window shows "stopped".
- On a failed start, `_portText` keeps the typed value so the user can edit it
  immediately.
- The window refreshes `_portText` from settings when opened
  (`open()` override or `AppDelegate` sets it before `open()`), so it never
  shows a stale value.

## ToolBar change

### Modify: `apps/bg2e_composer/src/ToolBar.hpp/.cpp`

`ToolBar::init` gains one pointer parameter (same style as the existing
`_uiSettingsWindow` / `_renderSettingsWindow`):

```cpp
void ToolBar::init(AppDelegate * delegate,
                   bg2e::ui::UISettingsWindow * uiSettings,
                   bg2e::ui::RenderSettingsWindow * renderSettings,
                   ImportSettingsWindow * importSettings);   // NEW
```

Menu entry in the `File` menu, after the export item's separator group and
before `Quit`:

```cpp
file.addMenuItem({});   // Separator
file.addMenuItem({ "Import Settings...", {
    .handler = [&]()
    {
        _importSettingsWindow->open();
    }
}});
file.addMenuItem({});   // Separator
file.addMenuItem({ "Quit", { ... }});
```

## Integration points

- `AppDelegate` owns the window instance, calls `init(&_importServer,
  &_importSettings)` in `initWorkspace()`, and passes it to `ToolBar::init`.
- `AppDelegate::drawUI()` draws it when `isOpen()` (same block as the other
  two settings windows).

## Verification

- Menu opens/closes the window.
- While stopped: port editable; enable with `abc` / `80` / `50000` → error
  dialog; enable with a busy port (e.g. `python3 -m http.server 8643`) →
  "already in use" dialog; enable with a free port → status line shows
  listening.
- While running: port field is greyed out (validates step 01).
