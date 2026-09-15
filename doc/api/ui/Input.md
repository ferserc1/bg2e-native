# Input

**Header:** `<bg2e/ui/Input.hpp>`
**Namespace:** `bg2e::ui`

A static-only collection of editable-value widgets: text, numbers, vectors,
colors, sliders, drags, combos, a 4×4 matrix editor, and a file-backed texture
picker helper. Every method modifies its value argument **in place** and
returns `true` on the frame the value changed.

```cpp
class BG2E_API Input {
public:
    static bool text     (const std::string& label, std::string& value, int maxLength = 200, bool sameLine = false);
    static bool textWithHint(const std::string& label, const std::string& hint, std::string& value, int maxLength = 200, bool sameLine = false);

    static bool number(const std::string& label, int*    value, bool sameLine = false);
    static bool number(const std::string& label, float*  value, bool sameLine = false);
    static bool number(const std::string& label, double* value, bool sameLine = false);

    static bool vec2/3/4(const std::string& label, int*   value, bool sameLine = false);
    static bool vec2/3/4(const std::string& label, float* value, bool sameLine = false);
    static bool vec2/3/4(const std::string& label, glm::vec2/3/4& value, bool sameLine = false);

    static bool slider(const std::string& label, int*   value, int   min = 0,   int   max = 100, bool sameLine = false);
    static bool slider(const std::string& label, float* value, float min = 0.f, float max = 1.f, bool sameLine = false);
    static bool sliderInt/Float/Double(...);

    static bool drag(const std::string& label, float* value, float speed = 0.1f, float min = 0.f, float max = 0.f, bool sameLine = false);
    static bool drag(const std::string& label, int*   value, float speed = 1.0f, int   min = 0,   int   max = 0,   bool sameLine = false);

    static bool colorPicker(const std::string& label, base::Color& color, bool sameLine = false);

    static bool comboBox(const std::string& label, const std::vector<std::string>& items, uint32_t& selected, bool sameLine = false, bool fitPreview = false);
    static bool comboBox(const std::string& label, ItemListCallback cb, uint32_t& selected, bool sameLine = false, bool fitPreview = false);

    static bool mat4(const std::string& label, glm::mat4& value, bool sameLine = false);
};
```

---

## The change-return contract

Return `true` means "the value changed **this frame**". Because widgets write
through the argument, the change is already reflected in the variable you
passed; the boolean just tells you *when* to react:

```cpp
static float exposure = 1.0f;
if (Input::slider("Exposure", &exposure, 0.0f, 4.0f)) {
    renderer->setExposure(exposure);   // runs only while dragging
}
```

Pass pointers to persistent storage. A common mistake is editing a temporary:

```cpp
float tmp = light.intensity();          // copy
Input::slider("I", &tmp);               // edits tmp only
// light.intensity() unchanged unless you copy back on the true result
```

---

## Text

- `text()` copies the `std::string` into a fixed `char` buffer of `maxLength`;
  typing past `maxLength` is truncated. Only writes back to the string when
  the field actually changed.
- `textWithHint()` shows a placeholder `hint` while the value is empty (the
  standard ImGui "InputTextWithHint").

---

## Numbers, vectors, colors

- `number` covers `int`/`float`/`double` (ImGui `InputInt/Float/Double` —
  arrow-stepping, editable text).
- `vecN` accepts raw `int*`/`float*` arrays **or** `glm::vecN&`. The GLM
  overloads stage through a fixed C array and copy back only on change.
- `colorPicker()` edits `base::Color` as RGBA (`ColorEdit4`), staging through
  a 4-float array.

---

## Sliders and drags

- `slider` clamps the value to `[min, max]`.
- `drag` follows the ImGui convention: `min == max == 0` means **unclamped**;
  otherwise the value is bounded. `speed` is the per-pixel step.
- `sliderDouble()` is implemented as a `float` drag/slider (reinterpret cast
  internally) — usable but bounded to float precision while editing.

---

## Combo boxes

```cpp
static uint32_t preset = 1;
std::vector<std::string> names { "Low", "Medium", "High" };
if (Input::comboBox("Quality", names, preset)) { /* preset changed */ }
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
Input::comboBox("Material", [&](std::vector<std::string>& out) {
    for (auto& m : project.materials()) out.push_back(m.name());
}, selectedMaterial);
```

---

## Matrix editor: `mat4` and the euler cache

`mat4(label, glm::mat4&)` presents a 4×4 transform as three vec3 rows —
**Position**, **Rotation (degrees)**, **Scale** — recomposing
`T(pos) * R(eulerXYZ) * S(scale)` when any changes. It is the widget used by
the scene transform editors.

Why it is non-trivial: an arbitrary matrix has no stable euler
decomposition, and extracting angles every frame would make the Rotation field
jitter and drift as you drag. `mat4` therefore caches the euler angles
per `label` in a `static std::unordered_map`:

- On first draw, or when `value` differs from the cached matrix (external
  change), it re-extracts position/scale and the euler angles.
- While you edit, it keeps showing the cached angles, so dragging is smooth.
- On commit, it recomposes and stores the new matrix + angles.

Consequences to respect:

1. **`label` is the cache key.** Give each matrix editor a distinct, stable
   label, and don't generate a fresh unique label per frame — that leaks map
   entries forever. For multiple matrices that share a display caption, use
   the `##` suffix (e.g. `"Transform##light1"`).
2. External edits to the matrix (gizmo drag) are picked up automatically on
   the next `mat4()` call because they change `value`.
3. `mat4` ignores `sameLine` (it always starts a new block of three rows).

`NodeEditor` uses the same decomposition for the per-node transform, but keeps
the euler cache **keyed by node pointer** (`_eulerCacheNode` / `_cachedEuler`)
instead of by label, so switching selection re-extracts while staying stable
while editing the same node.

---

## See also

- [quick_start — Recipe 8](quick_start.md#recipe-8-input-widgets-and-the-mat4-trap)
- [BasicWidgets](BasicWidgets.md) — labels, buttons, layout.
- [reference.md — Input](reference.md#input)
