# Window

**Header:** `<bg2e/ui/Window.hpp>`
**Namespace:** `bg2e::ui`

A declarative wrapper around an ImGui window. Holds a title, a set of
`Options` flags translated to `ImGuiWindowFlags`, an open/close state, and an
optional position/size. It is the base class of `Toolbar`, `StatusBar`,
`UISettingsWindow` and `RenderSettingsWindow`, and can also be used directly
for application panels.

```cpp
class BG2E_API Window {
public:
    enum DockingSide { DockLeft, DockRight, DockBottom };

    struct Options {
        bool noTitleBar = false;   bool noScrollbar = false;
        bool noMenu = false;       bool noMove = false;
        bool noResize = false;     bool noCollapse = false;
        bool noNav = false;        bool noBackground = false;
        bool noBringToFront = false; bool noClose = false;
        int minWidth = 0;          int minHeight = 0;
        int maxWidth = INT_MAX;    int maxHeight = INT_MAX;
    };
    Options options;

    void setTitle(const std::string&);      const std::string& title() const;

    // Mode 1: draw now with a body lambda (+ optional per-call menu)
    virtual void draw(std::function<void()> drawFunction,
                      std::function<void()> menuFunction = nullptr);

    // Mode 2: configure body, then draw with no arguments
    virtual void setDrawFunction(std::function<void()>);
    void         setMenuFunction(std::function<void()>);
    virtual void draw();

    void close();  void open();  bool isOpen() const;
    void setPosition(int x, int y);  int positionX() const;  int positionY() const;
    void setSize(int w, int h);      int width() const;      int height() const;
};
```

---

## Two draw modes

**Mode 1 — lambda at call site.** Pass the body to `draw(fn[, menuFn])` every
frame. The body and menu closures capture whatever state they need; good for
one-off windows.

**Mode 2 — preconfigured body.** Store the body once with
`setDrawFunction()` (and optionally `setMenuFunction()`), then call `draw()`
with no arguments. This is the mode `Workspace` uses: it calls `_window->draw()`
and therefore **requires** every managed window to have a draw function.
Subclasses with fixed content (`Toolbar`, `StatusBar`) override
`setDrawFunction()` to a no-op.

Both modes do nothing while the window is closed.

---

## Position, size and the FirstUseEver rule

`setPosition` / `setSize` feed `ImGui::SetNextWindowPos` / `SetNextWindowSize`,
but with a subtlety implemented in `Window::draw`:

- If `noMove` / `noResize` is **false**, the value is applied with
  `ImGuiCond_FirstUseEver` — i.e. once, then the user owns the geometry.
- If `noMove` / `noResize` is **true**, the value is applied **every frame**,
  so the window is effectively locked where the engine says.

`Workspace` sets both `noMove` and `noResize` on its toolbar/panels/status bar
and repositions them, which is why resizing the viewport calls
`Workspace::resize()`.

A size set with `setSize()` is also constrained by `SetNextWindowSizeConstraints`
(`options.minWidth/Height`, `maxWidth/Height`).

---

## Menu bar

The menu bar region is only created when a menu function exists
(`setMenuFunction` or the `menuFunction` argument). `Window::draw()` opens the
`ImGui::BeginMenuBar()`/`EndMenuBar()` scope itself, so the callback emits the
menu contents directly, using the [`Menu`](Menu.md) primitives:

```cpp
_window.setMenuFunction([&]() {
    if (Menu::beginMenu("File")) {
        if (Menu::menuItem("Save", "Ctrl+S")) { save(); }
        Menu::separator();
        Menu::endMenu();
    }
});
```

Keep `options.noMenu = false` (the default) so `ImGuiWindowFlags_MenuBar` is
enabled.

---

## Open / close

`close()` / `open()` toggle an internal flag; `isOpen()` reports it. When
`options.noClose` is false the ImGui close button writes to the same flag, so
the user's **X** and your code share state. The recommended pattern for a
togglable panel is a menu item / shortcut bound to:

```cpp
if (_panel.isOpen()) _panel.close(); else _panel.open();
```

---

## Subclassing

Override `draw()` to render fixed content (call `Window::draw(body[, menu])`
from it), and override `setDrawFunction()` to a no-op if the content is not
user-suppliable — exactly what `Toolbar` and `StatusBar` do. The virtual
`draw()` (no-arg) is the entry point used by `Workspace`.

---

## See also

- [quick_start — Recipe 2](quick_start.md#recipe-2-a-simple-window-with-a-draw-lambda)
- [Workspace](Workspace.md) — layout built on top of `Window`.
- [reference.md — Window](reference.md#window)
