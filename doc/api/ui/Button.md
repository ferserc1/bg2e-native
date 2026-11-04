# Button

**Header:** `<bg2e/ui/Button.hpp>`
**Namespace:** `bg2e::ui`

Static-only button family: command buttons, check boxes and radio buttons.

```cpp
class BG2E_API Button {
public:
    static bool button(const std::string& title, bool sameLine = false, bool disabled = false);
    static bool checkBox(const std::string& title, bool* value = nullptr, bool sameLine = false, bool disabled = false);
    static bool radioButton(const std::string& label, int* value = nullptr, int id = 0, bool sameLine = false, bool disabled = false);
};
```

---

## The return contract

`button`, `checkBox`, `radioButton` (and every value editor in
[`Numeric`](Numeric.md), [`Vector`](Vector.md) and [`Value`](Value.md)) return
`true` **only on the frame the user interacts** (click / change). Drive your
reactive logic off that boolean, not by comparing stored values:

```cpp
if (Button::checkBox("Wireframe", &_wireframe))   { rebuildPipelines(); }
if (Button::button("Export"))                     { exportScene(); }
```

- `checkBox` / `radioButton` accept a nullable pointer; pass a pointer to
  persistent storage to have the widget read/write it (a plain `bool*`).
- `radioButton` sets `*value = id` for whichever option is clicked — model a
  group as several `radioButton` calls sharing one `int*`.
- The `disabled` argument wraps a single control in
  [`Group::beginDisabled()/endDisabled()`](Group.md); `nullptr` values are
  allowed (draw-only, not editable).

---

## Icon buttons

Pass a PNG path and a stable control ID independently:

```cpp
const std::filesystem::path savePng = "assets/icons/save.png";
if (bg2e::ui::Button::iconButton("save-document", savePng, 20.0f,
    {.tooltip = "Save document"}))
{
    saveDocument();
}
if (bg2e::ui::Button::iconTextButton("save-copy", "Save copy", savePng, 20.0f))
{
    saveCopy();
}
```

The same PNG shares one resource while the two IDs identify different actions.
The additive static members are:

```cpp
// Members of ui::Button:
static bool iconButton(const std::string& id, const std::filesystem::path& path,
    float size = 20.0f, const IconDrawOptions& options = {});
static bool iconButton(const std::string& id, const std::shared_ptr<Icon>& icon,
    float size = 20.0f, const IconDrawOptions& options = {});
static bool iconTextButton(const std::string& id, const std::string& label,
    const std::filesystem::path& path, float size = 20.0f, const IconDrawOptions& options = {});
static bool iconTextButton(const std::string& id, const std::string& label,
    const std::shared_ptr<Icon>& icon, float size = 20.0f, const IconDrawOptions& options = {});
```

Both render one native interactive button: one hit rectangle, focus target,
keyboard/gamepad activation and return value. `id` must be nonempty and unique
in the current UI ID scope. Labels are literal visible text, including `##`;
changes to visible text, PNG path or display size do not change identity.

`size` is the logical icon square, not total button size. The shared measurement
adds text, inner spacing and normal frame padding. The source fits proportionally
inside the square; interface scale applies once. [IconDrawOptions](Image.md)
provides tint, tooltip, same-line placement and disabled state. Button text keeps
the current text color; image tint affects only the icon. Tooltips remain
available for disabled controls.

Empty paths/null resources omit the icon slot. iconTextButton then displays only
the text; iconButton produces a blank ordinary button. A nonempty failing path
keeps the icon box and shows the shared substitute. Without an active UI session,
or with a stale/cross-session resource, return false without drawing. Empty IDs
or nonpositive/nonfinite icon sizes raise invalid_argument when used in a live
session. Original text-button/check/radio APIs are unchanged.

See [PNG icons](Icons.md) for explicit resource loading, cache behavior and
recoverable asset errors.

---

## See also

- [Layout](Layout.md) — placement and button measurement.
- [Group](Group.md) — disabled groups and ID stack.
- [reference.md — Button](reference.md#button)
- `examples/draw/01_window_ui/src/main.cpp`
