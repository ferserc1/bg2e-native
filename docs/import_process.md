# Asynchronous glTF Import — Implementation Plan

## Goal

The glTF import in Composer (menu `File > Import GLTF Scene` and the HTTP
import service) currently runs `bg2e::db::loadGltf()` synchronously on the
main thread, freezing the UI for the whole import. This plan moves the import
to the same asynchronous model used by `File > Open Scene`
(`AppDelegate::asyncLoadGuarded` → `MainLoop::asyncLoad` → modal
`ui::Loader` on a worker thread), adding progress reporting to the glTF
loader.

Requirements confirmed with the user:

1. Fix the `MainLoop::_safeUpdateScene` vector race condition with a mutex,
   keeping in mind `safeUpdateScene` can also be called from the main thread
   itself (including from inside a lambda executed by
   `executeSafeUpdateScene`).
2. Drag-and-drop must be blocked while an import (or any async load) is in
   progress.
3. `MessageBox` calls from the worker thread: keep the existing `openScene`
   pattern, do not change it.
4. The import must always block the UI with the modal loader, both for the
   menu path and for the HTTP import service path.
5. `importModelBg2` (.bg2/.vwglb) is out of scope; do not modify it.
6. The HTTP service accepts one import at a time. A second `POST /import`
   receives `503 Service Unavailable` with a busy message, without being
   queued. The accepted request waits without an application timeout until
   the import has finished and Composer has restored its UI.

## Current state

- `SceneImporter::processOne()` (`apps/bg2e_composer/src/SceneImporter.cpp:59`)
  runs on the main thread once per frame and calls
  `StageScene::importGltfScene()` synchronously.
- `bg2e::db::loadGltf()` (`lib/src/bg2e/db/scene_gltf.cpp:515`) does all heavy
  work inline: cgltf parse, image decode + temp PNG writes, vertex
  decompression, tangent generation, and `Drawable::load(engine)` GPU uploads.
- Scene loading already uses the async model: `ToolBar.cpp:59` →
  `AppDelegate::asyncLoadGuarded` (`AppDelegate.cpp:52`) →
  `MainLoop::asyncLoad` (`lib/src/bg2e/app/MainLoop.cpp:374`). The scene is
  paused, a detached worker thread runs the load lambda with a mutex-protected
  `ui::Loader`, and completion is posted back through `_mainThreadQueue`.
- GPU resource creation on the worker thread is safe thanks to the dedicated
  immediate-submit queue (see `docs/immediate-submit-dedicated-queue/`).
- `_asyncLoadsInProgress` (`AppDelegate`) already gates
  `SceneImporter::processQueue()`, so imports are automatically serialized
  while any async load runs.
- `openScene()` already calls `setEditableRoot()` → `safeUpdateScene()` from
  the worker thread, so the cross-thread scene-mutation pattern this plan
  relies on is already in use.

## Target threading model

| Thread | Work |
|---|---|
| HTTP worker | validate + reserve the single import slot + enqueue + wait for completion or service stop; reject concurrent imports |
| Main | `processQueue` table lookup, modal `Loader` rendering, scene mutations via `safeUpdateScene`, selection changes |
| asyncLoad worker | cgltf parse, image decode, mesh build, `Drawable::load` GPU uploads, progress callbacks |

Both entry points (menu and HTTP) route through
`AppDelegate::asyncLoadGuarded`, so the UI always shows the modal loader and
the scene is paused during import.

The HTTP result is published on the main thread **after** the queued scene
changes have run, the loader frame override has been cleared, and the scene
has resumed. The server's occupied slot is released when this result is
published. This is the completion point for HTTP `200` or import `500`.

---

## Step 1 — `MainLoop`: thread-safe `safeUpdateScene`

| File | Action |
|---|---|
| `lib/include/bg2e/app/MainLoop.hpp` | modify |
| `lib/src/bg2e/app/MainLoop.cpp` | modify |

### Problem

`safeUpdateScene()` pushes into `_safeUpdateScene` without synchronization
(`MainLoop.hpp:150`), but it is called from the asyncLoad worker thread
(`openScene` → `setEditableRoot`, and the new import path). In addition, a
lambda executed by `executeSafeUpdateScene()` may itself call
`safeUpdateScene()` (same-thread re-entry), which today would mutate the
vector while it is being iterated.

### Design

