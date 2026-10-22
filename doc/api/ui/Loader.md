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
app::MainLoop::current()->asyncLoad([tasks](bg2e::ui::Loader* loader) {
    loader->setMessage("Importing scene...");           // safe off-thread

    for (uint32_t i = 0; i < tasks.size(); ++i) {
        process(tasks[i]);                               // long work
        loader->setProgress(float(i + 1) / tasks.size());
    }
}, glm::vec4{0.1f, 0.1f, 0.1f, 1.f});                  // clear color while paused
```

This fragment assumes caller-provided CPU-only tasks/process and a live MainLoop;
it is not a standalone source file. Captured task data lives through the worker.
The sequence in lib/src/bg2e/app/MainLoop.cpp is:

```text
asyncLoad(fn, clearColor, onComplete)
  1. Pause scene through the active graphics execution adapter.
  2. Install Loader frame override when UI is initialized.
  3. Start a loop-owned std::jthread; invoke fn and capture exceptions.
  4. Queue a SafeUpdateToken-guarded main-thread completion.
  5. If current: clear override, resume scene, invoke onComplete(error).
  6. On exit: join workers, expire tokens, then release UI/graphics resources.
```

Because the override swaps out the whole UI for the loader, users cannot
interact with menus/panels during the load — that is the intended modal
behavior.

### Rules for the worker callback

- **Only** `setMessage()` / `setProgress()` touch the loader; both lock the
  mutex.
- Do not call into `draw::Engine`, the scene graph, or VMA from the worker.
  Cross-thread scene changes must be posted with
  `MainLoop::safeUpdateScene(fn)` (runs at a safe point between frames).
- Workers are joined, not detached. The callable must finish without waiting
  for main-thread completion; it has no automatic cancellation parameter. Use
  application-owned cooperative cancellation for long CPU work. Native scene/GPU
  calls belong on the stopped main thread.
- Worker exceptions do not escape the thread: supply the optional onComplete
  callback to receive std::exception_ptr and report errors. Completion is suppressed
  for an exiting run. See [MainLoop](../app/MainLoop.md#asynchronous-loading).
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
- [Safe scene updates](../../safe_update_scene.md) — cross-thread scene mutations.
