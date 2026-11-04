# Menu and MenuItem

**Header:** `<bg2e/ui/Menu.hpp>`
**Namespace:** `bg2e::ui`

Two complementary ways to build menus:

- **`Menu` static methods** — thin wrappers over the ImGui menu-bar calls for
  hand-written menus.
- **`Menu` instance + `MenuItem` tree** — a data-driven model where entries
  are *declared once* with their keyboard shortcut and handler, and the same
  `ShortcutData` drives both the menu item and the global hotkey through
  `app::Shortcuts`.

```cpp
class BG2E_API Menu {
public:
    // Static (hand-written) menu-bar primitives
    static bool beginMenuBar();           static void endMenuBar();
    static bool beginMenu(label, enabled = true);  static void endMenu();
    static bool menuItem(label, shortcutDisplay = "", selected = false, enabled = true);
    static void separator();

    // Data-driven item tree
    void addMenuItem(const MenuItem& item);
    void draw();                          // renders the tree (+ installs shortcuts)
};

class BG2E_API MenuItem {
public:
    MenuItem();                                       // separator
    MenuItem(const std::string& label);               // submenu container / separator
    MenuItem(label, ShortcutData);                    // action
    MenuItem(label, ShortcutData, std::function<bool()> isChecked);  // checkable

    std::string label;
    std::filesystem::path iconPath;  // optional PNG
    void addMenuItem(const MenuItem& item);           // -> Submenu, appends child
    void draw() const;
    void initShortcuts() const;
};

enum MenuItemType { Action, CheckableAction, Separator, Submenu };
```

---

## Which API to use

| Situation | Use |
|-----------|-----|
| Small, static menu; you write the layout inline | `Menu::beginMenuBar()` + `Menu::menuItem(...)` |
| Command with a global keyboard shortcut (menu entry *and* hotkey share one handler) | `MenuItem` tree with `ShortcutData` |
| Checkable item whose state lives elsewhere (e.g. panel visibility) | `MenuItem` with the `isChecked` predicate |

The `shortcut` string in the *static* `Menu::menuItem(label, shortcut, ...)`
is **display-only** (it just prints `"Ctrl+S"` next to the item); it registers
no hotkey. Only the `MenuItem`/`ShortcutData` path wires real shortcuts.

---

## `ShortcutData`

`MenuItem` actions carry an `app::Shortcuts::ShortcutData`
(`<bg2e/app/Shortcuts.hpp>`):

```cpp
struct ShortcutData {
    bool altModifier = false;
    bool ctrlModifier = false;   // on macOS see note below
    bool shiftModifier = false;
    KeyEvent::Key key = KeyEvent::KeyUnknown;
    ShortcutHandler handler;      // std::function<void()>
    std::string getShortcutString() const;   // "Ctrl+Shift+S", shown in the menu
};
```

The same `handler` runs whether the user clicks the menu item or presses the
hotkey, so it is the single source of truth for the command. Writing
`key = KeyUnknown` makes a click-only action (no hotkey registered).

### When shortcuts are registered

`Menu::draw()` calls `initShortcuts()` **once**, on the first frame the menu
is drawn. `initShortcuts()` walks the tree and registers every `Action` /
`CheckableAction` with `app::MainLoop::current()->shortcuts()`. Therefore:

- The `Menu`/`MenuItem` must be drawn inside a running `MainLoop` (it needs
  `MainLoop::current()`).
- Items **added after the first draw** render but their shortcuts are not
  registered (the `_shortcutInitialized` latch already fired). Build the full
  tree during `init()`.

---

## Building a data-driven menu

```cpp
using app::Shortcuts::ShortcutData;
using bk = app::KeyEvent;

// File menu with an action + separator + quit
MenuItem fileMenu("File");                              // label-only ctor => submenu container
fileMenu.addMenuItem(MenuItem("Save",
    ShortcutData{ .ctrlModifier = true, .key = bk::KeyS,
                  .handler = [this]{ save(); } }));
fileMenu.addMenuItem(MenuItem());                        // separator (default ctor)
fileMenu.addMenuItem(MenuItem("Quit",
    ShortcutData{ .ctrlModifier = true, .key = bk::KeyQ,
                  .handler = [this]{ MainLoop::current()->exit(); } }));

// View menu with a checkable item tracking panel visibility
MenuItem viewMenu("View");
viewMenu.addMenuItem(MenuItem("Scene Tree",
    ShortcutData{ .ctrlModifier = true, .key = bk::KeyT,
                  .handler = [this]{ _workspace.toggleLeftPanel(); } },
    [this]{ return _workspace.leftPanelVisible(); }));

_menu.addMenuItem(fileMenu);
_menu.addMenuItem(viewMenu);
```

And render it inside a window's menu function — `Window::draw()` opens the
menu-bar scope itself (`ImGui::BeginMenuBar`/`EndMenuBar`), so the callback
only emits the items:

