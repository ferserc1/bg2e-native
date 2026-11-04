# Settings Windows

Use UISettingsWindow for interface preferences and
[GraphicsSettingsWindow](GraphicsSettingsWindow.md) for live draw renderer settings.
The old RenderSettingsWindow class/header is removed.

## `UISettingsWindow`

Title `"UI Settings"`, created **closed**. The initial window is 400×450 normally
and 400×550 when either optional performance section is enabled. Normal Window
scrolling/resizing remains available. Body:

| Section | Controls |
|---------|----------|
| Interface | `Interface Scale` slider `1.0 .. 2.0` → `UserInterface::setScale()` (persisted by `UserInterface::cleanup`) |
| Toolbar | Presentation: Text / Icons with tooltips / Icons and text; logical Icon Size slider `12 .. 32`. Uses the active session defaults. |
| Scene Performance (optional) | Foreground/background scene FPS, with zero meaning unlimited; enabled by the second init argument. Takes priority over Background Performance. |
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
inside the UI layer. Existing callers can continue using `init()`; it now includes toolbar controls
and the larger initial size. Use `init(false, true)` to show scene FPS controls.

---

## Toolbar preferences

UI Settings edits the [session-owned toolbar configuration](UserInterface.md#icons-and-toolbar-defaults),
not individual Toolbar instances. Toolbars inherit it on each draw by default;
[local whole-configuration overrides](Toolbar_and_StatusBar.md#runtime-api)
remain effective until useSessionDefaults() is called.

| Key in `ui` preferences | Stored value | Default / invalid-value fallback |
| --- | --- | --- |
| `toolbarPresentation` | Integer 0 = Text, 1 = IconsWithTooltips, 2 = IconsAndText | 2 (IconsAndText) |
| `toolbarIconSize` | Finite positive logical size | 16 |

Both values load before delegate initialization. Changing the settings calls
UserInterface::setToolbarConfiguration, applies immediately to subsequent draws
and saves the changed defaults. Unchanged drawing does not write preferences.
Per-toolbar overrides are not saved as global defaults. Missing/malformed values
fall back independently; invalid stored data does not prevent UI startup.

The slider edits sizes from 12 to 32 and clamps temporary text input to that
range. The programmatic API still accepts any finite positive size, and valid
stored sizes outside the slider range reload unchanged. Changing presentation
alone does not clamp such a size. Icon size precedes interface scaling; image
source pixels are independent. Large icons require enough height in the embedding
toolbar window. Buttons without paths stay textual in all presentation modes.
Menu icon size follows its text line height and is unaffected by these defaults.
Changing presentation/size preserves icon cache entries and button identity.

---

## Draw graphics settings

`GraphicsSettingsWindow` binds a shared draw::RenderSettingsPreferences facade
and queues edits/imports at the renderer safe boundary. It reads live reflected
metadata and values, refreshes on generations and reports availability/errors.
See [settings contract](../draw/Settings.md). Production RenderSettingsWindow
and UISettingsWindow keep the contracts above.

For the current draw window lifecycle, target IDs, availability and code wiring, see [GraphicsSettingsWindow](GraphicsSettingsWindow.md) and [live settings](../draw/Settings.md). It must be explicitly connected, bound and drawn by the application.
