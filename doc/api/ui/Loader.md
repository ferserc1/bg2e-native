# Loader

**Header:** `<bg2e/ui/Loader.hpp>`
**Namespace:** `bg2e::ui`

A modal, non-interactive progress overlay: a fixed-size (420×90) centered
window with no title bar containing the current message and a progress bar.
All state is guarded by a `std::recursive_mutex`, so a worker thread can push
updates while the UI thread draws — this is the widget that backs
`app::MainLoop::asyncLoad()`.

```cpp
class BG2E_API Loader {
public:
    void setMessage(const std::string& msg);   // thread-safe
    std::string getMessage() const;

    void setProgress(float progress);           // thread-safe, clamped [0,1]
    float getProgress() const;

    void draw();                                // main thread, once per frame
};
```

---

## Usage via `MainLoop::asyncLoad` (recommended)

`MainLoop` owns one `Loader` and wires it to a worker thread for you:

```cpp
app::MainLoop::current()->asyncLoad([](bg2e::ui::Loader* loader) {
    loader->setMessage("Importing scene...");           // safe off-thread

    for (uint32_t i = 0; i < tasks.size(); ++i) {
        process(tasks[i]);                               // long work
        loader->setProgress(float(i + 1) / tasks.size());
    }
}, glm::vec4{0.1f, 0.1f, 0.1f, 1.f});                  // clear color while paused
```

The sequence (`lib/src/bg2e/app/MainLoop.cpp`, `MainLoop::asyncLoad`):

```
asyncLoad(fn, clearColor)
  1. _renderLoop.pauseScene(clearColor)          // scene freezes, still presents
  2. _userInterface.setFrameOverride(draw loader) // normal delegate->drawUI() is bypassed
  3. std::thread: fn(&_loader)                    // detached worker
  4. worker exit -> main-thread queue:
        _userInterface.clearFrameOverride()
        _renderLoop.resumeScene()
```

Because the override swaps out the whole UI for the loader, users cannot
interact with menus/panels during the load — that is the intended modal
behavior.

### Rules for the worker callback

- **Only** `setMessage()` / `setProgress()` touch the loader; both lock the
  mutex.
- Do not call into `render::Engine`, the scene graph, or VMA from the worker.
  Cross-thread scene changes must be posted with
  `MainLoop::safeUpdateScene(fn)` (runs at a safe point between frames).
- The thread is **detached**: `fn` must complete; it cannot be cancelled, and
  exceptions escaping it terminate the process. Report failures by storing a
  result and acting on it after `asyncLoad` returns control (e.g. in a timer
  or in the next `drawUI`).
- `asyncLoad` returns immediately — do not assume the work is finished on the
  next line.

---

## Manual usage

A `Loader` can also be a standalone widget: keep one, call `loader.draw()`
from your `drawUI()` (or install it with
`UserInterface::setFrameOverride`) whenever you want the overlay, and stop
drawing it when done. Defaults: message `"Loading..."`, progress `0`.

`draw()` positions the window at the center of the ImGui display every frame,
so it follows the viewport on resize. The progress bar spans the full window
width.

---

## See also

- [quick_start — Recipe 14](quick_start.md#recipe-14-modal-loader-during-async-work)
- [UserInterface](UserInterface.md) — the frame-override mechanism.
- `doc/safe_update_scene.md` — cross-thread scene mutations.
