# HTTP Scene Import Service

`bg2e_composer` exposes a localhost-only HTTP service that lets external tools
(DCC applications, scripts, Postman during development) push glTF scenes into
the running Composer session without user interaction. All of the service code
lives at application level (`apps/bg2e_composer/src`); the engine itself is
untouched except for one small UI helper.

## Part 1 — Using the service

### Startup and default port

The service is configured through the `File > Import Settings...` menu entry
and persists its state across sessions. On application startup:

- If the service was enabled the last time Composer ran (the default), the
  server starts automatically on the configured port.
- The **default port is `8643`**.
- The server binds to `127.0.0.1` only; it is never reachable from other
  machines. There is no TLS.
- If the port is already occupied, the service does not start and an error is
  shown in the status bar (`Import service failed: ...`).

### Settings window

`File > Import Settings...` opens the **Import Settings** window, which shows:

- **Port** — a text field with the TCP port. It is editable only while the
  service is stopped (the field is disabled while running). Valid range:
  `1024–49151`.
- **Service enabled** — a checkbox that starts/stops the server. Enabling it
  validates the port (an error dialog appears for invalid input or a busy
  port), persists the new settings, and starts listening. Disabling stops the
  server immediately; any in-flight request fails.
- A status line at the bottom shows either `Listening on 127.0.0.1:<port>` or
  `Service stopped`.

Both settings (port and enabled flag) are persisted to
`preferences_import.json` and restored on the next launch.

### Endpoints

Base URL: `http://127.0.0.1:<port>` (default `http://127.0.0.1:8643`).

#### `GET /status`

Health check. Returns the running state, bound port, and the number of import
requests currently queued waiting for the main thread:

```json
{ "running": true, "port": 8643, "queued": 0 }
```

#### `POST /import`

Imports a glTF file from the local filesystem into the current scene.

- Header: `Content-Type: application/json` (mandatory).
- Body fields:

| Field | Type | Required | Description |
|---|---|---|---|
| `filePath` | string | yes | Absolute path to a `.gltf` or `.glb` file. The file must exist. |
| `fileName` | string | no | Informational name; defaults to the file name portion of `filePath`. |
| `units` | string | no (default `"m"`) | Source file units: `m`, `cm`, `mm`, `in`, `ft`. Drives a uniform scale on the imported wrapper node (meters per source unit: 1, 0.01, 0.001, 0.0254, 0.3048). Case-insensitive. |
| `coordinateSystem` | string | no (default `"y_up"`) | Source axis convention: `y_up` (glTF native, no conversion) or `z_up` (applies a −90° rotation on X to convert to Y-up). Case-insensitive. |

Example:

```sh
curl -X POST http://127.0.0.1:8643/import \
  -H "Content-Type: application/json" \
  -d '{"fileName":"DamagedHelmet.glb","filePath":"/abs/path/DamagedHelmet.glb","units":"cm","coordinateSystem":"z_up"}'
```

The request is **synchronous**: the HTTP response is not sent until the scene
has actually been modified on the main thread (or the import fails).

Responses:

| Status | Meaning |
|---|---|
| `200` | Import succeeded: `{"status":"ok","message":"Imported <fileName>"}` |
| `400` | Malformed JSON, missing `filePath`, unknown `units`/`coordinateSystem`, or unsupported extension (only `.gltf`/`.glb`) |
| `404` | `filePath` does not exist on disk |
| `415` | `Content-Type` is not `application/json` |
| `500` | The import failed on the main thread (message contains the reason) |
| `504` | The main thread did not process the request within 60 seconds |

### Import semantics

- The imported glTF subtree hangs from a **wrapper node** named after the file
  stem. The wrapper carries the unit scale and the optional Z-up→Y-up rotation
  in its transform, so the source asset is never modified.
- Every import (HTTP or `File > Import GLTF Scene`) **clears the current
  selection first**, so the new content never hangs from a node that is about
  to be removed. On a first import the wrapper hangs from the primary selected
  node (or the editable root if nothing is selected); on a **reimport** it
  hangs from the **same parent as the previous imported node**, regardless of
  the current selection.
- The wrapper node (and every light/camera/environment/drawable node inside
  the imported subtree) is **selectable**: it receives a
  `SelectableComponent` so it can be picked in the viewport, plus a
  `GizmoComponent` for the transform/type gizmo — exactly like any node
  created through the Composer UI.
- The service keeps a table mapping `file path → imported node instance`
  (tracked by instance identity, never by name, so user renames are
  irrelevant). Re-importing the same path **replaces** the previous node.
