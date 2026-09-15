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

## See also

- [Layout](Layout.md) — placement and button measurement.
- [Group](Group.md) — disabled groups and ID stack.
- [reference.md — Button](reference.md#button)
- `examples/02_ui/src/main.cpp`
