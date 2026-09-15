# Group

**Header:** `<bg2e/ui/Group.hpp>`
**Namespace:** `bg2e::ui`

Static-only scoped helpers that group widgets or modify the state of a group
of subsequent widgets. All members are begin/end or push/pop pairs (except
`collapsingHeader`, which is self-closing).

```cpp
class BG2E_API Group {
public:
    // Collapsible scopes
    static bool beginTree(const std::string& label);
    static void endTree();
    static bool collapsingHeader(const std::string& title, bool visible = true, bool allowOverlap = false);

    // Group enable/disable
    static void beginDisabled(bool disabled = true);
    static void endDisabled();

    // ID stack
    static void pushId(int id);
    static void popId();
};
```

---

## Collapsible scopes

- `collapsingHeader(title, visible, allowOverlap)` is a self-closing section: it returns
  `true` while expanded, and you do **not** pair it with an end call. When
  `visible` is `true` it starts open. Set `allowOverlap` to `true` when you draw
  widgets on top of the header row (via `Layout::sameLine`), so those widgets
  receive mouse clicks instead of the header.
- `beginTree(label)` / `endTree()` are the paired form: if `beginTree` returns
  `true` you **must** call `endTree()` after drawing the children (and skip it
  entirely if it returned `false`).

```cpp
if (Group::collapsingHeader("Transform")) {   // no matching end
    drawTransformFields();
}
```

---

## Group disable

`beginDisabled(flag)` greys out and blocks everything until `endDisabled()`
(unconditional pairing). Reflection widgets and inspectors use it to present
read-only values as visible-but-inactive controls rather than hiding them.
The `disabled` argument of the [`Button`](Button.md) controls wraps a single
control in a `beginDisabled()/endDisabled()` pair.

---

## Identifier management

ImGui keys each widget by its label. Duplicated labels in the same scope
collide (shared state, wrong tooltip target, flicker). Two tools:

1. **`##suffix` convention** — `Button::button("OK##dialog1")` shows `OK` but
   IDs as `dialog1`. This is used throughout the engine (e.g.
   `"UV Set##metallic"`, `"Remove##..."`) so several materials/rows can show
   the same caption.
2. **`pushId(int)` / `popId()`** — prefix a numeric ID onto everything drawn
   between the calls; `ComponentInspector` and `ReflectionWidget` wrap each
   row/property in a `pushId(index)/popId()` pair to disambiguate repeated
   labels inside a list.

---

## See also

- [Layout](Layout.md) — placement helpers.
- [Button](Button.md) — controls commonly wrapped by these scopes.
- [reference.md — Group](reference.md#group)
