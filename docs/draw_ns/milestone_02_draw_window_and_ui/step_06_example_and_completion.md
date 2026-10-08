# Step 06: Add the draw example and finish integration

## Example files
Create examples/draw/01_window_ui/src/main.cpp and its CMakeLists.txt. Register the example in examples/CMakeLists.txt using the repository's existing platform/bundle/output conventions. Keep all current examples unchanged. Build integration edits are limited to this example and the Metal backend dependency selection described in the summary; obtain required authorization if not already granted.

Add examples/draw/README.md explaining production render versus experimental draw, selection arguments and the retained scene/UI behavior. No shader compilation is required for a clear-only scene.

## Application code
Use app::Application and app::MainLoop. Call Application::init with argc/argv as existing launchers do. Register a draw::RenderLoopDelegate, an InputDelegate and a UserInterfaceDelegate. Select Vulkan by default and accept --backend=vulkan or --backend=metal. Reject unknown values and Metal on non-macOS with actionable exceptions. Avoid interactive console selection in a GUI application's startup.

Call run(&application, engineConfig). The application does not construct a GraphicsExecution or initialize Factory. The draw delegate clears the retained scene image through gpu command interfaces; it contains no Vulkan/Metal includes or production scene resources.

The UI delegate draws ImGui's demo and a small example control window. Include a background-color control, a pause/resume scene control and a scene-refresh action. Color edits invalidate the scene; while paused, the retained color remains unchanged and UI remains interactive. A visible UI counter can demonstrate UI updates independently of scene redraws. Use callbacks into draw::RenderLoop or a small application-owned controller; do not expose internal GraphicsExecution.

## MainLoop completion
Connect the new UserInterface preparation/composition to DrawGraphicsExecution and the draw coordinator. processEvent and frameOverride use the initialized selected UI path. Pausing scene work through asyncLoad preserves the retained scene image and still draws Loader/UI; completion resumes scene refresh on the main thread. Do not change legacy asyncLoad pause semantics.

Retain common event timing, resize debounce, window preferences, focus handling and frame limits. Ensure requestFrame can request presentation without necessarily dirtying the scene; scene invalidation requests both scene refresh and a presentation wakeup. Read updated backend metadata after surface recreation.

## Completion and documentation
Remove remaining temporary not-implemented boundaries for the supported window path; retain explicit exceptions for unsupported features. Ensure experimental delegate mismatch errors are still evaluated before allocation. Document serialized GPU completion as the initial policy and per-object resource rings as the intended future allocation model.

Record completion against these behaviors for project-lead runtime acceptance: production launchers unchanged; draw Vulkan/Metal clear and demo; pause retains scene while UI updates; positive-size resize recreates targets; minimize/restore handles unavailable frames; shutdown waits and releases selected resources. Do not create tests, add test targets or invoke builds.
