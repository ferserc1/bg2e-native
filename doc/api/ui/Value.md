# Value

**Header:** `<bg2e/ui/Value.hpp>`
**Namespace:** `bg2e::ui`

Static-only non-numeric value editors: text fields, an RGBA color picker and
combo boxes (including a dynamic-list overload). Every method modifies its
value argument **in place** and returns `true` on the frame the value changed
(see the [change-return contract](Numeric.md#the-change-return-contract)).

```cpp
class BG2E_API Value {
public:
    static bool text(const std::string& label, std::string& value, int maxLength = 200, bool sameLine = false);
    static bool textWithHint(const std::string& label, const std::string& hint, std::string& value, int maxLength = 200, bool sameLine = false);

    static bool colorPicker(const std::string& label, base::Color& color, bool sameLine = false);

    static bool comboBox(const std::string& label, const std::vector<std::string>& items, uint32_t& selected, bool sameLine = false, bool fitPreview = false);
    static bool comboBox(const std::string& label, ItemListCallback cb, uint32_t& selected, bool sameLine = false, bool fitPreview = false);

    typedef std::function<void(std::vector<std::string>&)> ItemListCallback;
};
```

---

## Text fields

- `text()` copies the `std::string` into a fixed `char` buffer of `maxLength`;
  typing past `maxLength` is truncated. Only writes back to the string when
  the field actually changed.
- `textWithHint()` shows a placeholder `hint` while the value is empty (the
  standard ImGui "InputTextWithHint"). It requires the `value` string to be
  writable; the call reserves `maxLength` internally but the visible text is
  still bounded by the input width — keep default 200 for normal fields.

## Color picker

`colorPicker()` edits `base::Color` as RGBA (`ColorEdit4`), staging through
a 4-float array.

## Combo boxes

```cpp
static uint32_t preset = 1;
std::vector<std::string> names { "Low", "Medium", "High" };
if (Value::comboBox("Quality", names, preset)) { /* preset changed */ }
```

- The selected index is displayed as `"idx: label"`; `selected` is clamped
  into `[0, items.size()-1]` (empty list clamps to `0`, guard separately).
- `fitPreview` sizes the combo width to the current preview (`##`-suffixed
  labels help keep captions stable).
- **Dynamic list** overload — `ItemListCallback` is
  `std::function<void(std::vector<std::string>&)>`; it refills the items each
  frame, so the list can mirror live data (asset caches, node names) without
  you owning the vector:

```cpp
Value::comboBox("Material", [&](std::vector<std::string>& out) {
    for (auto& m : project.materials()) out.push_back(m.name());
}, selectedMaterial);
```

---

## See also

- [Numeric](Numeric.md) — scalar editors.
- [Vector](Vector.md) — vector and matrix editors.
- [reference.md — Value](reference.md#value)
