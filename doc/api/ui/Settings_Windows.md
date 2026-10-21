# Settings Windows

Use UISettingsWindow for interface preferences and
[GraphicsSettingsWindow](GraphicsSettingsWindow.md) for live draw renderer settings.
The old RenderSettingsWindow class/header is removed.

## `UISettingsWindow`

Title `"UI Settings"`, created **closed**. The window is 320×350 normally and
320×430 when `showBackgroundFrameRateSettings` is true. Body:

| Section | Controls |
|---------|----------|
| Interface | `Interface Scale` slider `1.0 .. 2.0` → `UserInterface::setScale()` (persisted by `UserInterface::cleanup`) |
| Background Performance (optional) | Enable limiting while unfocused and edit maximum background FPS (`0.1 .. 240`). Changes apply immediately through `app::MainLoop::current()` and persist in the `"app"` preferences context. |
| Gizmos | one collapsing header per `manipulation::GizmoType` (Camera, Point/Spot/Directional Light, Environment): Visible checkbox, Opacity slider `0..1`, Scale slider `0.01..0.5` |
| Transform | separate header: Visible, Scale, plus `Uniform scale control` / `Axis scale controls` checkboxes. No opacity — the transform gizmo renders depth-tested and opaque, so opacity would be a no-op. |

Gizmo settings persist through the `GizmoComponent` static state, so they
apply engine-wide, not per window. No `onChanged` callback — the settings
apply immediately as they are drawn.

Pass `true` only when the host application wants to expose the application
scheduler controls:

```cpp
_uiSettings.init(true);
```

The optional controls configure `MainLoop`; they do not implement throttling
inside the UI layer. When disabled, existing callers can continue using
`init()` with no behavior or size change.

---

## Draw graphics settings

`GraphicsSettingsWindow` binds a shared draw::RenderSettingsPreferences facade
and queues edits/imports at the renderer safe boundary. It reads live reflected
metadata and values, refreshes on generations and reports availability/errors.
See [settings contract](../draw/Settings.md). Production RenderSettingsWindow
and UISettingsWindow keep the contracts above.

For the current draw window lifecycle, target IDs, availability and code wiring, see [GraphicsSettingsWindow](GraphicsSettingsWindow.md) and [live settings](../draw/Settings.md). It must be explicitly connected, bound and drawn by the application.
