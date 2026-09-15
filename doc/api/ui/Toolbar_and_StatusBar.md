# Toolbar and StatusBar

**Headers:** `<bg2e/ui/Toolbar.hpp>`, `<bg2e/ui/StatusBar.hpp>`
**Namespace:** `bg2e::ui`

Two specialized [`Window`](Window.md) subclasses that provide fixed-content
chrome for a [`Workspace`](Workspace.md): a top **toolbar** (menu + buttons)
and a bottom **status bar** (live text fields). Both own their window body —
their `setDrawFunction()` is overridden to a no-op, and content is added
through dedicated item APIs instead.

---

## `Toolbar`

```cpp
struct ToolbarButton {
    int32_t id = -1;
    std::string label;
    std::function<void()> action = nullptr;   // nullptr => draw as plain text
    bool disabled = false;
};

class BG2E_API Toolbar : public Window {
public:
    enum Alignment { AlignLeft, AlignRight };

    int32_t addButton(ToolbarButton&  button, Alignment align = AlignLeft);
    int32_t addButton(ToolbarButton&& button, Alignment align = AlignLeft);

    void draw() override;

    void updateButtonLabel(int32_t buttonId, const std::string& newLabel);
    void updateButtonLabel(int32_t buttonId, std::string&& newLabel);
    void enableButton(int32_t buttonId);
    void disableButton(int32_t buttonId);
    ToolbarButton* findButton(int32_t buttonId);

    void addMenuItem(const MenuItem& item);   // forwards to an internal Menu
};
```

### Layout model

Left-aligned buttons are drawn in insertion order; right-aligned buttons are
drawn after, packed against the right edge (the total right-side width is
measured each frame with `Layout::calcButtonWidth` / `calcTextWidth`,
then placed with `Layout::sameLine(-total)`). Because alignment is computed
from live text metrics, `updateButtonLabel()` re-flows automatically.

### IDs

`addButton` returns the button's final id. If you pass `id < 0` the toolbar
generates the smallest unused non-negative id (unique across both sides); the
reference overload also writes the generated id back into your
`ToolbarButton`. Keep the returned id and use it later with
`updateButtonLabel` / `enableButton` / `disableButton` / `findButton`
(unknown id ⇒ no-op / `nullptr`).

### Buttons vs text

`action == nullptr` renders the entry as a plain text label inside the bar —
useful for read-only indicators (mode names, statistics). Clickable entries
grey out when `disabled` is set; `disableButton(id)` is the idiomatic way to
track application state (e.g. no active document).

### Menu

The toolbar hosts one [`Menu`](Menu.md) instance: call `addMenuItem()` per
top-level menu during setup; the toolbar installs it as the window menu-bar
function on first draw and the `Menu` draws itself afterwards (shortcuts are
registered on that first draw).

```cpp
void ToolBar::init(AppDelegate* app)
{
    _toolbar.updateButtonLabel(undoId, "Undo");    // live updates elsewhere
    MenuItem file("File");
    file.addMenuItem(MenuItem("Open...", openShortcut));
    _toolbar.addMenuItem(file);
}
```

> **Gotcha (source review):** `Toolbar::draw(...)` guards menu installation
> with `if (!_menuInitialized)` but assigns `_menuInitialized = false` inside
> the branch (looks like a typo for `true`), so the menu function is
> (re)installed on every draw. Functionally harmless — the callback is stable
> — but do not rely on `_menuInitialized` as a "one-shot" signal.

---

## `StatusBar` and `StatusItem`

```cpp
class BG2E_API StatusItem {
public:
    void setText(const std::string& text);
    void setText(std::string&& text);
    const std::string& getText() const;
};

class BG2E_API StatusBar : public Window {
public:
    enum Alignment { AlignLeft, AlignRight };
    void draw() override;
    void addItem(std::shared_ptr<StatusItem> item, Alignment align = AlignLeft);
};
```

`StatusItem` is a tiny observable string. The bar renders the left group
(first item on a new line, the rest inline) and right-aligns the right group
by measuring the total text width and using the negative-`sameLine` trick.
Vertical centering inside the bar height is done with `Layout::padding`.

### Update pattern

`addItem` stores a `shared_ptr`, so the application keeps its own handle and
mutates the text from anywhere — UI, document code, async callbacks — and the
change shows on the next frame:

```cpp
_fileStatus = std::make_shared<ui::StatusItem>();
_statusBar.addItem(_fileStatus, ui::StatusBar::AlignLeft);

// ... later, in Document:
_fileStatus->setText(documentPath().filename().string());
```

No dirty tracking, no redraw requests: the immediate-mode loop redraws the bar
every frame anyway. The `apps/model_edit` "file / save status" fields use
exactly this pattern (`AppDelegate::initWorkspace`).

---

## See also

- [quick_start — Recipes 5 & 6](quick_start.md#recipe-5-toolbar-buttons-with-live-state)
- [Workspace](Workspace.md) — geometry and option ownership.
- [Menu](Menu.md), [Window](Window.md).
