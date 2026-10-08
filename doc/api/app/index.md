# Application API

The `bg2e::app` namespace owns the windowed application lifecycle, SDL input
routing, runtime scheduling, preferences, shortcuts, native dialogs, recent-file
history, and the offscreen application entry point.

The main entry point for an interactive application is [`MainLoop`](MainLoop.md).
It creates the SDL window and production rendering engine, connects the delegates
stored by `Application`, processes events, and submits frames until the
application exits.

```cpp
#include <bg2e/app/all.hpp>
```

---

## Architecture

```text
Application
  |-- render delegate
  |-- input delegate
  `-- user-interface delegate
           |
           v
       MainLoop
         |-- SDL window and events
         |-- input routing and shortcuts
         |-- timers and safe main-thread work
         |-- frame scheduling
         `-- rendering lifecycle
```

`Application` stores the delegates selected by the program. `MainLoop` owns the
runtime objects and is a singleton while it exists; code running inside an
application can access it through `MainLoop::current()`.

## Background frame-rate limiting

`MainLoop` can limit complete frames while the window does not have input focus.
The feature is disabled by default. When enabled, foreground rendering remains
unrestricted, while background `update + render + present` work runs no faster
than the configured rate.

```cpp
bg2e::app::MainLoop loop("org.example.editor");
loop.setBackgroundMaxFrameRate(1.0);
loop.setBackgroundFrameRateLimitEnabled(true);
```

Fractional rates are supported: `0.5` means one frame every two seconds and
`0.1` means one frame every ten seconds. The value must be finite and greater
than zero.

Between background frames the main thread continues to process SDL events,
timers, queued safe updates, and other lightweight maintenance, sleeping in
short bounded intervals to reduce CPU use. Losing focus therefore substantially
reduces GPU submission without suspending the application. Minimized windows
remain a separate state and do not render.

See [MainLoop — Background frame-rate limiting](MainLoop.md#background-frame-rate-limiting)
for scheduling details and the asynchronous-work contract.

## Module areas

| Area | Main types |
|------|------------|
| Lifecycle and scheduling | [`Application`](Application_and_input.md#application), [`MainLoop`](MainLoop.md), `WindowConfig`, `SafeUpdateToken` |
| Input | [`InputDelegate`](Application_and_input.md#inputdelegate), `InputManager`, `KeyEvent`, `Keyboard`, `Mouse` |
| Commands | `Shortcuts`, `ShortcutData` |
| Persistence | [`Preferences`, `PreferencesStore`](Preferences.md), [`FileHistory`](Platform_services.md#filehistory) |
| Native services | [`FileDialog`, `MessageBox`](Platform_services.md), `GPUSelectionDialog` |
| Headless rendering | [`OffscreenApplication`](OffscreenApplication.md), `OffscreenApplicationDelegate`, `OffscreenApplicationConfig` |
| Standalone tool | [`lightmap_generator`](LightmapGenerator.md) — headless model/prefab lightmap CLI |
| Safe UV2 editor reload | [`Uv2SafeReload`](Uv2SafeReload.md) |

## Where to go next

- [Quick start](quick_start.md) — short recipes using `bg2e::app` APIs only.
- [Examples](examples.md) — background scheduling, preferences, shortcuts, and dialogs.
- [API reference](reference.md) — header and symbol catalog.
- [MainLoop](MainLoop.md) — lifecycle, focus throttling, safe updates, timers, and asynchronous loading.
- [Application and input](Application_and_input.md) — delegate setup and event routing.
- [Preferences](Preferences.md) — global and scoped persistent settings.
- [Platform services](Platform_services.md) — files, history, and message boxes.
- [Uv2SafeReload](Uv2SafeReload.md) — schedule UV2 generation and loaded Drawable reload safely.
- [LightmapGenerator](LightmapGenerator.md) — standalone UV2 generation, headless baking, and output files.
