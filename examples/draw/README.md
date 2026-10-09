# Experimental draw examples

`bg2e::render` is the maintained Vulkan framework used by bg2 engine applications,
including its scene/material facilities. `bg2e::draw` is the experimental
multi-backend framework over `bg2e::gpu`: it supplies GPU-based scene delegates
and retained scene/UI composition while its high-level API can evolve. These
labels describe API maturity and scope, not Debug/Release builds or deployment
servers. Both frameworks are supported; render is not deprecated.
These examples use the same Application/MainLoop entry points with the explicit
EngineConfig overload. They do not initialize Factory or construct a private
execution wrapper.

## 01: Window and UI

The `draw_window_ui` target creates a window, clears the retained scene image
through abstract GPU commands and shows the ImGui demo through `bg2e::ui`.
It uses the existing SDL application bundle/output conventions and requires no
scene shaders. Executables are placed under `bin/<platform>`; on macOS the
executable is inside `draw_window_ui.app/Contents/MacOS/`.

With no arguments, EngineConfig selects Metal on macOS and Vulkan elsewhere.
Pass `--backend=vulkan` or `--backend=metal` to override it. Metal requires macOS.
Unknown arguments and repeated backend selection fail before window/GPU
allocation with a descriptive error. There is no interactive startup prompt.

The control window provides background color, scene pause/resume and refresh.
Scene and UI counters show that presentation/UI continue without scene redraws.
Color changes invalidate the scene and request presentation. While paused, the
last valid scene remains visible and changes/refresh requests stay pending until
resume. Resize must replace the retained image; while paused, its initial clear
uses the current pause clear color. Resuming draws the currently selected color.

`requestFrame()` requests presentation only. `requestSceneFrame()` invalidates
the scene and requests presentation. Scene controls run on the main thread;
worker changes should be queued through `safeUpdateScene()`. Async loading uses
the same pause path, displays Loader/UI and resumes through main-thread completion.

Submission is asynchronous. Acquisition waits only for reuse of an occupied
frame slot, preserving frames in flight. Global waitIdle tracks all registered
submissions across queues; callers coordinate producers for resize/teardown.
Deferred cleanup uses completion records independent of reusable command/fence
wrappers. Objects should own persistent resources indexed by frame slot rather
than allocating/destroying resources or descriptors on every frame.
