# Toolbar and StatusBar

**Headers:** `<bg2e/ui/Toolbar.hpp>`, `<bg2e/ui/ActionDefinition.hpp>`, `<bg2e/ui/StatusBar.hpp>`
**Namespace:** `bg2e::ui`

Two specialized [`Window`](Window.md) subclasses that provide fixed-content
chrome for a [`Workspace`](Workspace.md): a top **toolbar** (menu + buttons)
and a bottom **status bar** (live text fields). Both own their window body —
their `setDrawFunction()` is overridden to a no-op, and content is added
through dedicated item APIs instead.

---

## Toolbar icons and configuration

Add PNG paths directly during toolbar setup. Application handlers are assumed
below; the toolbar performs demand loading only when an icon mode uses a path.

```cpp
const auto saveId = toolbar.addButton({
    .label = "Save",
    .action = [&document]() { document.save(); },
    .iconPath = std::filesystem::path{"assets/icons/save.png"},
    .tooltip = "Save document"
});
toolbar.addButton({.label = "Ready"}, bg2e::ui::Toolbar::AlignRight);

// Optional local override; otherwise the toolbar follows UI Settings.
toolbar.setPresentation(bg2e::ui::ToolbarPresentation::IconsAndText);
toolbar.setIconSize(20.0f);
```

```cpp
struct ToolbarButton {
    int32_t id = -1;
    std::string label;
    std::function<void()> action = nullptr;
    bool disabled = false;
    std::filesystem::path iconPath;
    std::string tooltip;
};

enum class ToolbarPresentation
{
    Text = 0,
    IconsWithTooltips = 1,
    IconsAndText = 2
};

struct ToolbarConfiguration {
    ToolbarPresentation presentation = ToolbarPresentation::IconsAndText;
    float iconSize = 16.0f;
};
```

The appended fields preserve old aggregate initializer order. ToolbarConfiguration
and ToolbarPresentation are declared in `<bg2e/ui/ToolbarConfiguration.hpp>`.

| Mode | Entry with a nonempty icon path | Entry without a path |
| --- | --- | --- |
| Text | Label; no icon loading | Label |
| IconsWithTooltips | Icon; tooltip defaults to visible label | Label |
| IconsAndText | Icon and label in one button | Label |

An explicit tooltip takes priority in every mode. Nonempty missing/corrupt PNG
paths display the shared [icon substitute](Icons.md) and retain their layout box.
An entry with no action remains noninteractive content in all three modes, with
icon/text presentation and disabled/tooltip styling. Text buttons preserve the
old label `##` hidden-suffix convention; actionless labels remain literal text.

### Runtime API

| Method | Behavior |
| --- | --- |
| `ToolbarConfiguration configuration() const` | Effective local or active-session settings, resolved at call time. |
| `void setConfiguration(const ToolbarConfiguration&)` | Validates and installs a complete local override. |
| `void setPresentation(ToolbarPresentation)` | Overrides presentation and snapshots the current size. |
| `void setIconSize(float)` | Overrides size and snapshots the current presentation. |
| `bool inheritsSessionDefaults() const` | True until overridden, or after resetting inheritance. |
| `void useSessionDefaults()` | Removes the complete local override. |
| `void updateButtonLabel(id, const std::string&)` / `(id, std::string&&)` | Updates visible text. |
| `void updateButtonIconPath(id, const std::filesystem::path&)` | Changes source on subsequent draws. |
| `void updateButtonTooltip(id, const std::string&)` | Changes tooltip text. |
| `void enableButton(id)` / `void disableButton(id)` | Updates enabled state. |
| `ToolbarButton* findButton(id)` / `const ToolbarButton* findButton(id) const` | Finds either side; nullptr if absent. |

Unknown IDs in update/enable/disable methods are no-ops. `iconSize` must be finite
and positive, in logical units. Scale applies once, and aspect ratio is preserved
inside the square. Size/tint/presentation do not create another cached image.
Earlier source paths remain cached when a widget changes path.

