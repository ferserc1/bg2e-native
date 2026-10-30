# UI API

The production UI layer wraps ImGui without exposing its types in engine public
headers. UserInterface owns the context/platform session and a private GPU adapter;
Vulkan and Metal native rendering live in GPU. MainLoop drives the session.

## Lifecycle

Register UserInterfaceDelegate through Application. Its init(draw::Engine*,
UserInterface*) runs after session initialization; drawUI builds widgets. MainLoop
routes events and passes gpu::CommandBuffer/SurfaceFrame to newFrame and draw.
Application widget code uses bg2e::ui wrappers. Preview snapshots retain image
owners and become invalid on cleanup. Release widgets/previews before UI and
Engine/Device teardown; queue resource edits at the stopped application boundary.
GPU remains independent of UI and scene namespaces.

## Guides

- [UserInterface](UserInterface.md), [quick start](quick_start.md), [reference](reference.md)
- [Workspace](Workspace.md), [Window](Window.md), [menus](Menu.md)
- [Toolbar/status](Toolbar_and_StatusBar.md), [Loader](Loader.md)
- [DrawMaterialEditor and drawable editing](Material_Editors.md)
- [Optional edit sessions and typed intents](EditInteraction.md)
- [DrawTextureWidgets](DrawTextureWidgets.md), [TexturePreview](TexturePreview.md)
- [Reflection](../reflection/index.md) and [live graphics settings](../draw/Settings.md)
- [MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md)
- [Model editor workflow](../../tutorials/draw/model_edit.md)

Use GraphicsSettingsWindow for live renderer settings and application environment
editors for scene lights/environment. Performance controls belong in the
application's Performance view rather than serialized graphics settings.

These interfaces do not require command history. See the
[command/memento SDK integration guide](../../architecture/command-memento/sdk-integration.md)
for opt-in use and migration from mutation notifications.
