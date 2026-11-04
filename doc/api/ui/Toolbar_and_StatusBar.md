# Toolbar and StatusBar

**Headers:** `<bg2e/ui/Toolbar.hpp>`, `<bg2e/ui/StatusBar.hpp>`
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
Every draw remeasures both groups, vertically centers them to the tallest entry,
and packs right entries against the content edge when they fit. Insufficient
width clips rather than overlapping the left group or wrapping. The embedding
Window/Workspace owns window height; provide enough height for large icons or
multiline labels. Toolbar does not shrink configured icons or resize the workspace.

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

`addMenuItem(const MenuItem&)` forwards to the internal [Menu](Menu.md), including
its iconPath support. The toolbar installs its window menu callback after the
first body draw; shortcut registration occurs when the menu first draws. The
existing `_menuInitialized` assignment remains false, so that callback continues
to be installed on subsequent draws. This icon work does not change that behavior.

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