By default, each toolbar reads the active session configuration on each draw.
The default configuration is IconsAndText/16: assigning a path displays the icon
alongside the label. An explicitly saved Text preference still hides icons. Global changes are persisted through
UserInterface and exposed by [UI Settings](Settings_Windows.md#toolbar-preferences).
Local overrides affect neither preferences nor other toolbars:

```cpp
auto configuration = bg2e::ui::UserInterface::getToolbarConfiguration();
configuration.presentation = bg2e::ui::ToolbarPresentation::IconsWithTooltips;
bg2e::ui::UserInterface::setToolbarConfiguration(configuration);

toolbar.useSessionDefaults();  // follow those current defaults again
```

Either convenience setter creates a whole-configuration override; there is no
per-field inheritance. Explicit overrides remain effective when UI Settings
changes session defaults.

### Layout and identity

Left/right entries retain insertion order. Shared float content measurements
include icon boxes, text, inner spacing and frame padding for actionable entries.
Every draw remeasures both groups, vertically centers entries, and packs right
entries against the content edge. A customizable toolbar puts entries that
exceed the available width into **More Tools**. Other toolbars clip at the
window edge. The embedding Window or Workspace sets the height. Use
`Workspace::setToolBarHeight` to allow room for large icons or multiline labels.

`addButton(ToolbarButton&, Alignment = AlignLeft)` and its rvalue overload return
the final ID. Negative IDs are replaced with the smallest available nonnegative
ID across both sides; the reference overload updates its input. Explicit duplicate
IDs throw invalid_argument. Exhaustion raises overflow_error. IDs are scoped by
the toolbar instance and button ID, so changing path, label or mode keeps identity.
Do not change IDs through mutable findButton access; draw rejects negative or
duplicate IDs introduced that way. Returned pointers can be invalidated by adding
buttons. Adding/updating from an action callback is rendered on the next draw;
actions run after the current row has been submitted using a snapshot.

### Menu

`registerAction` registers an action in the toolbar catalog and any menu named
by its `path`. Use `|` between parent names, for example `"View|Settings"`.
An empty path makes the action available through the toolbar catalog alone.
The toolbar creates each parent on first use and preserves registration order.
Use stable action IDs so that saved layouts continue to resolve after a label,
icon, or menu path changes. A shared definition supplies execution, availability,
the checked state, and a dynamic label to both surfaces. The settings catalog
uses its base `label`. Shortcuts are independent of menu and button visibility.

`registerActions` accepts a span of definitions or a braced list. Both
registration methods support `addSeparator`. It adds a menu separator after
the only or final action when that action has a path. All IDs and paths in a
batch are validated before any action is added.

Pass `Toolbar::MenuSeparator::AfterSubmenu` to `registerActions` to place the
separator after the final action's submenu in its parent menu.

Call `setDefaultLayout` after registering the actions. Its left and right
lists use action IDs and can include `ToolbarLayout::Separator` for a toolbar
separator. This layout is also used by **Restore Defaults**. Standalone
menus can use [Menu and MenuItem](Menu.md) to build their own trees.

For example, an editor can register these actions during initialization.

```cpp
using bg2e::ui::Toolbar;
using bg2e::ui::ToolbarLayout;

toolbar.registerAction({
    .id = "file.open",
    .label = "Open...",
    .execute = [this]() { openDocument(); },
    .path = "File"
});
toolbar.registerActions({
    {
        .id = "window.interface",
        .label = "User Interface",
        .execute = [this]() { openInterfaceSettings(); },
        .path = "Window|Settings"
    },
    {
        .id = "window.graphics",
        .label = "Graphics",
        .execute = [this]() { openGraphicsSettings(); },
        .path = "Window|Settings"
    }
}, Toolbar::MenuSeparator::AfterSubmenu);
toolbar.registerAction({
    .id = "window.resources",
    .label = "Resources",
    .execute = [this]() { openResources(); },
    .path = "Window"
});
toolbar.setDefaultLayout({
    .left = {"file.open", ToolbarLayout::Separator, "window.resources"}
});
```

This produces **Window > Settings** with two entries, followed by a separator
and **Resources** in **Window**. The separator in the left toolbar layout is
independent of that menu separator.

### Toolbar customization

Call `enableCustomization()` after registering the actions and setting the
default layout. Right-click the toolbar to open **Toolbar Settings…**. The
window edits the presentation, logical icon size (12–32), and the order of
actions on each side. Drag actions from **Available Tools** into either side,
between sides, or back to the catalog. Drop on a row to insert before it.
The **Separator** catalog item can be added multiple times and each instance
can be moved or removed independently.

For persistence, supply a distinct file per toolbar and use the application's
settings directory. Composer uses this form.

```cpp
toolbar.enableCustomization(
    bg2e::base::PlatformTools::settingsPath() / "composer-scene-tools.toolbar");
```

On first use, the toolbar copies the session appearance into its local settings.
The file stores appearance and, when customized, both ordered lists. A saved
empty side stays empty. Unknown action IDs and repeated actions are ignored
while repeated separators are preserved. An unreadable or unsupported file
falls back silently to the initial layout and appearance. **Restore Defaults**
restores the declared layout and the standard appearance. Use
`UISettingsWindow::setToolbarSettingsVisible(false)` when the application
exposes toolbar appearance through this window.

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
