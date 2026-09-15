# User Interface Layer

The `bg2e::ui` namespace provides the engine's immediate-mode user interface
layer. It is a curated, object-oriented wrapper around **Dear ImGui** (with the
SDL2 and Vulkan backends), plus a set of higher-level editor widgets built on
top of it (material editors, scene trees, component inspectors, toolbar and
status-bar windows, and a fixed editor-workspace layout).

The module sits at Layer 8 (the top of the engine stack): it may depend on
`render`, `scene`, `manipulation`, `reflection`, `app`, `base`, `math` and
`geo`, but little inside the engine depends on `ui` in return. The two
exceptions are in `app`: `app::MainLoop`, which owns a `UserInterface`
instance and drives it once per frame, and the GPU picker
(`app::GPUSelectionDialog`, which draws `BasicWidgets`/`SelectableList`
directly over its own ImGui context).

> **Status:** `bg2e::ui` is the production UI layer used by `apps/model_edit`.
> It targets the production `bg2e::render::Engine` (Vulkan) and renders on top
> of the swapchain image through a dynamic-rendering pass. It does **not** use
> the experimental `bg2e::gpu` abstraction directly (except through
> `render::Engine` internals).

---

## Table of Contents

1. [Architecture](#architecture)
2. [Integration with the main loop](#integration-with-the-main-loop)
3. [Class hierarchy](#class-hierarchy)
4. [Class catalog](#class-catalog)
5. [Immediate-mode conventions](#immediate-mode-conventions)
6. [UI scale](#ui-scale)
7. [Where to go next](#where-to-go-next)

---

## Architecture

The layer is organized into three tiers:

1. **Infrastructure** (`UserInterface`, `UserInterfaceDelegate`) — owns the
   ImGui context, the Vulkan command buffer/pool/fence and descriptor pool,
   translates SDL events, and exposes the per-frame hooks used by `app::MainLoop`.
2. **Primitive wrappers** (`BasicWidgets`, `Input`, `Menu`, `SelectableList`,
   `TextureWidgets`, `Window`, `Workspace`, `Toolbar`, `StatusBar`, `Loader`) —
   thin, allocation-free static or window classes that cover the common ImGui
   patterns used by the engine's applications.
3. **Composite editors** (`MaterialEditor`, `DrawableEditor`, `SubmeshSelector`,
   `NodeEditor`, `SceneTree`, `LightEditor`, `CameraSettings`,
   `PolarTransformControllerEditor`, `ReflectionWidget`, `ComponentInspector`,
   `UISettingsWindow`, `RenderSettingsWindow`) — scene-aware widgets that bind
   engine data (`scene::Node`, `render::MaterialBase`, `base::Light`,
   `manipulation::SelectionManager`, `reflection::TypeInfo`) to UI controls.

All widgets follow the immediate-mode contract: they must be called **once per
frame** between `UserInterface::newFrame()` and `UserInterface::draw()`. State
that must survive across frames (selected indexes, euler caches, textures)
lives inside the widget objects, never in ImGui.

### Widget tree of a typical editor application

```
app::MainLoop
  +-- ui::UserInterface                    (owns ImGui context + Vulkan resources)
        +-- ui::UserInterfaceDelegate      (application drawUI() hook, via Application)
              +-- ui::Workspace            (fixed layout: toolbar + 3 panels + status bar)
              |     +-- ui::Toolbar        (Window subclass, menu bar + buttons)
              |     +-- ui::Window         (left panel, e.g. DrawableEditor host)
              |     +-- ui::Window         (right panel, e.g. environment host)
              |     +-- ui::Window         (bottom panel, optional)
              |     +-- ui::StatusBar      (Window subclass, status items)
              +-- ui::UISettingsWindow     (floating window, optional)
              +-- ui::RenderSettingsWindow (floating window, optional)
```

---

## Integration with the main loop

`app::MainLoop` owns a `ui::UserInterface` member and drives its whole
lifecycle; applications only implement the delegate:

```
MainLoop::run(application)
    -> delegate->_viewportWidth/_viewportHeight = SDL window size
    -> _userInterface.setDelegate(application->uiDelegate())
    -> _userInterface.init(&_engine)               // also calls delegate->init()
    -> _renderLoop.renderUICallback(...)           // records _userInterface.draw() in the frame
    |
    |  per frame:
    |     SDL_PollEvent -> _inputManager (InputDelegate) -> _userInterface.processEvent()
    |     _userInterface.newFrame()                // ImGui::NewFrame + delegate->drawUI() + Render
    |     _renderLoop.acquireAndPresent()          // scene render + _userInterface.draw(cmd, imageview)
    |
    -> _userInterface.cleanup()                    // persists "uiScale" preference
```

The application plugs in through `app::Application::setUiDelegate()`. A single
class typically implements all three delegates (`RenderLoopDelegate`,
`InputDelegate`, `UserInterfaceDelegate`):

```cpp
class MyDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
                   public bg2e::app::InputDelegate,
                   public bg2e::ui::UserInterfaceDelegate
{
    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override
    {
        // Configure windows once the engine exists.
        // uiWidth()/uiHeight() hold the current viewport size.
    }

    void drawUI() override
    {
        _workspace.draw();
    }

protected:
    bg2e::ui::Workspace _workspace;
};
```

See `examples/02_ui/src/main.cpp` for a minimal delegate and
`apps/model_edit/src/AppDelegate.cpp` for a complete workspace-based one.

### Event routing order

For every SDL event, `MainLoop` first feeds the `InputDelegate` (scene input)
and then `UserInterface::processEvent()` (ImGui). ImGui consumes what it needs
for its own widgets, but the input delegate is **not** automatically filtered:
if a widget has the mouse/keyboard, guard scene interaction in your input
callbacks (for example, do not rotate the camera while the user drags a
slider).

### Frame override

`UserInterface::setFrameOverride(fn)` replaces `delegate->drawUI()` for as long
as it is set. `app::MainLoop::asyncLoad()` uses it to render a modal
[`Loader`](Loader.md) window while a worker thread loads a document:

```cpp
MainLoop::current()->asyncLoad([](ui::Loader* loader) {
    loader->setMessage("Loading model...");        // thread-safe
    loader->setProgress(0.5f);                     // thread-safe
    // ... long-running load (no engine calls off-thread!) ...
});
// When the load finishes, MainLoop restores the normal drawUI() path.
```

---

## Class hierarchy

```
ui::UserInterface                 (concrete, owned by app::MainLoop)
ui::UserInterfaceDelegate         (abstract, implemented by the application)
ui::Window                        (concrete base for all floating/docked windows)
  +-- ui::Toolbar                 (fixed top bar: menu + buttons)
  +-- ui::StatusBar               (fixed bottom bar: status items)
  +-- ui::UISettingsWindow        (built-in settings window)
  +-- ui::RenderSettingsWindow    (built-in deferred-render settings)
ui::Workspace                     (concrete, not polymorphic)
ui::Menu / ui::MenuItem           (concrete, not polymorphic)
ui::BasicWidgets                  (static-only helper class)
ui::Input                         (static-only helper class)
ui::SelectableList                (static-only helper class)
ui::TextureWidgets                (concrete, per-texture slot widget)
ui::MaterialEditor                (concrete editor)
ui::DrawableEditor                (concrete editor, owns a SubmeshSelector)
ui::SubmeshSelector               (concrete editor)
ui::LightEditor                   (concrete editor)
ui::CameraSettings                (concrete editor)
ui::PolarTransformControllerEditor(concrete editor)
ui::NodeEditor                    (concrete editor, composes the editors above)
ui::SceneTree                     (concrete widget)
ui::ReflectionWidget              (static-only generic metadata drawer)
ui::ComponentInspector            (concrete widget)
ui::Loader                        (concrete, thread-safe progress widget)
ui::StatusItem                    (concrete value widget held by StatusBar)
ui::DemoWindow                    (static-only, wraps ImGui::ShowDemoWindow)
```

---

## Class catalog

### Core infrastructure

| Class | Header | Description |
|-------|--------|-------------|
| **[`UserInterface`](UserInterface.md)** | `UserInterface.hpp` | Owns the ImGui context, SDL2/Vulkan backends, command buffer, fence and descriptor pool. Drives `newFrame()` / `draw()` / `processEvent()` and the global UI scale. |
| **[`UserInterfaceDelegate`](UserInterface.md#userinterfacedelegate)** | `UserInterfaceDelegate.hpp` | Application hook: `init(engine, ui)` and `drawUI()`. Exposes `uiWidth()` / `uiHeight()` (current viewport size). |
| **`DemoWindow`** | `DemoWindow.hpp` | Single static `draw()` that forwards to `ImGui::ShowDemoWindow()`; the default delegate body. |
| **[`Loader`](Loader.md)** | `Loader.hpp` | Thread-safe modal progress window (message + progress bar), used by `MainLoop::asyncLoad()`. |

### Windows and layout

| Class | Header | Description |
|-------|--------|-------------|
| **[`Window`](Window.md)** | `Window.hpp` | ImGui window wrapper: title, `Options` flags, position/size, open/close, and two usage modes (draw-lambda parameter or preconfigured draw function). |
| **[`Workspace`](Workspace.md)** | `Workspace.hpp` | Fixed editor layout: toolbar on top, left/right/bottom panels, status bar on the bottom. Manages panel sizes and re-locks window options. |
| **[`Toolbar`](Toolbar_and_StatusBar.md)** | `Toolbar.hpp` | `Window` subclass with a left/right aligned button list, id-based lookup/enable/label update, and an attached [`Menu`](Menu.md). |
| **[`StatusBar` / `StatusItem`](Toolbar_and_StatusBar.md)** | `StatusBar.hpp` | Left/right aligned text items updated live through `shared_ptr<StatusItem>`. |
| **[`Menu` / `MenuItem`](Menu.md)** | `Menu.hpp` | Static menu-bar primitives plus a data-driven item tree with automatic keyboard-shortcut registration (`app::Shortcuts`). |

### Primitive widgets

| Class | Header | Description |
|-------|--------|-------------|
| **[`BasicWidgets`](BasicWidgets.md)** | `BasicWidgets.hpp` | Text, buttons, checkboxes, radio buttons, trees, collapsing headers, tooltips, disabled groups, ID stack, size queries, child regions, `sameLine` with negative = right-aligned positioning. |
| **[`Input`](Input.md)** | `Input.hpp` | Value editors: text, numbers, vec2/3/4, colors, sliders, drags, combos, and a decomposed `mat4` (position/rotation/scale) editor with an internal euler cache. |
| **[`SelectableList`](BasicWidgets.md#selectablelist)** | `SelectableList.hpp` | Multi-column table of selectable rows with optional per-row buttons. |
| **[`TextureWidgets`](TextureWidgets.md)** | `TextureWidgets.hpp` | Renders a `render::Texture` as an ImGui image/image-button; adds/removes ImGui Vulkan descriptor sets, with deferred texture swapping. |

### Scene editors

| Class | Header | Description |
|-------|--------|-------------|
| **[`SceneTree`](SceneEditors.md#scenetree)** | `SceneTree.hpp` | Node-hierarchy tree; selection delegated to a `manipulation::SelectionManager` (read-only without one). |
| **[`NodeEditor`](SceneEditors.md#nodeeditor)** | `NodeEditor.hpp` | Per-node inspector: name, enabled flag, component list, transform (P/R/S), drawable, environment, light and camera sections. |
| **[`MaterialEditor`](Material_Editors.md#materialeditor)** | `MaterialEditor.hpp` | Full PBR material editor (albedo/normal/metallic/roughness/AO/emissive/sheen/fresnel) with multi-material editing and selection-driven mode. |
| **[`DrawableEditor` / `SubmeshSelector`](Material_Editors.md)** | `DrawableEditor.hpp`, `SubmeshSelector.hpp` | Submesh list + name/group/visibility editing for the selected drawable. |
| **[`LightEditor`](SceneEditors.md#lighteditor)** | `LightEditor.hpp` | Edits a `scene::LightComponent`'s `base::Light` (type, color, intensity, shadows, spot cone). |
| **[`CameraSettings`](SceneEditors.md#camerasettings)** | `CameraSettings.hpp` | Focal length + sensor size presets for a `scene::CameraComponent` with `math::OpticalProjection`. |
| **[`PolarTransformControllerEditor`](SceneEditors.md#polartransformcontrollereditor)** | `PolarTransformControllerEditor.hpp` | Azimuth/elevation/distance/euler/target sliders for `scene::PolarTransformControllerComponent`. |

### Reflection-driven widgets

| Class | Header | Description |
|-------|--------|-------------|
| **[`ReflectionWidget`](Reflection_Widgets.md)** | `ReflectionWidget.hpp` | Generic ImGui form generated from `bg2e::reflection` metadata (`drawProperties` / `drawActions`). |
| **[`ComponentInspector`](Reflection_Widgets.md)** | `ComponentInspector.hpp` | Lists a node's components as collapsing headers, each expanded through `ReflectionWidget`, with remove buttons. |

### Settings windows

| Class | Header | Description |
|-------|--------|-------------|
| **[`UISettingsWindow`](Settings_Windows.md)** | `UISettingsWindow.hpp` | Interface-scale slider + per-gizmo visibility/opacity/scale controls. |
| **[`RenderSettingsWindow`](Settings_Windows.md)** | `RenderSettingsWindow.hpp` | Render-scale, indirect-lighting mode, RT reflections, temporal accumulator and denoiser settings for `RendererDeferred`. |

---

## Immediate-mode conventions

These rules apply to every class in the module and are the main source of
surprises when coming from retained-mode UIs:

1. **Widgets are code, not objects.** A "label" or "button" only exists while
   the frame is being drawn. Persistent state (which row is selected, the last
   typed text) must live in your own variables — the engine widgets take values
   by pointer/reference and modify them in place.
2. **Every widget draws itself every frame.** `draw()` methods both render and
   read input; they return `true` on the frame in which the value changed.
   Editor widgets (`MaterialEditor`, `LightEditor`, ...) additionally invoke the
   callback registered through `onChanged(...)`.
3. **Unique IDs.** ImGui identifies widgets by label. Use the `##suffix`
   convention to keep visible labels short while IDs stay unique (see
   [BasicWidgets](BasicWidgets.md#identifier-management)), or wrap repeated
   items with `pushId()`/`popId()`.
4. **Do not block inside `drawUI()`.** Long work belongs in
   `MainLoop::asyncLoad()`; scene-graph mutations issued from a UI callback
   while it runs should go through `app::MainLoop::safeUpdateScene()` so they
   execute between frames instead of mid-traversal (see
   `doc/safe_update_scene.md`).

---

## UI scale

The whole layer is DPI-scale aware:

- `UserInterface::getScale()` / `setScale()` manage a static scale factor
  (`float`, default `1.0`). The style and the global font scale are rebuilt on
  the next `newFrame()`.
- The value is persisted automatically: `init()` reads and `cleanup()` writes
  the `uiScale` key of the `"ui"` preferences context.
- `UISettingsWindow` exposes it as a 1.0–2.0 slider.
- `Workspace` listens to scale changes and recomputes all panel geometry,
  multiplying logical sizes by the scale.

---

## Where to go next

- **[quick_start.md](quick_start.md)** — recipe-oriented practical guide with
  focus on the less trivial widgets (Workspace, Toolbar, Menu, TextureWidgets,
  MaterialEditor, SceneTree, reflection widgets, Loader).
- **[reference.md](reference.md)** — full class, struct and enum reference.
- Per-class docs: [UserInterface](UserInterface.md), [Window](Window.md),
  [Workspace](Workspace.md), [BasicWidgets & SelectableList](BasicWidgets.md),
  [Input](Input.md), [Menu](Menu.md), [Toolbar & StatusBar](Toolbar_and_StatusBar.md),
  [TextureWidgets](TextureWidgets.md),
  [Material & Drawable editors](Material_Editors.md),
  [Scene editors](SceneEditors.md), [Reflection widgets](Reflection_Widgets.md),
  [Settings windows](Settings_Windows.md), [Loader](Loader.md).
- Related global docs: `doc/delegate_system.md` (delegate architecture),
  `doc/workspace_api.md`, `doc/window_api.md`, `doc/input_delegate.md`.
- The reflection metadata that drives
  [ReflectionWidget](Reflection_Widgets.md) is documented in
  [doc/api/reflection](../reflection/index.md).
