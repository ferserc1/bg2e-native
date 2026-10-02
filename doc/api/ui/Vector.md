# Vector

**Header:** `<bg2e/ui/Vector.hpp>`
**Namespace:** `bg2e::ui`

Static-only compound math editors: `vec2`/`vec3`/`vec4` input, slider and drag
component rows (raw arrays or GLM vectors), plus a decomposed 4×4 transform
editor. Every method modifies its value argument **in place** and returns
`true` on the frame the value changed (see the
[change-return contract](Numeric.md#the-change-return-contract)).

```cpp
class BG2E_API Vector {
public:
    static bool vec2/3/4(const std::string& label, int*   value, bool sameLine = false);
    static bool vec2/3/4(const std::string& label, float* value, bool sameLine = false);
    static bool vec2/3/4(const std::string& label, glm::vec2/3/4& value, bool sameLine = false);

    static bool sliderVec2/3/4(label, float* or glm::vecN& value,
                               float min, float max, bool sameLine = false);
    static bool dragVec2/3/4(label, float* or glm::vecN& value,
                             float speed = 0.1f, float min = 0.0f,
                             float max = 0.0f, bool sameLine = false);

    static bool mat4(const std::string& label, glm::mat4& value, bool sameLine = false);
};
```

---

## Vectors

- `vecN` accepts raw `int*`/`float*` arrays **or** `glm::vecN&`. The GLM
  overloads stage through a fixed C array and copy back only on change.
- `sliderVecN` and `dragVecN` accept raw `float*` arrays or `glm::vecN&` and
  apply the same range and drag speed to every component.
- Slider and drag components support Ctrl-click keyboard entry. The wrapper
  does not force clamping of values entered this way.
- With the GLM overloads the component order in the widget is X,Y,(Z,(W)).

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

The generic `NodeEditor` now delegates transform editing to reflected
translation, rotation, and scale properties through `ComponentInspector`; it
does not use this matrix widget. `Vector::mat4` remains available to specialized
editors that need decomposed matrix editing.

---

## See also

- [quick_start — Recipe 8](quick_start.md#recipe-8-value-editors-and-the-mat4-trap)
- [Numeric](Numeric.md) — scalar editors.
- [Value](Value.md) — text, color and combo editors.
- [reference.md — Vector](reference.md#vector)