```cpp
_window.setMenuFunction([this]{
    _menu.draw();               // draws + (first frame) installs shortcuts
});
```

A `Toolbar` exposes the same tree directly: `_toolBar.addMenuItem(item)`
forwards to its internal `Menu`, which it draws in the toolbar window's menu
bar automatically.

### Item type rules

| Construction | Resulting `MenuItemType` |
|--------------|--------------------------|
| `MenuItem()` or `MenuItem(label)` alone | `Separator` (with a label it is meant to become a submenu) |
| `MenuItem(label, shortcut)` | `Action` |
| `MenuItem(label, shortcut, checkedFn)` | `CheckableAction` |
| `item.addMenuItem(child)` on any item | promoted to `Submenu` |

`CheckableAction` polls `checkedFn()` every frame to draw the tick; it never
stores the check itself, so the predicate must reflect live state.

---

## macOS modifier note

`ShortcutData::ctrlModifier` maps to the physical Ctrl key. On macOS the
conventional "command" chord needs the engine's cmd/Ctrl abstraction (see
`app::Shortcuts` / `KeyEvent`); the header comment explicitly flags that
`ctrlModifier` is the right choice on Windows/Linux but to prefer a
cmd-or-ctrl form on Apple platforms.

---

## PNG icons

MenuItem accepts an optional path without changing existing constructors:

```cpp
bg2e::ui::MenuItem file("File");
bg2e::ui::MenuItem save("Save", saveShortcut);
save.iconPath = std::filesystem::path{"assets/icons/save.png"};
file.addMenuItem(save);
file.addMenuItem(bg2e::ui::MenuItem("Close", closeShortcut));
menu.addMenuItem(file);
```

The tree reserves the same icon gutter for all siblings whenever any action or
submenu has a path. Rows without icons stay aligned; separators do not influence
the gutter. Actions, checkable rows and submenu headers support icons. Constructors,
checked predicates, enabled callbacks and shortcut registration keep their prior
contracts. A path/cache hit does not register a shortcut again.

For manual menus, use explicit IDs and the new path-based primitives:

```cpp
// Members of ui::Menu:
static bool beginIconMenu(const std::string& id, const std::string& label,
    const std::filesystem::path& iconPath, bool enabled = true, bool reserveIconSpace = true);
static bool iconMenuItem(const std::string& id, const std::string& label,
    const std::filesystem::path& iconPath, const std::string& shortcut = "",
    bool selected = false, bool enabled = true, bool reserveIconSpace = true);
```

```cpp
if (bg2e::ui::Menu::beginMenuBar())
{
    if (bg2e::ui::Menu::beginIconMenu("file-menu", "File", std::filesystem::path{}))
    {
        if (bg2e::ui::Menu::iconMenuItem("save", "Save",
            std::filesystem::path{"assets/icons/save.png"}, "Ctrl+S"))
        {
            saveDocument();
        }
        if (bg2e::ui::Menu::iconMenuItem("close", "Close", std::filesystem::path{}))
        {
            closeDocument();
        }
        bg2e::ui::Menu::endMenu();
    }
    bg2e::ui::Menu::endMenuBar();
}
```

The shortcut string here is display-only. Use MenuItem/ShortcutData for actual
hotkeys. Pair successful beginIconMenu with endMenu and use the UI menu-bar
wrappers, including when composing a custom Window menu callback.

Low-level icon wrappers reserve a gutter by default even with empty paths; use
them for all sibling rows that should align. Set reserveIconSpace to false for a
whole icon-free sibling group. A nonempty path always reserves space. Legacy
beginMenu/menuItem do not reserve the extra gutter automatically.

Size follows current text-line height, already scaled; menus do not use toolbar
size preferences. A measured blank prefix contributes to the native label width,
and the icon is overlaid on that single native row. Check marks, shortcut text,
submenu arrows, popup closing and navigation remain native. Submenu icons draw on
the parent menu's captured draw list even after entering the child popup.

Explicit IDs must be nonempty and independent of the source path. Visible labels
retain native `##` display semantics, unlike iconTextButton's literal label.
Tree items use instance identity, stable across dynamic-label/path changes while
the tree structure stays stable; moving/rebuilding the tree can change those IDs.
Use manual explicit IDs when persistence across structural rebuilds is required.

Missing PNGs share [the cached fallback](Icons.md). Without an ImGui context,
the new static methods return false. In a bare ImGui context without UserInterface,
native text/gutters can draw but no icon is resolved; normal use is within the
owning initialized UI session.

---

## See also

- [quick_start — Recipe 4](quick_start.md#recipe-4-menus-with-keyboard-shortcuts)
- [Toolbar_and_StatusBar.md](Toolbar_and_StatusBar.md) — menus hosted in a toolbar.
- [Window](Window.md) — the menu-bar region.
- `doc/input_delegate.md`, `app/Shortcuts.hpp`.
