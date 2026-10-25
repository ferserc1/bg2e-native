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

`run(application)` initializes SDL video, creates the window and engine, installs the
application delegates, initializes the scene, and processes events and frames
until exit. Cleanup waits for the device, releases rendering and application
resources, destroys the window, and returns an exit code.

## Execution selection and validation

```cpp
int32_t run(Application* application);
int32_t run(Application* application, const draw::EngineConfig& config);
```

Both overloads select `bg2e::draw`. The one-argument overload uses
`draw::EngineConfig{}`: Metal on macOS, Vulkan elsewhere. Pass the configuration
explicitly to override the backend, validation or formats. Selecting Vulkan also
uses draw. MainLoop copies the configuration into its private execution object.
See [EngineConfig](../draw/EngineConfig.md).

Call the application's `init(argc, argv)` in the launcher before `run()`;
MainLoop does not call it automatically.

### Configuration errors

Before initializing SDL or creating a window, `run()` throws
`std::invalid_argument` for a null application, missing draw graphics delegate,
missing UI/input delegate, or Metal selected outside macOS, in that order.
Application has one graphics slot, exposed through `renderDelegate()`.
See [registration](Application_and_input.md#graphics-delegate-registration).

### Draw execution

After validation, MainLoop prepares and retains the backend before creating its
SDL window. Empty EngineConfig::applicationName uses MainLoop's appId. The draw
coordinator owns retained scene color and presentation. UI preparation happens
after presentation-image acquisition, including while the scene is paused.
Event forwarding and loader overrides require an initialized UI backend.

### Common loop and graphics execution

MainLoop owns input, UserInterface, Loader, timers, background scheduling and
joined async workers. The private DrawGraphicsExecution owns Engine and RenderLoop
and handles initialization, scene startup, resize, frame work, GPU waiting,
pause/resume and cleanup. MainLoop passes milliseconds; the adapter converts to
seconds for draw callbacks.

Safe updates wait through the graphics execution before invoking queued work;
executed callbacks invalidate retained scene, canceled callbacks do not.
Async loading pauses scene refresh while Loader/UI continues, then completes
on the main thread. Workers join before normal or exceptional teardown.
The renderer's separately typed final-color update path retains its narrow
synchronization rules; this migration adds no blanket per-frame drain.

## Plugin discovery and cleanup

Each run installs a plugin-session cleanup guard and scans
`PlatformTools::pluginPath()` before constructing the graphics execution or
acquiring its backend. Registration is graphics-free; the application's earlier
`init(argc, argv)` must not depend on automatic plugin types already being present.
A missing default directory is empty success, and individual recoverable load
failures or `NotAPlugin` candidates do not terminate the scan.

Only modules first discovered by that session belong to automatic cleanup;
explicit loads retain their explicit owner. At shutdown MainLoop blocks new queued
work, joins workers, stops plugin jobs, drains GPU users and prepares plugins, then
calls `Application::preparePluginShutdown` while graphics services still exist.
The application must remove plugin components from every active scene and release
external objects, weak control blocks and callbacks there. Delegates/UI and owned
registrations are cleaned before the engine; plugin finalization and native release
follow execution cleanup in reverse load order.

The loader trusts this programmer-owned cleanup contract; unknown surviving uses
may crash after unload. See [plugin lifecycle](../plugin/lifecycle.md),
[unload responsibilities](../plugin/unloading.md) and the
[automatic plugin tutorial](../../tutorials/plugins/plugin.md). MainLoop async
parsing holds a registry-read lease; plugin loading/unloading reports `Busy` while
those reads are active.

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

## Scene controls

```cpp
void requestSceneFrame();
void pauseScene(const glm::vec4& clearColor = {0.f, 0.f, 0.f, 1.f});
void resumeScene();
```

Call these methods on the main thread during an active run; otherwise they throw
`std::logic_error`. Each requests a presentation wakeup. In draw,
`requestSceneFrame()` marks the retained scene dirty; while paused, it stays
pending until resume. `requestFrame()` alone does not dirty the scene.
`pauseScene()` preserves the last valid scene and stores a clear color for a
replacement target (for example, after resize). UI preparation/composition keeps
running. `resumeScene()` requests a scene refresh. Do not mix manual pause ownership with an overlapping asyncLoad operation:
async completion resumes the scene unconditionally.

See the [window/UI example](../../../examples/draw/README.md).

## Safe scene updates

```cpp
void safeUpdateScene(std::function<void()> fn,
                     std::shared_ptr<SafeUpdateToken> token = nullptr);
```

The callable is queued and executed on the main thread at a safe point before
the next rendered frame. Enqueuing automatically requests a frame, so completion
work from a worker thread is not held until a low background deadline.

When a `SafeUpdateToken` is supplied, the queue keeps only a weak reference to
it. Destroying the caller's last token reference before execution cancels the
callable. Omitting the token queues unconditional work.

## Asynchronous loading

```cpp
void asyncLoad(
    std::function<void(ui::Loader*)> loadFn,
    glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f},
    std::function<void(std::exception_ptr)> onComplete = nullptr
);
```

`asyncLoad()` pauses scene drawing, runs `loadFn` on a loop-owned joinable worker thread,
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

## Implementation references

The [MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md)
walks through concrete source and ownership. The selected draw Device uses
[tracked submissions and waitIdle](../gpu/Submission_tracking_and_waitIdle.md).
Safe updates and teardown require coordination with background GPU producers:
waitIdle reopens admission on return. MainLoop joins async workers before normal
or exceptional graphics/UI cleanup and expires completion tokens when run exits.
A worker must return without depending on a future main-thread completion.
A successful example run does not imply arbitrary cross-thread scene access.

## Draw scene path

The separate [draw renderer lifecycle](../draw/RendererDeferred.md) and [live settings contract](../draw/Settings.md)
cover deferred scene ownership, public injection and queued safe changes.
The former render application execution path is removed.
