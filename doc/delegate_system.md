# Application delegates

A windowed Application registers separate draw graphics, input and UI delegates.
MainLoop validates them, initializes Engine/UserInterface, schedules events and
scene frames, and tears down joined work and owners before the native context.
Both run overloads use draw. EngineConfig chooses Metal on macOS and Vulkan
elsewhere by default; assign its backend explicitly to override that choice.

## Graphics callbacks

`draw::RenderLoopDelegate` is the custom command-recording interface. FrameContext
borrows the command buffer, output surface and slot metadata. Close rendering and
compute scopes before returning; retain resource owners through GPU completion.
Use the non-template `draw::DefaultRenderLoopDelegate` for a Renderer, normally
RendererDeferred. It exclusively owns the renderer, handles stopped-boundary
configuration and history invalidation, and avoids duplicate scene traversal.

## Input and UI

InputDelegate receives application input. Route gestures between selection/gizmos
and camera controls as shown in the manipulation example. UserInterfaceDelegate
receives `draw::Engine*` and UserInterface; create widgets through bg2e::ui wrappers.
Do not include ImGui headers in application-facing engine API. Clear previews and
GPU-backed widgets before destroying UI/context/device owners.

## Headless jobs

OffscreenApplication accepts DrawOffscreenApplicationDelegate and its typed
configuration. It creates a windowless Engine and no input/UI runtime. The
DefaultOffscreenApplicationDelegate freezes camera/scene/settings, renders the
exact requested completed camera samples and publishes linear accumulation.
Standalone UV baking instead uses the bake namespace and its own submission loop.

Start with [window/UI](tutorials/draw/01_window_ui.md), then
[deferred scenes](tutorials/draw/10_deferred_renderer.md),
[manipulation](tutorials/draw/11_manipulation.md) or
[offscreen rendering](tutorials/draw/13_offscreen_application.md).
See [MainLoop](api/app/MainLoop.md), [Application/input](api/app/Application_and_input.md),
[graphics delegate](api/draw/RenderLoopDelegate.md),
[UserInterface](api/ui/UserInterface.md) and [frame ownership](api/draw/FrameContext.md).
