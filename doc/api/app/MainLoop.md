# MainLoop

**Header:** `<bg2e/app/MainLoop.hpp>`  
**Namespace:** `bg2e::app`

`MainLoop` owns the lifetime of an interactive application. Only one instance
may exist at a time; constructing a second instance throws. The active instance
is available through `MainLoop::current()`.

## Basic lifecycle

```cpp
MainLoop loop("org.example.editor");
loop.initWindowConfig(WindowConfig::maximized("Example", true));
return loop.run(application);
```

`run()` initializes SDL video, creates the window and engine, installs the
application delegates, initializes the scene, and processes events and frames
until exit. Cleanup waits for the device, releases rendering and application
resources, destroys the window, and returns an exit code.

## WindowConfig

`WindowConfig` describes initial position, size, state, decoration, resizing,
always-on-top behavior, and optional size persistence. Factory functions cover
the common forms:

```cpp
WindowConfig::withSize("Editor", 1280, 720, true);
WindowConfig::withPositionAndSize("Editor", 50, 50, 1280, 720, true);
WindowConfig::maximized("Editor", true);
WindowConfig::fullscreen("Viewer");
```

When `persistentSize` is true, the last window size is stored in the `"app"`
preferences context.

## Background frame-rate limiting

```cpp
void setBackgroundFrameRateLimitEnabled(bool enabled);
bool backgroundFrameRateLimitEnabled() const;

void setBackgroundMaxFrameRate(double fps);
double backgroundMaxFrameRate() const;

void requestFrame();
```

The limiter applies only while the SDL window lacks input focus. Foreground
rendering is always unrestricted. The default configuration is disabled with a
stored limit of `1.0` FPS.

`setBackgroundMaxFrameRate()` accepts positive finite values, including
fractional rates. It throws `std::invalid_argument` for zero, negative, NaN, or
infinite values. Changing either setting requests a prompt frame so the new
configuration takes effect without waiting for an old deadline.

While a deadline is pending, the loop:

1. polls and routes SDL events;
2. executes safe scene updates;
3. drains main-thread work;
4. executes due timers;
5. skips frame update, rendering, and presentation;
6. sleeps for at most 50 milliseconds before checking again.

This reduces GPU work to the configured background rate and avoids a busy CPU
loop. Focus changes are observed within the bounded polling interval. Gaining
focus disables throttling immediately and resets frame timing so the first
foreground update does not receive the entire background interval as delta.

### Requesting an exceptional frame

`requestFrame()` is thread-safe. It causes the next background polling
iteration to render once even when the normal deadline has not arrived. Use it
when asynchronous work is dispatched from a render/update callback and must
start promptly:

```cpp
if (auto* loop = MainLoop::current()) {
    loop->requestFrame();
}
```

The call does not change the configured limit and has no meaningful cost while
the application is in the foreground.

## Safe scene updates

```cpp
void safeUpdateScene(std::function<void()> fn,
                     std::shared_ptr<SafeUpdateToken> token = nullptr);
```

The callable is queued and executed on the main thread at a safe point before
the next rendered frame. Enqueuing automatically requests a frame, so completion
work from a worker thread is not held until a low background deadline.

## Asynchronous loading

```cpp
void asyncLoad(
    std::function<void(ui::Loader*)> loadFn,
    glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f},
    std::function<void(std::exception_ptr)> onComplete = nullptr
);
```

`asyncLoad()` pauses scene drawing, runs `loadFn` on a detached worker thread,
then queues restoration and `onComplete` on the main thread. The worker must not
mutate main-thread-only engine state directly. Progress methods on the supplied
loader are thread-safe.

## Timers and exit

- `timeout()` exposes the loop-owned `base::Timeout` scheduler.
- `setOnExitFunction(fn)` installs a close-request callback; return `true` to
  exit or `false` to cancel.
- `exit()` posts an SDL quit event and is safe to use from application actions.
- `requestResizeEvent()` requests swapchain resize processing and a prompt frame.
- `shortcuts()` returns the current loop's `Shortcuts` registry.
