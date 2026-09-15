# BasicWidgets

**Header:** `<bg2e/ui/BasicWidgets.hpp>`
**Namespace:** `bg2e::ui`

A stateless, static-only façade over the most common ImGui controls and
layout operations. It adds no per-widget object model: every call draws
immediately into the current window/child scope. Use it as the building block
for the bodies you hand to [`Window`](Window.md) draw lambdas.

```cpp
class BG2E_API BasicWidgets {
public:
    // Placement
    static void sameLine(int32_t xPos = 0);
    static void spacing(int32_t spacing = 20);
    static void padding(uint32_t width, uint32_t height);
    static float getContentRegionAvailWidth();
    static float getContentRegionAvailHeight();
    static void beginChild(const std::string& id, float w = 0, float h = 0, bool border = true);
    static void endChild();

    // Text / decoration
    static void text(const std::string&, bool sameLine = false);
    static void separator(const std::string& title = "", bool sameLine = false);
    static void listItem(const std::string&, bool sameLine = false);
    static void tooltip(const std::string& text);

    // Controls (return true on interaction / change)
    static bool button(const std::string& title, bool sameLine = false, bool disabled = false);
    static bool checkBox(const std::string& title, bool* value = nullptr, bool sameLine = false, bool disabled = false);
    static bool radioButton(const std::string& label, int* value = nullptr, int id = 0, bool sameLine = false, bool disabled = false);

    // Collapsible scopes
    static bool beginTree(const std::string& label);
    static void endTree();
    static bool collapsingHeader(const std::string& title, bool visible = true);

    // Group enable/disable + ID stack
    static void beginDisabled(bool disabled = true);
    static void endDisabled();
    static void pushId(int id);
    static void popId();

    // Measurement
    static uint32_t calcTextWidth(const std::string&);
    static uint32_t calcTextHeight(const std::string&);
    static uint32_t calcButtonWidth(const std::string&);
    static uint32_t calcButtonHeight(const std::string&);
    static uint32_t getItemHorizontalSpacing();
    static uint32_t getItemVerticalSpacing();
};
```

---

## Layout helpers

### `sameLine(int32_t xPos = 0)`

Places the *next* widget on the current line. The sign of `xPos` changes the
reference frame:

| `xPos` | Behavior |
|--------|----------|
| `0` (default) | Natural next-column position (ImGui default). |
| positive | Absolute x offset from the **left** window edge. |
| **negative** | Offset measured from the **right** edge (`|xPos|` px). |

The negative form is how the engine right-aligns widgets without knowing the
window width. `Toolbar`, `StatusBar` and `ComponentInspector` compute a group
width with the `calc*` helpers and then call `sameLine(-totalWidth)`:

```cpp
auto w = calcButtonWidth("Remove") + getItemHorizontalSpacing();
sameLine(-w);                 // pin the Remove button to the right edge
button("Remove");
```

### `beginChild(id, w, h, border)` / `endChild()`

An indented sub-region with its own scroll. `w`/`h` of `0` fill the available
space. Use it to give a list or a preview its own scroll bar independent of
the parent window.

### `spacing(n)` / `padding(w, h)`

Both insert empty vertical space (`spacing` is a shorthand for a 0-wide
`padding`). They are `ImGui::Dummy` boxes, not layout margins.

---

## Controls and their return contract

`button`, `checkBox`, `radioButton` and every `Input::*` widget return `true`
**only on the frame the user interacts** (click / change). Drive your
reactive logic off that boolean, not by comparing stored values:

```cpp
if (checkBox("Wireframe", &_wireframe))   { rebuildPipelines(); }
if (button("Export"))                     { exportScene(); }
```

- `checkBox` / `radioButton` accept a nullable pointer; pass a pointer to
  persistent storage to have the widget read/write it (a plain `bool*`).
- `radioButton` sets `*value = id` for whichever option is clicked — model a
  group as several `radioButton` calls sharing one `int*`.
- The `disabled` argument wraps a single control in
  `beginDisabled()/endDisabled()`; `nullptr` values are allowed (draw-only,
  not editable).

### `tooltip(text)`

Call it **immediately after** the widget you want annotated; it shows `text`
when that widget is hovered. Empty text is a no-op, so it is safe to always
emit the call.

---

## Collapsible scopes

- `collapsingHeader(title, visible)` is a self-closing section: it returns
  `true` while expanded, and you do **not** pair it with an end call. When
  `visible` is `true` it starts open.
- `beginTree(label)` / `endTree()` are the paired form: if `beginTree` returns
  `true` you **must** call `endTree()` after drawing the children (and skip it
  entirely if it returned `false`).

```cpp
if (collapsingHeader("Transform")) {   // no matching end
    drawTransformFields();
}
```

---

## Group disable

`beginDisabled(flag)` greys out and blocks everything until `endDisabled()`
(unconditional pairing). Reflection widgets and inspectors use it to present
read-only values as visible-but-inactive controls rather than hiding them.

---

## Identifier management

ImGui keys each widget by its label. Duplicated labels in the same scope
collide (shared state, wrong tooltip target, flicker). Two tools:

1. **`##suffix` convention** — `button("OK##dialog1")` shows `OK` but IDs as
   `dialog1`. This is used throughout the engine (e.g. `"UV Set##metallic"`,
   `"Remove##..."`) so several materials/rows can show the same caption.
2. **`pushId(int)` / `popId()`** — prefix a numeric ID onto everything drawn
   between the calls; `ComponentInspector` and `ReflectionWidget` wrap each
   row/property in a `pushId(index)/popId()` pair to disambiguate repeated
   labels inside a list.

The `calc*` + `getItem*Spacing` helpers measure text/buttons in the *current
scale* so manual right-alignment (see `sameLine`) stays correct when the UI
scale changes.

---

## `SelectableList`

**Header:** `<bg2e/ui/SelectableList.hpp>`

A small static helper that renders a bordered table whose rows are selectable,
with an optional trailing button column — the widget behind `SubmeshSelector`.

```cpp
class BG2E_API SelectableList {
public:
    static void beginList(int columns = 1);
    static bool item(const std::string& title, bool& selected);   // true on click
    static bool itemButton(const std::string& title);             // true on click
    static void endList();
};
```

- `beginList(columns)` opens an ImGui table (settings intentionally not saved
  across sessions).
- `item(title, selected)` starts a new row + first column; `selected` is the
  in/out selection flag. Use one shared `bool` per row for single-select, or a
  per-row flag for multi-select.
- `itemButton(title)` adds a button in the **next** column of the current row.
- Order in a loop is `item()` then optional `itemButton()` per row, wrapped by
  `beginList()/endList()`.

Recipe with toggle semantics:
[quick_start Recipe 7](quick_start.md#recipe-7-selectable-lists).

---

## See also

- [Input](Input.md) — editable value widgets (built on the same primitives).
- [reference.md — BasicWidgets](reference.md#basicwidgets)
- `examples/02_ui/src/main.cpp` — a tour of most `BasicWidgets` calls.
