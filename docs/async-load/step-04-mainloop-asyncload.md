# Step 04 — MainLoop: asyncLoad() + completion queue

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/app/MainLoop.hpp` | modify |
| `lib/src/bg2e/app/MainLoop.cpp` | modify |

## Design notes

* `asyncLoad` launches a detached `std::thread`. The thread owns the lambda and
  posts its completion back to the main thread via a thread-safe queue.
* The main-thread queue (`_mainThreadQueue`) is the only shared state between
  the worker and the main thread. It is protected by `_mainThreadQueueMutex`.
* The queue is drained once per frame in `run()`, right after
  `executeSafeUpdateScene()`. The drain function swaps the queue out under the
  lock and executes the captured callbacks unlocked to avoid holding the mutex
  while Vulkan/ImGui calls are made.
* `ui::Loader _loader` is a member of `MainLoop` and lives for the lifetime of
  the application, so the worker thread can safely reference it through a raw
  pointer without lifetime issues.

## `MainLoop.hpp` modifications

### New includes

```cpp
#include <bg2e/ui/Loader.hpp>
#include <thread>
#include <mutex>
#include <queue>
```

### New public method

```cpp
void asyncLoad(
    std::function<void(ui::Loader*)> loadFn,
    glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f}
);
```

### New private members

```cpp
ui::Loader _loader;

std::mutex                         _mainThreadQueueMutex;
std::queue<std::function<void()>>  _mainThreadQueue;

void drainMainThreadQueue();
```

## `MainLoop.cpp` modifications

### `asyncLoad()` implementation

```cpp
void MainLoop::asyncLoad(
    std::function<void(ui::Loader*)> loadFn,
    glm::vec4 clearColor)
{
    // 1. Drain the GPU and freeze scene rendering.
    _renderLoop.pauseScene(clearColor);

    // 2. Replace the UI frame with the loading screen.
    _userInterface.setFrameOverride([this]{ _loader.draw(); });

    // 3. Run the user's load function on a worker thread.
    std::thread([this, fn = std::move(loadFn)]() mutable
    {
        fn(&_loader);

        // 4. Post completion back to the main thread.
        {
            std::lock_guard lock(_mainThreadQueueMutex);
            _mainThreadQueue.push([this]()
            {
                _userInterface.clearFrameOverride();
                _renderLoop.resumeScene();
            });
        }
    }).detach();
}
```

### `drainMainThreadQueue()` implementation

```cpp
void MainLoop::drainMainThreadQueue()
{
    if (_mainThreadQueue.empty()) return;  // fast path, no lock

    std::queue<std::function<void()>> local;
    {
        std::lock_guard lock(_mainThreadQueueMutex);
        std::swap(local, _mainThreadQueue);
    }
    while (!local.empty())
    {
        local.front()();
        local.pop();
    }
}
```

### `run()` modification

In the per-frame block, after the `executeSafeUpdateScene()` call, add:

```cpp
drainMainThreadQueue();
```

The ordering matters:

```
executeSafeUpdateScene();   // existing
drainMainThreadQueue();     // new — may call resumeScene() here
if (resizing) { ... }
if (stopRendering) { ... }
if (!resizing && _engine.newFrame()) { ... }
_userInterface.newFrame();
if (!resizing) { _renderLoop.acquireAndPresent(); }
```

`resumeScene()` calls `waitIdle()` internally, which is safe here because we
are between frames (no command buffer is recording at this point).

## Sequence diagram

```
Main thread                         Worker thread
───────────                         ─────────────
asyncLoad(fn, color)
  pauseScene(color)                 (GPU drained)
  setFrameOverride(loader.draw)
  thread.detach() ──────────────── fn(&_loader)
  return                              loader.setProgress(...)
                                      loader.setMessage(...)
                                      ... heavy work ...
[frame N] drainMainThreadQueue()       fn returns
  (queue empty, no-op)           ── post { clearFrameOverride()
[frame N+1]                              resumeScene() }
  ...
[frame M] drainMainThreadQueue()
  clearFrameOverride()            (GPU drained)
  resumeScene()
[frame M+1] normal rendering resumes
```

## Compile-time check

All new includes (`<thread>`, `<mutex>`, `<queue>`) are standard C++20.
No existing call sites change. Full build must succeed cleanly.

## Tests / manual verification

1. Call `asyncLoad` from a button click in an existing example.
2. Confirm the viewport shows the clear colour and the loader window appears.
3. Inside the lambda, call `setProgress` in a loop with `std::this_thread::sleep_for`
   between iterations; confirm the progress bar updates visually.
4. After the lambda returns, confirm normal scene rendering resumes without
   artefacts or validation errors.
5. Trigger a window resize during an async load; confirm it is handled correctly
   (the `resizing` guard in `run()` already covers this).