- If the user deletes an imported node, closes the scene, or opens another
  scene, the table entry is cleaned up automatically and the path can be
  imported again from scratch.

## Part 2 — Implementation

All classes live in `apps/bg2e_composer/src`. The design follows one strict
rule: **the HTTP worker thread never touches the scene graph, Vulkan, or
imgui**. It only validates requests, enqueues them, and blocks until the main
thread fulfils them.

### Architecture overview

```
Postman / DCC tool
       │  POST /import  {fileName, filePath, units, coordinateSystem}
       ▼
┌──────────────── HTTP worker thread (cpp-httplib) ───────────────┐
│ ImportServer::handleImport                                       │
│   validate JSON → enqueue ImportRequest → wait condition_variable│
└──────────────────────────────────────────┬───────────────────────┘
                                           │ mutex + deque + result map
                                           ▼
┌──────────────── Main / render thread ───────────────────────────┐
│ AppDelegate::update()  (once per frame)                          │
│   └─ if (no async load in progress)                              │
│        SceneImporter::processQueue(ImportServer&)                │
│          ├─ sweep stale table entries                            │
│          ├─ lookup path → alive? remove old node : first import  │
│          ├─ StageScene::importGltfScene(path, scale, zUp, err)   │
│          └─ register path → node, ImportServer::fulfil(id, ...)  │
│ HTTP worker wakes → 200 / 4xx / 500 / 504                        │
└──────────────────────────────────────────────────────────────────┘
```

### `ImportRequest` (ImportRequest.hpp)

A plain POD (no dependency on httplib or scene types) describing one pending
import, plus the `ImportCoordinateSystem` enum (`YUp`, `ZUp`):

- `id` — assigned by `ImportServer`, used to match the result slot.
- `fileName` — informational / fallback node name.
- `filePath` — validated absolute path.
- `unitsScale` — meters per source unit (already converted from the `units`
  string by the server).
- `coordinateSystem` — source axis convention.

### `ImportServer` (ImportServer.hpp/.cpp)

Facade over a vendored, header-only **cpp-httplib 0.58.0**
(`apps/third_party/cpp-httplib`, added to the target include dirs in
`apps/bg2e_composer/CMakeLists.txt`). `httplib.h` is included **only** in
`ImportServer.cpp`; the header uses a pImpl and forward-declared
`httplib::Request`/`Response` so no other translation unit sees httplib.

State:

- `Impl` — holds the `httplib::Server` (in a `unique_ptr`, recreated on every
  `start()` because a stopped `httplib::Server` owns atomics/threads and
  cannot be reused), the listen thread, the bound port, and a
  `condition_variable` + paired mutex used to wake blocked handlers.
- `_mutex`, `_queue` (`std::deque<ImportRequest>`), `_results`
  (`id → shared_ptr<Slot>`), `_nextId` — the producer/consumer queue shared
  with the main thread. Each `Slot { done, ok, message }` is the per-request
  completion cell.

Public API:

- `start(port)` — validates the range (1024–49151), installs socket options,
  registers routes, and binds **synchronously** with
  `bind_to_port("127.0.0.1", port)` so a busy port is reported before the
  function returns (`"Port N is already in use"`). The listen loop runs on a
  dedicated `std::thread` via `listen_after_bind()`. Socket options are
  overridden (`SO_REUSEADDR` on POSIX, `SO_EXCLUSIVEADDRUSE` on Windows)
  because httplib's default `SO_REUSEPORT` would silently share the port
  between two processes. `new_task_queue` is forced to a
  `ThreadPool(1, 1)` so **all requests are serialized on a single worker
  thread**.
- `stop()` — stops the server, joins the thread, destroys the `Server`
  instance, then marks every pending slot as failed (`"Service stopped"`),
  clears the queue and notifies all waiters so no handler outlives the server.
  Safe to call when already stopped; also called from the destructor.
- `popPending()` — main-thread side: drains the whole queue into a vector.
- `fulfil(id, ok, message)` — main-thread side: writes the outcome into the
  slot and notifies the condition variable, waking the blocked HTTP handler.
- `isRunning()`, `port()`, `pendingCount()` — status accessors used by the UI
  and by `GET /status`.

`handleImport()` (the only route handler with logic) runs on the worker thread
and performs, in order:

1. `Content-Type` check (case-insensitive) → `415`.
2. JSON parse with `bg2e::json::JsonParser` → `400` on malformed/non-object
   bodies.
3. Field extraction: `filePath` (required → `400`), `fileName` (defaults to
   the path's filename), `units` via `unitsToScale()` (`m/cm/mm/in/ft`,
   case-insensitive → `400` if unknown), `coordinateSystem`
   (`y_up`/`z_up`, default `y_up` → `400` if unknown).
