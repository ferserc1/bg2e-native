# RenderLoop

**Header:** `<bg2e/draw/RenderLoop.hpp>`  
**Namespace:** `bg2e::draw`

Coordinates scene rendering and presentation on the graphics queue. Engine owns
the GPU context; RenderLoop owns the persistent scene color and the command/frame
wrappers retained for each frame in flight.

## Lifecycle

Register a RenderLoopDelegate, call `init(Engine*)`, then `initScene()` before
`frame(float deltaSeconds)`. MainLoop performs these operations through its draw
execution adapter and converts milliseconds to seconds. The existing
`frame(float, ui::UserInterface&)` overload forwards to the same frame path; it
does not initialize or call UI functions.

The first available presentation frame creates a scene color image matching the
actual drawable extent and format. The delegate receives `resize()` in drawable
pixels, then `update()` and `render()` when the scene is dirty and unpaused.
FrameContext contains the retained color image, synchronized resource slot,
surface frame number and elapsed time. There is no depth target in the
scene contract. The coordinator clears the scene color before rendering; the
delegate opens and closes any scopes it needs. A scope left open is a
configuration error reported by exception.

Every presentation copies the retained scene color into the acquired drawable.
The loop keeps scene work and copies on one graphics queue, with GPU resource
dependencies between reads and writes. Slot reuse waits through Surface::beginFrame;
there is no full-device or per-submit CPU wait during ordinary frame execution
with unchanged targets/UI metadata. Vulkan may also wait prior use of an acquired
swapchain image.
Unavailable/zero-sized drawables skip work without advancing application slots.
Resize and shutdown drain all users before destroying shared targets.

## Invalidation and pause

- `requestSceneFrame()` marks the scene dirty. It does not wake MainLoop itself.
  Application controls should use `MainLoop::requestSceneFrame()`, which also
  requests presentation. `MainLoop::pauseScene()` and `resumeScene()` forward
  scene controls and request a wakeup; all three are main-thread-only.
- `requestResize()` records target invalidation for the next available frame.
- `setSceneClearColor(glm::vec4)` changes the background and marks the scene dirty.
- `pauseScene(clearColor)` stores that background and pauses scene work while
  preserving the last valid scene image. An image without valid contents is
  cleared even when paused, including after resize.
- `resumeScene()` resumes scene work and marks it dirty.

Scene rendering starts dirty. Requests made during delegate callbacks remain
pending for a subsequent frame. Presentation continues while the scene is clean
or paused.

## Optional UI composition

```cpp
using UICompositionCallback =
    std::function<void(gpu::CommandBuffer&, gpu::SurfaceFrame&)>;
void setUICompositionCallback(UICompositionCallback callback);
```

The callback runs after the scene copy, with presentation color in
ColorAttachment layout and no active scene scope. It must close all scopes it
opens. The callback is empty by default. MainLoop binds the selected Vulkan or
Metal UI backend. `setUIFramePreparationCallback` installs a callback with the same
signature, invoked after acquisition/slot synchronization and before scene work.
It must leave all command scopes closed.

## Cleanup

Call `cleanup()` while Engine is alive and all producers are stopped. It waits
for GPU completion, cleans the initialized delegate once, releases retained
commands/frames and scene color, then resets coordination state. The registered
delegate remains available for another initialization. Repeated cleanup is safe;
the destructor also attempts cleanup without propagating exceptions.

See [FrameContext](FrameContext.md), [RenderLoopDelegate](RenderLoopDelegate.md)
and [Engine](Engine.md).

## Synchronization reference

Surface::beginFrame waits only for occupied slot reuse; Queue::submit is
asynchronous and Surface::endFrame advances submitted frames without waiting.
Target recreation/cleanup and renderer reinitialization intentionally use global
waits. See [submission tracking](../gpu/Submission_tracking_and_waitIdle.md) and
[source-backed frame flow](../../architecture/MainLoop_render_draw_gpu.md).
Independent scene/UI passes currently share the same main-thread loop and GPU
command buffer; they do not establish a separate UI thread.
