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

The one-argument overload always selects production `bg2e::render`, with Vulkan.
This route uses render graphics delegates and Vulkan resources.
The two-argument overload selects experimental `bg2e::draw`; it has no default
second argument. Pass `draw::EngineConfig{}` to use Metal on macOS or Vulkan on
Linux/Windows, or set `config.backend` explicitly. See
[EngineConfig](../draw/EngineConfig.md) for all configuration fields.

The run overload, not the registered delegate or low-level backend, selects
the high-level framework. Selecting Vulkan in EngineConfig still selects draw.
`MainLoop` copies the supplied configuration into its private execution object.

Call the application's `init(argc, argv)` in the launcher before `run()`.
`MainLoop` does not call it automatically.

### Configuration errors

Before initializing SDL or creating a window, `run()` throws
`std::invalid_argument` for:

- a null application pointer;
- a graphics delegate from the other framework, including two populated graphics slots;
- a missing graphics delegate for the selected framework;
- a missing UI or input delegate;
- Metal selected for draw on a platform other than macOS.

Validation follows this order: application pointer, opposite graphics slot,
required graphics slot, UI delegate, input delegate, then the experimental
platform restriction. Registering a draw delegate and calling `run(application)`
does not switch frameworks; it reports the mismatch. See
[Application registration](Application_and_input.md#graphics-delegate-registration).

### Experimental execution

Draw initializes the GPU context and coordinates retained scene color and
presentation. After validation, MainLoop prepares and retains the configured
backend before creating its SDL window. Empty EngineConfig::applicationName uses
MainLoop's appId.

The draw path initializes UserInterface with Vulkan or Metal.
UI preparation happens after presentation-image acquisition, even when the scene
is paused. Event forwarding and loader frame overrides run only when the
selected UI backend is initialized.
The render route uses its Vulkan UI callback.

### Common loop and graphics execution

`MainLoop` owns the input manager, UserInterface, Loader, timers and common
scheduling. The private graphics execution implementation owns Engine and
RenderLoop and handles initialization, scene startup, resize requests, frame
work, GPU waiting, scene pause/resume and cleanup. It is not public API.

The render implementation initializes its descriptor pool and uses its
Vulkan UI callback. During resize debounce it prepares UI frames but suppresses
scene acquisition/presentation. MainLoop passes elapsed milliseconds to the
execution implementation; the draw adapter converts to seconds for its
RenderLoop contract. Render frame callbacks use milliseconds.

Safe updates wait through the active graphics execution before invoking queued
work. Async loading pauses/resumes that execution while Loader/frame override
and worker completion remain common MainLoop behavior in both execution paths.
In draw, executed safe updates also invalidate the retained scene; canceled
callbacks do not. Async loading retains the last valid scene while Loader/UI
continues, then resumes scene refresh on the main thread. The render coordinator defines its own pause behavior.

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
running. `resumeScene()` requests a scene refresh. Production forwards pause and
resume to its existing coordinator; its scene already updates every frame.
Do not mix manual pause ownership with an overlapping asyncLoad operation:
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

## Implementation references

The [MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md)
walks through concrete source and ownership. The selected draw Device uses
[tracked submissions and waitIdle](../gpu/Submission_tracking_and_waitIdle.md).
Safe updates and teardown require coordination with background GPU producers:
waitIdle reopens admission on return. Async workers capture MainLoop and must
finish before its destruction; detached-worker lifetime is not managed by run.
A successful example run does not imply arbitrary cross-thread scene access.