4. Filesystem validation: file must exist (`404`) and have a `.gltf`/`.glb`
   extension, case-insensitive (`400`).
5. Enqueue: creates the `Slot`, assigns the id, pushes the request.
6. **Block** on the condition variable with a 60-second timeout
   (`kRequestTimeout`). The predicate re-reads the slot under `_mutex` (which
   guards the slot state written by `fulfil()`/`stop()`), while the paired
   `cvMutex` only protects the wait itself.
7. Build the response: `504` on timeout, `500` on failed fulfil, `200`
   otherwise. Response bodies are hand-built JSON; a local `jsonEscape()`
   helper escapes the message field, and a `fail()` helper produces the
   `{"status":"error","message":...}` shape.

`GET /status` simply serializes `{running, port, queued}`.

### `ImportSettings` (ImportSettings.hpp/.cpp)

Typed wrapper over `bg2e::app::Preferences("import")` (persisted to
`preferences_import.json`), following the `RenderSettingsPreferences` style:

- Constants: `DefaultPort = 8643`, `MinPort = 1024` (below: privileged),
  `MaxPort = 49151` (above: ephemeral/dynamic range).
- Keys: `port` (uint32, default 8643) and `serviceEnabled` (bool, default
  true), with `load()`/`save()` pass-throughs.
- `parsePort(text, out)` — static validator for the UI text field: rejects
  empty or non-numeric input, catches `std::stoul` `out_of_range`, and enforces
  the `[MinPort, MaxPort]` range.

### `SceneImporter` (SceneImporter.hpp/.cpp)

Main-thread consumer that owns the **import table**
(`_table: canonical path string → ImportEntry { weak_ptr<Node>, identifier }`).

- `tableKey(path)` — `std::filesystem::weakly_canonical(path).string()`, so
  equivalent paths map to the same entry.
- `sweep()` — runs every frame before processing: erases entries whose node
  expired (`weak_ptr`), whose node was detached from the scene
  (`parent() == nullptr`, e.g. the user deleted it), or whose `identifier()`
  no longer matches (the slot now holds a different node). This guarantees the
  table never holds dangling entries.
- `processQueue(server)` — called once per frame: `sweep()`, then for each
  drained request, `processOne()`.
- `processOne(server, request)`:
  1. Looks up the canonical key; if a previous node exists and is still
     attached, it is kept aside for replacement together with its **parent**
     (a detached weak node is discarded).
  2. Calls `StageScene::importGltfScene(path, unitsScale, sourceIsZUp, error,
     parentOverride)`, passing the old node's parent as `parentOverride` on a
     reimport so the new wrapper hangs from the same place as the old one,
     regardless of the current selection. On failure, fulfils the request
     with `ok=false` and the error string.
  3. On success, removes the old node (`StageScene::removeImportedNode`),
     registers `key → { newNode, newNode->identifier() }`, and fulfils with
     `"Imported <fileName>"`.
- `clear()` — drops the whole table (scene swap, shutdown).

Tracking by instance (weak pointer + node identifier) rather than by name
means renaming an imported node in the editor never breaks replacement.

### `StageScene` import support (StageScene.hpp/.cpp)

Two members were added to the existing stage manager:

- `importGltfScene(path, unitsScale, sourceIsZUp, errorOut, parentOverride)` —
  the **programmatic** overload used by the service (the existing
  single-argument interactive overload that shows dialogs is untouched apart
  from the selection clearing described below).
  It:
  0. **Clears the current selection** (`SelectionManager::deselect()`) as its
     first action — both `importGltfScene` overloads do this, including the
     interactive one used by `File > Import GLTF Scene` — so an import never
     inserts content under a node that is about to be replaced or removed.
  1. Loads the glTF file with `bg2e::db::loadGltf` (returns `nullptr` + error
     string on failure; exceptions are caught and reported through
     `errorOut`).
  2. Calls `addGizmoComponents()` on the loaded root, which recursively adds a
     `GizmoComponent` to every node and a `SelectableComponent` to every
     light/environment/camera/drawable node that lacks one, so the imported
     subtree is pickable through gizmos in the viewport.
  3. Builds the **wrapper node** named after the file stem, with a
     `TransformComponent` whose matrix combines the optional −90° X rotation
     (`sourceIsZUp`) and the uniform `unitsScale` factor (skipped when 1.0).
     The loaded tree becomes the wrapper's only child.
  4. Inserts the wrapper under `parentOverride` when provided (reimport: the
     previous node's parent), otherwise under `newNodeParent()` (primary
     selected node, or the editable root), via `insertNewNode()`, and returns
     the wrapper.

