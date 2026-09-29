# HTTP Scene Import Service for bg2e_composer

## Problem statement

`bg2e_composer` needs an automatic scene-import entry point reachable over HTTP on
localhost, so that external tools (DCC applications, scripts, Postman during
development) can push a scene into the running Composer session without user
interaction.

Requirements:

- `POST` endpoint receiving a JSON body with at least: file name, absolute local
  file path, units of measurement, and source coordinate system.
- Units must drive a scaling factor; the coordinate system must drive an axis
  conversion into the engine's native system (glTF convention: Y-up,
  right-handed, meters).
- A persistent table mapping `file path -> imported node instance` (by instance,
  never by name, so user renames are irrelevant). Re-importing a known path
  replaces the old node; if the user deleted the node from the scene, the table
  entry must be cleaned up so the file can be imported again (no leaks).
- The HTTP worker threads must never touch the scene graph directly; import runs
  on the main/render thread.
- A settings window (`File > Import Settings`) to enable/disable the service and
  set the TCP port (default `8643`, editable only while stopped, validated range,
  "port in use" alert via `bg2e::app::MessageBox`), persisted with
  `bg2e::app::Preferences`.
- Everything lives at **application level** (`apps/bg2e_composer`). The engine
  stays focused on graphics/scene; the only engine change is a `disabled`
  parameter on the one imgui wrapper function the new UI needs.

## Proposed solution

Four new app-level components plus small integrations:

- **`ImportServer`** — wraps `httplib::Server` (vendored cpp-httplib 0.58.0,
  header-only, no TLS), owns the listen thread, validates/parses JSON with
  `bg2e::json::JsonParser`, pushes requests into a thread-safe queue and blocks
  the HTTP worker on a per-request `condition_variable` until the main thread
  fulfils it (synchronous responses).
- **`ImportRequest` queue** — mutex + deque shared between the HTTP worker
  (producer) and the main thread (consumer).
- **`SceneImporter`** — main-thread consumer. Owns the import table
  (`path -> { weak_ptr<Node>, identifier }`), performs replacement logic, builds
  the wrapper node with unit scaling / axis conversion, and sweeps stale table
  entries every frame.
- **`ImportSettings`** — typed preferences wrapper over
  `bg2e::app::Preferences("import")` (port, enabled), following the
  `RenderSettingsPreferences` style.
- **`ImportSettingsWindow`** — `bg2e::ui::Window` subclass for configuration.
- **`AppDelegate::update()`** — the existing per-frame hook (called by
  `RenderLoop::acquireAndPresent` after scene init) pumps the queue. No engine
  lifecycle changes needed.

### Architecture diagram

```
 Postman / DCC tool
        │  POST /import  {fileName, filePath, units, coordinateSystem}
        ▼
┌─────────────────────────── HTTP worker thread ───────────────────────────┐
│ ImportServer                                                             │
│  httplib::Server (ThreadPool(1,1), SO_REUSEADDR/SO_EXCLUSIVEADDRUSE)     │
│  bind 127.0.0.1:<port>                                                   │
│  handler: validate JSON ──► enqueue ImportRequest ──► wait condvar       │
└──────────────────────────────────────────────┬───────────────────────────┘
                                               │ mutex + deque + result map
                                               ▼
┌─────────────────────────── Main / render thread ─────────────────────────┐
│ AppDelegate::update()            (once per frame, scene initialized)     │
│   └─ if (!asyncLoadInProgress) SceneImporter::processQueue()             │
│        ├─ sweep stale table entries (weak_ptr expired / detached)        │
│        ├─ lookup path ─► alive? remove old node : first import           │
│        ├─ StageScene: loadGltf + wrapper (unit scale, axis rotation)     │
│        │     via MainLoop::safeUpdateScene (waitIdle + mutate)           │
│        └─ register path -> new instance, fulfil request                  │
│ ImportServer handler wakes ─► HTTP 200 / 4xx / 500                       │
└──────────────────────────────────────────────────────────────────────────┘
```

## Files

