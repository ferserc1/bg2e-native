# Step 06 — AppDelegate integration (wiring, per-frame pump, lifecycle)

## Goal

Own the new components in `AppDelegate`, pump the import queue once per frame
on the main thread, and manage the server lifecycle. No engine lifecycle
changes: `RenderLoopDelegate::update(uint32_t, FrameResources&)` already exists
and is called once per rendered frame from `RenderLoop::acquireAndPresent`
(`lib/src/bg2e/render/RenderLoop.cpp:100`), always after `initScene()` — so the
scene is guaranteed initialized. `AppDelegate` doesn't override it yet.

## Files to modify

- `apps/bg2e_composer/src/AppDelegate.hpp`
- `apps/bg2e_composer/src/AppDelegate.cpp`

## Header additions

```cpp
#include "ImportServer.hpp"
#include "ImportSettings.hpp"
#include "SceneImporter.hpp"
#include "ImportSettingsWindow.hpp"

class AppDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    // ...
    void update(uint32_t currentFrame,
                bg2e::render::vulkan::FrameResources& frameResources) override;   // NEW

    // Tracks MainLoop::asyncLoad activity; the import queue is not processed
    // while a scene load is running on a worker thread (GPU resource creation
    // would race). Wraps the existing asyncLoad call sites.
    void asyncLoadGuarded(std::function<void(bg2e::ui::Loader*)> loadFn,
                          glm::vec4 clearColor);                                   // NEW

protected:
    // ...
    ImportServer _importServer;
    ImportSettings _importSettings;
    std::unique_ptr<SceneImporter> _sceneImporter;   // needs StageScene; built in initWorkspace
    ImportSettingsWindow _importSettingsWindow;
    std::atomic<int> _asyncLoadsInProgress { 0 };
};
```

## Implementation

### initWorkspace() — load prefs, build importer, init window, auto-start

```cpp
void AppDelegate::initWorkspace()
{
    // ... existing code ...

    _importSettings.load();
    _sceneImporter = std::make_unique<SceneImporter>(_stage.get());
    _importSettingsWindow.init(&_importServer, &_importSettings);
    _toolBar.init(this, &_uiSettingsWindow, &_renderSettingsWindow,
                  &_importSettingsWindow);

    if (_importSettings.serviceEnabled())
    {
        auto error = _importServer.start(_importSettings.port());
        if (!error.empty())
        {
            // Startup failure: no modal dialog (annoying on every launch);
            // surface it in the status bar instead. The window shows
            // "Service stopped" because isRunning() is false.
            _fileStatus->setText("Import service failed: " + error);
        }
    }
}
```

(Ordering note: `initWorkspace()` runs from `createScene()`, after `_stage`
exists — safe for `SceneImporter`'s `StageScene*`.)

### update() — per-frame pump

```cpp
void AppDelegate::update(uint32_t currentFrame,
                         bg2e::render::vulkan::FrameResources& frameResources)
{
    // Base implementation drives the renderer's per-frame update
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>
        ::update(currentFrame, frameResources);

    if (_asyncLoadsInProgress.load() == 0)
    {
        _sceneImporter->processQueue(_importServer);
    }
    // Requests arriving during an async load simply stay queued; the HTTP
    // handler blocks until they are processed or hit the 60 s timeout.
}
```

### asyncLoadGuarded() — wrap the two existing call sites

```cpp
void AppDelegate::asyncLoadGuarded(std::function<void(bg2e::ui::Loader*)> loadFn,
                                   glm::vec4 clearColor)
{
    _asyncLoadsInProgress.fetch_add(1);
    bg2e::app::MainLoop::current()->asyncLoad(
        [this, fn = std::move(loadFn)](bg2e::ui::Loader* loader) {
            fn(loader);
            _asyncLoadsInProgress.fetch_sub(1);
        }, clearColor);
}
```

Replace the direct `MainLoop::asyncLoad(...)` calls in
`AppDelegate::fileDropped()` and `ToolBar`'s *Open Scene* handler with
`_appDelegate->asyncLoadGuarded(...)`.

### drawUI() — draw the window

```cpp
void AppDelegate::drawUI()
{
    _workspace.draw();
    if (_uiSettingsWindow.isOpen())     _uiSettingsWindow.draw();
    if (_renderSettingsWindow.isOpen()) _renderSettingsWindow.draw();
    if (_importSettingsWindow.isOpen()) _importSettingsWindow.draw();   // NEW
}
```

### cleanup() — stop the server before the scene dies

```cpp
void AppDelegate::cleanup()
{
    _importServer.stop();        // joins the listen thread; fails pending requests
    _sceneImporter->clear();
    DefaultRenderLoopDelegate::cleanup();
    _stage.reset();
    _submeshPanel.cleanup();
    _sceneEditor.cleanup();
}
```

`cleanup()` is invoked from `MainLoop` before `_renderLoop.cleanup()` /
engine teardown, so the HTTP thread is fully stopped and joined while the app
objects are still valid. The window-close confirmation (`ToolBar`'s
`setOnExitFunction`) runs earlier and is unaffected.

## Frame/lifecycle summary

```
main() -> MainLoop::run()
  ├─ delegate->init(engine)                 (SelectionManager)
  ├─ RenderLoop::initScene()
  │    └─ createScene() -> StageScene::init() -> initWorkspace()
  │         └─ ImportSettings.load + SceneImporter + window init
  │            + auto-start server if enabled
  ├─ per frame: acquireAndPresent()
  │    └─ delegate->update() -> processQueue()   [skip if asyncLoad active]
  └─ exit: delegate->cleanup()
       └─ ImportServer.stop() + join
```

## Verification

- App starts with the service auto-enabled (default prefs) — `GET /status`
  responds.
- `File > Open Scene` while POSTs arrive → requests wait, then complete after
  the load finishes (no crash, no race).
- Quit while a request is in flight → clean shutdown, client gets a connection
  close or the 500 from `stop()`'s pending-request flush.