- `insertNewNode(node, parent)` — shared insertion helper. Before inserting it
  ensures the node has a **`SelectableComponent`** (viewport picking) and a
  **`GizmoComponent`** (transform/type gizmo) — this is what makes the
  imported wrapper node itself selectable even though it has no drawable.
  The actual tree mutation runs inside
  `MainLoop::current()->safeUpdateScene()` (which performs the change at a
  point where the device is idle), refreshes the scene with
  `scene()->updateAll()`, and marks the document as modified.

- `removeImportedNode(node)` — dialog-free removal used for replacement.
  It first checks whether the node (or any node in its subtree, walking up
  parents) is part of the current selection — both the primary selected node
  and the multi-selection list — and calls `SelectionManager::deselect()` if
  so, avoiding dangling selection pointers. Then it detaches the node from its
  parent inside `safeUpdateScene()` and refreshes the scene.

### `ImportSettingsWindow` (ImportSettingsWindow.hpp/.cpp)

A `bg2e::ui::Window` subclass holding raw pointers to the `ImportServer` and
`ImportSettings` plus the editable `_portText` string. `init()` seeds the text
from the saved port, sets title/size, and installs the draw function. The
window starts closed.

`drawUI()` renders:

- `bg2e::ui::Value::text("Port", _portText, 6, false, running)` — the port
  field, **disabled while the server is running** (this uses the `disabled`
  parameter added to the engine's `Value::text()` wrapper, the only engine
  change required by the feature; it wraps the `InputText` call in
  `ImGui::BeginDisabled()/EndDisabled()`).
- A `Button::checkBox("Service enabled", ...)` that mirrors the running state:
  - Enabling validates the port with `ImportSettings::parsePort()` and calls
    `ImportServer::start()`; failures surface through
    `bg2e::app::MessageBox::showError()`. On success it persists
    port + enabled flag.
  - Disabling calls `stop()` and persists the flag.
- A separator and a status line (`Listening on 127.0.0.1:<port>` /
  `Service stopped`).

### `AppDelegate` integration (AppDelegate.hpp/.cpp)

`AppDelegate` owns all four components by value/`unique_ptr`:
`_importServer`, `_importSettings`, `_sceneImporter`, `_importSettingsWindow`,
plus the `std::atomic<int> _asyncLoadsInProgress` guard counter.

- **Initialization** (in `initWorkspace()`, once the stage exists): loads the
  preferences, constructs `SceneImporter` with the stage pointer, initializes
  the settings window and toolbar, and — if `serviceEnabled()` — auto-starts
  the server, reporting a bind failure in the status bar.
- **Per-frame pump** (`update()`): after the base delegate update, if no async
  load is in progress, calls `_sceneImporter->processQueue(_importServer)`.
  The async guard matters because `MainLoop::asyncLoad` creates GPU resources
  on a worker thread, which would race with import-time scene mutation.
- **`asyncLoadGuarded(loadFn, clearColor)`** — wraps `MainLoop::asyncLoad`,
  incrementing the counter before and decrementing it inside the completion
  lambda. Both the File > Open Scene menu path and the drag-and-drop `.json`/
  `.vitscnj` path route through it.
- **Scene swap** — the `StageScene::onSceneSwap` callback deselects everything
  and calls `_sceneImporter->clear()`, so opening/closing scenes invalidates
  the import table.
- **Shutdown** (`cleanup()`) — stops the server (failing any pending requests)
  and clears the importer before the stage is destroyed.
- **UI** — `drawUI()` draws the settings window when open.

### `ToolBar` menu entry (ToolBar.hpp/.cpp)

`ToolBar::init()` takes an extra `ImportSettingsWindow*` parameter and adds
`File > Import Settings...` between separators just before Quit; its handler
simply calls `_importSettingsWindow->open()`.

### Thread safety summary

- One HTTP worker thread total (`ThreadPool(1, 1)`); it only enqueues and
  waits.
- Queue and result slots are guarded by `ImportServer::_mutex`; the condition
  variable has its own paired mutex, and the wait predicate re-locks `_mutex`
  to read slot state, avoiding data races between `fulfil()`, `stop()`, and
  the handler.
- All scene mutation happens on the main thread inside
  `MainLoop::safeUpdateScene()`, skipped entirely while an async scene load is
  in progress.
- `stop()` fails every outstanding slot so no worker thread can outlive the
  server instance.
