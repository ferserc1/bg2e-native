# Layout

**Header:** `<bg2e/ui/Layout.hpp>`
**Namespace:** `bg2e::ui`

A stateless, static-only façade over ImGui layout operations: in-line
placement, spacing, child regions and size metrics. It adds no per-widget
object model: every call applies immediately to the current window/child
scope. Use it inside the bodies you hand to [`Window`](Window.md) draw
lambdas.

```cpp
class BG2E_API Layout {
public:
    // Placement
    static void sameLine(int32_t xPos = 0);
    static void spacing(int32_t spacing = 20);
    static void padding(uint32_t width, uint32_t height);
    static float getContentRegionAvailWidth();
    static float getContentRegionAvailHeight();
    static void beginChild(const std::string& id, float w = 0, float h = 0, bool border = true);
    static void endChild();

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

## Placement

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
auto w = Layout::calcButtonWidth("Remove") + Layout::getItemHorizontalSpacing();
Layout::sameLine(-w);                // pin the Remove button to the right edge
Button::button("Remove");
```

### `beginChild(id, w, h, border)` / `endChild()`

An indented sub-region with its own scroll. `w`/`h` of `0` fill the available
space. Use it to give a list or a preview its own scroll bar independent of
the parent window.

### `spacing(n)` / `padding(w, h)`

Both insert empty vertical space (`spacing` is a shorthand for a 0-wide
`padding`). They are `ImGui::Dummy` boxes, not layout margins.

---

## Measurement

The `calc*` + `getItem*Spacing` helpers measure text/buttons in the *current
scale* so manual right-alignment (see `sameLine`) stays correct when the UI
scale changes.

- `calcTextWidth/Height(title)` — text measurement.
- `calcButtonWidth/Height(title)` — button frame measurement (text + frame
  padding).
- `getItemHorizontalSpacing()` / `getItemVerticalSpacing()` — style
  `ItemSpacing`.
- `getContentRegionAvailWidth()` / `getContentRegionAvailHeight()` — remaining
  space in the current window/child.

---

## See also

- [Text](Text.md) — labels and separators.
- [Button](Button.md) — the controls typically placed with these helpers.
- [reference.md — Layout](reference.md#layout)
- `examples/02_ui/src/main.cpp`