| File | Action | Description |
|---|---|---|
| `apps/bg2e_composer/src/ImportRequest.hpp` | Create | POD describing one import (name, path, unit scale, coordinate system, request id). No httplib dependency. |
| `apps/bg2e_composer/src/ImportServer.hpp` | Create | HTTP server facade (pImpl hides httplib). start/stop/isRunning, popPending, fulfil. |
| `apps/bg2e_composer/src/ImportServer.cpp` | Create | Single TU including `httplib.h`; routes, JSON parse/validate, queue, condvar completion. |
| `apps/bg2e_composer/src/ImportSettings.hpp/.cpp` | Create | Preferences wrapper (`Preferences("import")`): port (8643), enabled (true), validation. |
| `apps/bg2e_composer/src/SceneImporter.hpp/.cpp` | Create | Import table + per-frame queue processing + wrapper-node creation + stale sweep. |
| `apps/bg2e_composer/src/ImportSettingsWindow.hpp/.cpp` | Create | Settings window (port input, enable checkbox, status, alerts). |
| `apps/bg2e_composer/CMakeLists.txt` | Modify | One line: add `apps/third_party/cpp-httplib` to the target include dirs. (Approved.) |
| `apps/bg2e_composer/src/AppDelegate.hpp/.cpp` | Modify | Own the new components; override `update()`; start/stop lifecycle; draw window; async-load guard. |
| `apps/bg2e_composer/src/ToolBar.hpp/.cpp` | Modify | `File > Import Settings...` menu item. |
| `apps/bg2e_composer/src/StageScene.hpp/.cpp` | Modify | Programmatic glTF import overload returning the wrapper node + error string (no modal dialogs). |
| `lib/include/bg2e/ui/Value.hpp` | Modify | Add `bool disabled = false` to the labeled `Value::text()` overload. |
| `lib/src/bg2e/ui/Value.cpp` | Modify | Implement `disabled` with `ImGui::BeginDisabled/EndDisabled`. |
| `doc/api/ui/Value.md` | Modify | Document the new `disabled` parameter. |

## Steps

1. [step-01-ui-wrapper-disabled-text.md](step-01-ui-wrapper-disabled-text.md) — engine UI wrapper: disabled text input + docs.
2. [step-02-import-settings.md](step-02-import-settings.md) — `ImportSettings` preferences wrapper.
3. [step-03-import-server.md](step-03-import-server.md) — `ImportRequest`, `ImportServer`, CMake include, threading.
4. [step-04-scene-importer.md](step-04-scene-importer.md) — `StageScene` programmatic import + `SceneImporter` (table, replacement, sweep).
5. [step-05-import-settings-window.md](step-05-import-settings-window.md) — settings window + ToolBar menu entry.
6. [step-06-appdelegate-integration.md](step-06-appdelegate-integration.md) — wiring, per-frame pump, lifecycle.
7. [step-07-testing-postman.md](step-07-testing-postman.md) — verification matrix.

## Thread safety notes

- cpp-httplib handlers run on its internal `ThreadPool`; we force
  `new_task_queue = [] { return new httplib::ThreadPool(1, 1); }` so all
  requests are serialized on one worker thread.
- The worker thread never touches the scene graph, Vulkan, or imgui. It only
  locks the queue mutex, enqueues an `ImportRequest`, and waits on a
  per-request `condition_variable` (60 s timeout -> HTTP 504).
- The main thread drains the queue inside `AppDelegate::update()` (called once
  per rendered frame, after `initScene()`, so the scene is guaranteed to exist).
  Actual tree mutation reuses `MainLoop::safeUpdateScene()` (`device().waitIdle()`
  at frame top) — identical safety properties as the existing menu import.
- Queue processing is skipped while a `MainLoop::asyncLoad` is in progress
  (async scene loading creates GPU resources on a worker thread); an
  app-level atomic counter tracks this.
- Import table entries hold `std::weak_ptr<bg2e::scene::Node>`; a per-frame
  sweep erases entries whose node expired or whose `parent() == nullptr`
  (detached by `Node::removeChild`). This prevents dangling entries when the
  user deletes an imported node, closes the scene, or opens another scene.
- Default httplib socket options use `SO_REUSEPORT` on POSIX (two processes may
  share the port silently). We override `set_socket_options` with
  `SO_REUSEADDR` (POSIX) / `SO_EXCLUSIVEADDRUSE` (Windows) so a busy port makes
  `bind_to_port()` fail and the UI can alert the user.
- No TLS: `CPPHTTPLIB_OPENSSL_SUPPORT` is never defined. Server binds to
  `127.0.0.1` only.
