# Numeric

**Header:** `<bg2e/ui/Numeric.hpp>`
**Namespace:** `bg2e::ui`

Static-only scalar numeric value editors: stepped number inputs, sliders and
drags for `int`, `float` and `double`. Every method modifies its value
argument **in place** and returns `true` on the frame the value changed.

```cpp
class BG2E_API Numeric {
public:
    static bool number(const std::string& label, int*    value, bool sameLine = false);
    static bool number(const std::string& label, float*  value, bool sameLine = false);
    static bool number(const std::string& label, double* value, bool sameLine = false);

    static bool slider(const std::string& label, int*   value, int   min = 0,   int   max = 100, bool sameLine = false);
    static bool slider(const std::string& label, float* value, float min = 0.f, float max = 1.f, bool sameLine = false);
    static bool sliderInt/Float/Double(...);

    static bool drag(const std::string& label, float* value, float speed = 0.1f, float min = 0.f, float max = 0.f, bool sameLine = false);
    static bool drag(const std::string& label, int*   value, float speed = 1.0f, int   min = 0,   int   max = 0,   bool sameLine = false);
};
```

---

## The change-return contract

Return `true` means "the value changed **this frame**". Because widgets write
through the argument, the change is already reflected in the variable you
passed; the boolean just tells you *when* to react:

```cpp
static float exposure = 1.0f;
if (Numeric::slider("Exposure", &exposure, 0.0f, 4.0f)) {
    renderer->setExposure(exposure);   // runs only while dragging
}
```

Pass pointers to persistent storage. A common mistake is editing a temporary:

```cpp
float tmp = light.intensity();          // copy
Numeric::slider("I", &tmp);             // edits tmp only
// light.intensity() unchanged unless you copy back on the true result
```

---

## Number inputs

`number` covers `int`/`float`/`double` (ImGui `InputInt/Float/Double` —
arrow-stepping, editable text).

## Sliders and drags

- `slider` clamps the value to `[min, max]`.
- `drag` follows the ImGui convention: `min == max == 0` means **unclamped**;
  otherwise the value is bounded. `speed` is the per-pixel step.
- `sliderDouble()` is implemented as a `float` slider (reinterpret cast
  internally) — usable but bounded to float precision while editing.

---

## See also

- [Vector](Vector.md) — vec2/3/4 and mat4 editors.
- [Value](Value.md) — text fields, color picker and combo boxes.
- [reference.md — Numeric](reference.md#numeric)