Use the same swap-out pattern as `drainMainThreadQueue()`: lock only to
push / to swap the vector into a local copy, and execute the lambdas without
holding the mutex. This makes the queue safe from any thread and allows
same-thread nested calls without a recursive mutex or deadlock. A lambda
queued from inside an executing lambda simply runs on the next frame.

### `MainLoop.hpp`

New member next to `_safeUpdateScene`:

```cpp
std::mutex _safeUpdateSceneMutex;
```

`safeUpdateScene` becomes:

```cpp
void safeUpdateScene(std::function<void()> fn, std::shared_ptr<SafeUpdateToken> token = nullptr)
{
    std::lock_guard lock(_safeUpdateSceneMutex);
    _safeUpdateScene.emplace_back(std::move(fn), std::move(token));
}
```

### `MainLoop.cpp` — `executeSafeUpdateScene()`

```cpp
void MainLoop::executeSafeUpdateScene()
{
    std::vector<std::pair<std::function<void()>, std::shared_ptr<SafeUpdateToken>>> local;
    {
        std::lock_guard lock(_safeUpdateSceneMutex);
        if (_safeUpdateScene.empty())
        {
            return;
        }
        std::swap(local, _safeUpdateScene);
    }

    _engine.device().waitIdle();
    for (auto& [fn, token] : local)
    {
        if (!token || token->alive->load())
        {
            fn();
        }
    }
}
```

### Notes

- `executeSafeUpdateScene()` keeps running every frame, also while the scene
  is paused by `asyncLoad`, so worker-queued mutations are applied while the
  modal loader is visible.
- Intentional behavior change (bug fix): a `safeUpdateScene` issued from
  inside an executing safe-update lambda now runs on the next frame instead
  of being a data race / potential iterator invalidation.

---

## Step 2 — `bg2e::db::loadGltf`: progress callback

| File | Action |
|---|---|
| `lib/include/bg2e/db/scene_gltf.hpp` | modify |
| `lib/src/bg2e/db/scene_gltf.cpp` | modify |

Mirror the `db/scene.hpp` API: add a last parameter
`bg2e::scene::SceneProgressCallback onProgress = nullptr` to both `loadGltf`
overloads (`SceneProgressCallback` is already visible through the existing
`#include <bg2e/scene/Node.hpp>`):

```cpp
extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& filePath,
    render::Engine* engine,
    bg2e::scene::SceneProgressCallback onProgress = nullptr
);
```

### Progress accounting in `scene_gltf.cpp`

Only when `onProgress` is set:

1. **Total**: `total = <unique referenced images> + meshes_count`.
   Compute the referenced-image count in a first pass over
   meshes/primitives/materials collecting `const cgltf_image*` into an
   `std::unordered_set` (the base color, metallic-roughness and normal texture
   views). Using `data->images_count` directly would overcount unreferenced
   images.
2. **Image steps**: in the existing image pre-resolution loop
   (`scene_gltf.cpp:522`), report one step per image resolved:
   `onProgress("image <index>", processed, total)`. The lazy
   `TemporaryImages::resolve()` calls inside `materialAttributes()` hit the
   cache afterwards, so they do not need their own steps.
3. **Mesh steps**: in the drawable loop (`scene_gltf.cpp:571`), report one
   step per mesh after `drw->load(engine)`, using the drawable name:
   `onProgress(drw->name(), processed, total)`.

Callers must guard against `total == 0` (a glTF with no meshes/images) before
dividing.

The second overload (`basePath`, `fileName`) just forwards the callback.

---

## Step 3 — `StageScene`: progress-aware import overloads

| File | Action |
|---|---|
| `apps/bg2e_composer/src/StageScene.hpp` | modify |
| `apps/bg2e_composer/src/StageScene.cpp` | modify |

### Signature changes

Add a progress callback to both `importGltfScene` overloads and forward it to
`db::loadGltf`:

```cpp
void importGltfScene(
    const std::filesystem::path& path,
    bg2e::scene::SceneProgressCallback progressCallback = nullptr
);

std::shared_ptr<bg2e::scene::Node> importGltfScene(
    const std::filesystem::path& path,
    float unitsScale,
    bool sourceIsZUp,
    std::string& errorOut,
    std::shared_ptr<bg2e::scene::Node> parentOverride = nullptr,
    bg2e::scene::SceneProgressCallback progressCallback = nullptr
);
```

### Main-thread deselect

Both overloads currently call
`_appDelegate->selectionManager()->deselect()` as their first action
(`StageScene.cpp:244`, `StageScene.cpp:280`). Since the overloads will now run
on the asyncLoad worker thread, remove those calls and deselect on the main
thread in the callers **before** launching the async load:

- `ToolBar` handler (step 8).
- `SceneImporter::processOne` (step 6).

This preserves the documented semantics ("every import clears the current
selection first") while keeping `SelectionManager` on the main thread.

### What stays as-is

- `insertNewNode()` / `removeImportedNode()` already mutate the scene through
  `safeUpdateScene()`, which is safe from the worker thread after step 1.
- `Document::setUnsavedChanges()` (status bar text) is invoked from the worker
  thread via `insertNewNode`; `openScene` already does the same from its
  worker thread, so this is consistent with the existing pattern.
- `MessageBox::showError` in the interactive overload keeps running from the
  worker thread, exactly like `openScene` does today (per user decision 3).

---

## Step 4 — Completion after Composer resumes

Add an optional main-thread completion callback to `MainLoop::asyncLoad` and
pass it through `AppDelegate::asyncLoadGuarded` (headers and sources for both).
Queue the completion as a final `safeUpdateScene` callback after the worker's
scene mutations. It clears the frame override, resumes the scene, and invokes
the completion callback. This ordering also holds if the worker finishes
after the current frame has drained scene updates. Decrement
`_asyncLoadsInProgress` after restoration. Preserve existing callers without a
callback. Ensure worker exceptions still reach completion and restore the UI.

## Step 5 — `ImportServer`: one occupied slot

Modify `ImportServer.hpp` and `.cpp`. Replace the queue/result map with one
mutex-protected request slot. Reserve it atomically when a valid request is
accepted; taking the request on the main thread must not release it. Keep the
slot occupied until the main-thread completion callback publishes the result.
A second valid `POST /import` returns JSON `503 Service Unavailable` immediately
with an "Import service busy" message and is not queued. `503` describes a
temporary inability to serve the request; `409` would describe a conflict
with the target resource state.

Remove `kRequestTimeout` and wait on the completion condition variable without
an application deadline. Guard both the predicate and its notification against
missed wakeups. On stop, fail and notify the active request before joining HTTP
workers; restart with an empty slot. A client or intermediary may still impose
its own timeout.

Replace the single HTTP worker with enough concurrent handlers to respond to
another import and `GET /status` while the first handler waits. Check the
library's bounded task-queue behavior: a second request must get a prompt
`503`, not silently sit waiting for a worker. Keep `/status` responsive and
report `busy`; remove `queued` unless compatibility requires returning zero.

## Step 6 — `SceneImporter`: asynchronous processing

| File | Action |
|---|---|
| `apps/bg2e_composer/src/SceneImporter.hpp` | modify |
| `apps/bg2e_composer/src/SceneImporter.cpp` | modify |
| `apps/bg2e_composer/src/AppDelegate.cpp` | modify (ctor call only) |

### Constructor

`SceneImporter` needs access to `AppDelegate::asyncLoadGuarded`. Add an
`AppDelegate*` parameter and member:

```cpp
SceneImporter(StageScene * stage, AppDelegate * appDelegate);
```

Update the construction in `AppDelegate::initWorkspace()`
(`AppDelegate.cpp:276`) to pass `this`.

### Main-thread table ownership and request flow

Keep `_table` entirely on the main thread: `sweep()`, `clear()`, lookup and
registration need no new mutex. Change `processQueue()` to take at most the
single admitted request from `ImportServer`. Pass `AppDelegate*` to the
`SceneImporter` constructor for `asyncLoadGuarded()`.

On the main thread, look up the previous imported node and its parent, then
deselect. The worker calls the progress-aware `StageScene::importGltfScene`,
records the result and error in shared completion state, and, on success,
queues removal of the old node after insertion of the new one. A failed
replacement leaves the old node untouched. **Do not** call `server.fulfil()`
or update `_table` from the worker.

In the main-thread completion callback, after safe scene updates and UI
restoration, verify that the new node is attached, update `_table` on success,
and call `server.fulfil()` with success or the import error. Capture the request,
previous node, parent and result with appropriate ownership. A late callback
after service stop must not complete a later request; use the request ID when
publishing the result.

The server's occupied slot and `_asyncLoadsInProgress` together prevent
concurrent HTTP imports. The latter remains active until UI restoration. Scene
insertion, old-node removal and completion are queued FIFO in the safe-update
queue, so the mutations are applied before the completion callback.

---

## Step 7 — `AppDelegate::fileDropped`: block during async loads

| File | Action |
|---|---|
| `apps/bg2e_composer/src/AppDelegate.cpp` | modify |

SDL drop events are still processed while the modal loader is shown, so a
drop during an import would start a nested `asyncLoad`. Add an early return
at the top of `fileDropped()` (`AppDelegate.cpp:129`):

```cpp
if (_asyncLoadsInProgress.load() > 0)
{
    return;
}
```

The menu is unreachable while the frame override is active, so no equivalent
guard is needed in the `ToolBar` menu handlers.

---

## Step 8 — `ToolBar`: interactive import through `asyncLoadGuarded`

| File | Action |
|---|---|
| `apps/bg2e_composer/src/ToolBar.cpp` | modify |

Route `File > Import GLTF Scene` (`ToolBar.cpp:84`) through the same pattern
as `Open Scene`:

```cpp
file.addMenuItem({ "Import GLTF Scene", {
    .handler = [&]()
    {
        bg2e::app::FileDialog fd;
        fd.setFilters({
            { "glTF scene", "gltf,glb" }
        });
        auto filePath = fd.openFile();

        if (!filePath.empty())
        {
            _appDelegate->selectionManager()->deselect();
            _appDelegate->asyncLoadGuarded([&, filePath](bg2e::ui::Loader* loader)
            {
                _appDelegate->stage()->importGltfScene(filePath,
                    [loader](const std::string& name, int processed, int total) {
                        loader->setMessage("Importing " + name + "...");
                        loader->setProgress(total > 0
                            ? static_cast<float>(processed) / static_cast<float>(total)
                            : 0.0f);
                    });
            }, glm::vec4{ 0.2, 0.2, 0.31, 1.0f });
        }
    }
}});
```

`File > Import bg2 Model` is intentionally untouched (user decision 5).

---

## Step 9 — Documentation and comments

| File | Action |
|---|---|
| `apps/bg2e_composer/src/ImportServer.hpp` | document single-slot admission and completion after UI restoration |
| `doc/apps/composer/import_service.md` | document async flow, `503` busy response, no service-side timeout, `/status` busy state and progress |

## Step 10 — Check `model_edit` compatibility

`apps/model_edit/src/StageScene.cpp` calls `db::loadGltf(path, _engine)`
without a progress callback. Its menu and file-drop handlers call that same
`StageScene::importGltf()` method synchronously; they do not use
`MainLoop::asyncLoad`. Keep the new `loadGltf` callback optional with a default
`nullptr`, and keep the existing two-argument call valid. Also keep any new
`MainLoop::asyncLoad` completion callback optional. With those API defaults,
`model_edit` needs **no source changes** and retains its current behavior.
Review these call sites again after editing the shared declarations to ensure
they remain compatible. The example using the other `loadGltf` overload also
remains compatible via its default callback.

## Known limitations (not addressed by this plan)

- Application shutdown while an async load worker is still running (detached
  thread) is a pre-existing limitation shared with `openScene`; out of scope.
- `Document` status-bar updates from the worker thread: consistent with the
  existing `openScene` pattern; out of scope.

## Verification checklist

1. Review the changes and, only if explicitly requested, build the engine and
   `bg2e_composer` (repository instructions prohibit compiling otherwise).
2. Menu import: import a large `.glb`; confirm the modal loader appears, the
   progress bar advances (image steps, then mesh steps), the UI is blocked,
   and the scene resumes correctly afterwards with the wrapper node in place.
3. HTTP import (`POST /import` with curl/Postman) on the same file: confirm
   the modal loader also appears, the request returns `200` after completion,
   and the wrapper node is inserted.
4. Reimport the same path via HTTP: confirm replacement hangs from the same
   parent and the old node is removed.
5. While an import is running: try drag-and-drop of a `.json`/`.vitscnj`
   scene (must be ignored) and send a second `POST /import` (must promptly return `503`;
   `GET /status` remains responsive and reports busy). Retry after completion
   and confirm success.
6. Import an invalid/corrupt glTF via both paths: confirm error message
   (dialog for menu path, HTTP 500 for the service) and that the UI recovers.
7. Import a glTF with no meshes/textures: no division-by-zero in the progress
   lambda.
8. Run with Vulkan validation layers enabled: no new validation errors during
   worker-thread GPU uploads.

9. Confirm HTTP `200` is returned only after the imported node is attached,
   the loader is closed, and Composer is interactive. Stop/restart the service
   during a pending request: the handler must wake without deadlock, and a new
   request must be accepted after restart.
