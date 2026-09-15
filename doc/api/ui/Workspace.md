# Workspace

**Header:** `<bg2e/ui/Workspace.hpp>`
**Namespace:** `bg2e::ui`

Lays out a set of [`Window`](Window.md) instances into a fixed
editor arrangement: a top **toolbar**, **left / right / bottom** docked
panels, and a bottom **status bar**. It computes and locks each window's
position and size, reacting to viewport resizes and UI-scale changes. It is
**not** a free-docking system: it owns the geometry of the windows it manages.

```cpp
class BG2E_API Workspace {
public:
    struct PanelSize { uint32_t min = 200; uint32_t max = 400; float relative = 0.2f; };

    void setup(uint32_t width, uint32_t height,
               Window* toolBar, Window* leftPanel, Window* rightPanel,
               Window* bottomPanel, Window* statusBar = nullptr);
    void resize(uint32_t width, uint32_t height);
    void draw();

    bool isValid() const;

    // Per-region: XxxVisible() / toggleXxx() / showXxx() / hideXxx() / setXxxVisible(bool)
    //   Xxx in { ToolBar, LeftPanel, RightPanel, BottomPanel, StatusBar }
    bool toolBarVisible() const;   void toggleToolBar();   void showToolBar();  ...

    PanelSize& leftPanelSize();    PanelSize& rightPanelSize();    PanelSize& bottomPanelSize();
};
```

---

## Regions and geometry

The viewport is partitioned (all sizes multiplied by `UserInterface::getScale()`):

```
+------------------------------------------------------+
|  toolBar (fixed height 60 * scale, no title bar)     |
+-------------+---------------------+------------------+
|             |                     |                  |
|  leftPanel  |      free 3D        |    rightPanel    |
|             |      viewport       |                  |
|             |                     |                  |
+-------------+---------------------+------------------+
|  bottomPanel (title bar kept)                        |
+------------------------------------------------------+
|  statusBar (fixed height 50 * scale)                 |
+------------------------------------------------------+
```

- **Toolbar**: full width, top, height `_toolBarHeight` (60), no title bar.
- **Status bar**: full width, bottom, height `_statusBarHeight` (50).
- **Left / right panels**: width `clamp(vp * relative, min, max)`, full height
  between toolbar and status bar.
- **Bottom panel**: full width, above the status bar.
- Panels are `Window*`; any may be `nullptr` (skipped).

`PanelSize` controls the responsive width/height:

| Field | Meaning |
|-------|---------|
| `relative` | Fraction of the viewport the panel wants. |
| `min` / `max` | Clamped bounds (in *logical* pixels, before scale). |

The computed `getPanelSize()` is `clamp(vp * relative, min, max)`, then
`* scale`.

---

## Window options are owned by the Workspace

`setup()` and `updateWindows()` **overwrite** `Window::options`, position and
size of every managed window (setting `noMove`, `noResize`, `noCollapse`,
`noClose`, `noBringToFront`, and for the toolbar/status bar `noTitleBar`).
This is what lets `Window::draw()` re-apply geometry every frame (the
FirstUseEver rule flips to per-frame when `noMove`/`noResize` are set).

Consequences:

- Do not hand-configure the panels' `options`, `setPosition` or `setSize`;
  your changes are discarded on the next layout pass.
- Configure the **content** (via `setDrawFunction`) and the **sizes** (via
  `leftPanelSize()`, etc.), nothing else about the window frame.
- Left/right/bottom panels keep their title bar and collapse arrow (so you can
  label and collapse them); the toolbar and status bar do not.

---

## When layout runs

`updateWindows()` (recomputes geometry + options) runs on:

- `setup()`
- `resize()` — call from the delegate's `swapchainResized()`.
- Any `toggle/show/hide/setVisible` call.
- The first `draw()` after `UserInterface::getScale()` changes (the workspace
  caches `_uiScale` and detects drift).

`draw()` then renders each *visible* window by calling its no-arg `draw()`,
in order: toolbar → left → right → bottom → status bar.

---

## Visibility

Each region has `bool _drawXxx` (default `true`). The `toggle/show/hide/set`
family flips it and immediately relayouts, so hiding the toolbar reflows the
panels to reclaim the space. Use these to build a "View" menu (see
[quick_start Recipe 4](quick_start.md#recipe-4-menus-with-keyboard-shortcuts)).

---

## Minimal setup

```cpp
void AppDelegate::initWorkspace()
{
    _workspace.leftPanelSize() = { .min = 300, .max = 600, .relative = 0.20f };

    _workspace.setup(
        uiWidth(), uiHeight(),
        &_toolBar, &_leftPanel, &_rightPanel,
        nullptr,        // no bottom panel
        &_statusBar     // optional
    );
}

void AppDelegate::swapchainResized(VkExtent2D extent)
{
    DefaultRenderLoopDelegate::swapchainResized(extent);
    _workspace.resize(uiWidth(), uiHeight());
}

void AppDelegate::drawUI() { _workspace.draw(); }
```

Reference implementation: `apps/model_edit/src/AppDelegate.cpp`
(`initWorkspace`, `swapchainResized`, `drawUI`).

---

## See also

- [quick_start — Recipe 3](quick_start.md#recipe-3-build-an-editor-layout-with-workspace)
- [Window](Window.md), [Toolbar](Toolbar_and_StatusBar.md)
- Global tutorial: `doc/workspace_api.md`
