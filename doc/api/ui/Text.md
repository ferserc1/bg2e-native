# Text

**Header:** `<bg2e/ui/Text.hpp>`
**Namespace:** `bg2e::ui`

Static-only, non-interactive display widgets: labels, section separators,
bulleted items and hover tooltips.

```cpp
class BG2E_API Text {
public:
    static void text(const std::string&, bool sameLine = false);
    static void separator(const std::string& title = "", bool sameLine = false);
    static void listItem(const std::string&, bool sameLine = false);
    static void tooltip(const std::string& text);
};
```

---

## Members

- `text(text, sameLine)` — non-editable label.
- `separator(title, sameLine)` — section separator with an optional inline
  title (`""` draws a bare divider).
- `listItem(label, sameLine)` — bulleted line.

### `tooltip(text)`

Call it **immediately after** the widget you want annotated; it shows `text`
when that widget is hovered. Empty text is a no-op, so it is safe to always
emit the call.

---

## See also

- [Layout](Layout.md) — placement and measurement of display widgets.
- [Button](Button.md) — interactive controls.
- [reference.md — Text](reference.md#text)
- `examples/02_ui/src/main.cpp` — a tour of most `Text` calls.
