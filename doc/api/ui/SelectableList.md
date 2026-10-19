# SelectableList

**Header:** `<bg2e/ui/SelectableList.hpp>`
**Namespace:** `bg2e::ui`

A small static helper that renders a bordered table whose rows are selectable,
with an optional trailing button column — the widget behind `SubmeshSelector`.

```cpp
class BG2E_API SelectableList {
public:
    static bool beginList(int columns = 1);
    static bool item(const std::string& title, bool& selected);   // true on click
    static bool itemButton(const std::string& title);             // true on click
    static void endList();
};
```

- `beginList(columns)` opens an ImGui table (settings intentionally not saved
  across sessions). It returns whether the table was opened. Only submit items
  and call `endList()` inside `if (SelectableList::beginList(columns))`.
  A clipped/skipped table can return false; do not call `endList()` in that case.
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

- [reference.md — SelectableList](reference.md#selectablelist)
- [Material & Drawable editors](Material_Editors.md) — `SubmeshSelector` usage.
