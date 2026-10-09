# Application API

The `bg2e::app` namespace owns the windowed application lifecycle, SDL input
routing, runtime scheduling, preferences, shortcuts, native dialogs, recent-file
history, and the offscreen application entry point.

The main entry point for an interactive application is [`MainLoop`](MainLoop.md).
It creates the SDL window and the selected rendering engine, connects the delegates
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

## Production and experimental frameworks

The terms **production** and **experimental** describe the maturity and scope of
bg2 engine's graphics APIs, not build configurations, deployment environments
or compiler optimization settings.

- **Production: `bg2e::render`.** This is the maintained framework used by the
  engine's applications. It provides Vulkan rendering and integrates with the
  production scene, material and UI components. Use it for applications that
  need those established high-level facilities. Production does not imply that
  an application must run on a server or use a Release build.
- **Experimental: `bg2e::draw`.** This is a high-level framework built on the
  backend-neutral `bg2e::gpu` API. Its windowed path supports Vulkan and Metal,
  retained scene color and separate UI composition. Its high-level scene and
  resource contracts have a narrower scope than render and can evolve as the
  API develops; render components cannot simply be passed to draw. Use it when
  evaluating the multi-backend architecture or building against its GPU-based
  delegate contract. Experimental does not mean the code is only a mock or that
  its window/UI path is unavailable.

Draw is intended to become the successor to render. Both APIs coexist; render
remains supported and is not deprecated. `bg2e::gpu` provides lower-level devices,
queues, commands and resources; it does not supply render's high-level scene
framework. Choosing Vulkan in draw does not select render.

## Framework and backend selection

The `MainLoop::run()` overload selects the high-level framework:

| Call | Framework | Low-level backend |
|---|---|---|
| `run(application)` | `bg2e::render` | Vulkan on every supported platform |
| `run(application, draw::EngineConfig{})` | `bg2e::draw` | Metal on macOS, Vulkan elsewhere by default |

Explicitly assigning EngineConfig::backend overrides draw's default. MainLoop
owns common window/event handling, input, scheduling, timers, preferences and
queued work. A private graphics execution strategy owns the selected Engine and
rendering coordinator; applications do not construct it.

Both routes validate delegates before SDL/GPU allocation. Draw initializes the
selected GPU and UI backends, preserving scene color while paused so UI and
presentation can continue independently. See
[execution selection](MainLoop.md#execution-selection-and-validation) and
[draw API](../draw/index.md).

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

## Implementation architecture

See [MainLoop, render, draw and gpu](../../architecture/MainLoop_render_draw_gpu.md)
for source excerpts, execution strategies, ownership, frame flow, UI integration,
asynchronous work and synchronization boundaries.
