# Settings Windows

**Headers:** `<bg2e/ui/UISettingsWindow.hpp>`, `<bg2e/ui/RenderSettingsWindow.hpp>`
**Namespace:** `bg2e::ui`

Two ready-made [`Window`](Window.md) subclasses that ship with the engine so
applications do not re-implement their own settings dialogs. Both are
self-configuring: call `init(...)`, then draw them when `isOpen()` — the usual
pattern is a toolbar menu item that calls `open()`/`close()`.

```cpp
class BG2E_API UISettingsWindow : public Window {
public:
    void init();                       // sets title/size, closes, installs body
private:
    void drawUI();
};

class BG2E_API RenderSettingsWindow : public Window {
public:
    void init(render::RendererDeferred* renderer,
              render::RenderSettingsPreferences* prefs);
private:
    bool drawUI();
    // section helpers: drawRenderScaleSection, drawIndirectLightingModeSection,
    //                  drawRTAOSection, drawRTGISection, drawRTReflectionsSection,
    //                  drawTemporalAccumulatorSection, drawDenoiseFilterSection
};
```

---

## `UISettingsWindow`

Title `"UI Settings"`, 320×350, created **closed**. Body:

| Section | Controls |
|---------|----------|
| Interface | `Interface Scale` slider `1.0 .. 2.0` → `UserInterface::setScale()` (persisted by `UserInterface::cleanup`) |
| Gizmos | one collapsing header per `manipulation::GizmoType` (Camera, Point/Spot/Directional Light, Environment): Visible checkbox, Opacity slider `0..1`, Scale slider `0.01..0.5` |
| Transform | separate header: Visible, Scale, plus `Uniform scale control` / `Axis scale controls` checkboxes. No opacity — the transform gizmo renders depth-tested and opaque, so opacity would be a no-op. |

Gizmo settings persist through the `GizmoComponent` static state, so they
apply engine-wide, not per window. No `onChanged` callback — the settings
apply immediately as they are drawn.

---

## `RenderSettingsWindow`

Title `"Render Settings"`, 350×600, created **closed**. It edits a
`render::RendererDeferred` through a `render::RenderSettingsPreferences`
object (which owns load/persist — the *application* is responsible for calling
those, e.g. on startup and from a `MainLoop::timeout` as `model_edit` does).

Sections (all changes go to `_prefs`, which applies them to the renderer):

| Section | Controls |
|---------|----------|
| Render scale | combo over `RendererDeferred::scaleOptions()` (`setRenderScaleIndex`) |
| Indirect Lighting | mode combo: `Ambient Occlusion (RTAO)` / `Global Illumination (RTGI)` |
| RTAO / RTGI | per-mode quality/bias controls |
| RT Reflections | enable + quality controls |
| Temporal Accumulator | samples / blending controls |
| Denoise Filter | filter selection + strength |

> **Dependency:** requires `render::RendererDeferred` — the deferred
> renderer. Applications using `RendererBasicForward` do not use this window
> (there is no equivalent settings widget for the forward path).

---

## Wiring them to a menu

```cpp
// init
_uiSettings.init();
_renderSettings.init(renderer(), _renderPrefs.get());

// in the Toolbar menu (see Menu.md)
MenuItem settings("Settings");
settings.addMenuItem(MenuItem("UI Settings...", ShortcutData{
    .key = KeyEvent::KeyUnknown,
    .handler = [this]{ _uiSettings.isOpen() ? _uiSettings.close() : _uiSettings.open(); } }));
_toolBar.addMenuItem(settings);

// drawUI
_workspace.draw();
if (_uiSettings.isOpen())     _uiSettings.draw();
if (_renderSettings.isOpen()) _renderSettings.draw();
```

They are floating windows (not workspace-managed), so they keep the default
draggable/resizable chrome and can be closed with their X button.

---

## See also

- [quick_start — Recipe 3](quick_start.md#recipe-3-build-an-editor-layout-with-workspace)
- `apps/model_edit/src/AppDelegate.cpp` — reference wiring (including the
  60 s preference-persist timer pattern).
- [UserInterface](UserInterface.md) — the scale mechanism.
