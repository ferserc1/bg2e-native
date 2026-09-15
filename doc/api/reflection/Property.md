# Property types

**Header:** `<bg2e/reflection/Property.hpp>`
**Namespace:** `bg2e::reflection`

Defines the value types that describe a single reflected property: the
[`PropertyType`](#propertytype) and [`PropertyEditor`](#propertyeditor) enums,
the [`PropertyMetadata`](#propertymetadata) constraints/display bundle, and the
[`PropertyInfo`](#propertyinfo) struct that ties them to type-erased accessors.

```cpp
enum class PropertyType {
    Bool, Int, UInt, Float, Double, String,
    Vec2, Vec3, Vec4, Mat4,
    Color, Enum, Resource, Path,
    Object
};

enum class PropertyEditor {
    Default, Input, Slider, Drag,
    Checkbox, Combo, Color, Angle
};

struct PropertyMetadata {
    std::string displayName;
    std::string category;
    std::string tooltip;
    std::optional<double> min;
    std::optional<double> max;
    std::optional<double> step;
    std::vector<std::pair<std::string, int64_t>> enumOptions;
};

// Maximum depth of Object-property chains a consumer may recurse into.
// A root reflected instance is depth 0; the targets of its object
// properties are depth 1, and so on. See TypeRegistry::objectChainDepth().
inline constexpr uint32_t maxObjectDepth = 3;

struct PropertyInfo {
    std::string name;
    PropertyType type   = PropertyType::Float;
    PropertyEditor editor = PropertyEditor::Default;
    PropertyMetadata metadata;

    std::function<std::any(const void*)>        getter;
    std::function<void(void*, const std::any&)> setter;

    // Object properties (type == PropertyType::Object) only:
    std::string objectTypeName;                             // TypeRegistry key of the sub-object type
    std::function<const void*(const void*)> objectGetter;   // address of the sub-object (always set)
    std::function<void*(void*)> objectMutableGetter;        // empty => sub-object is read-only

    bool isReadOnly() const
    {
        if (type == PropertyType::Object)
        {
            return !static_cast<bool>(objectMutableGetter);
        }
        return !static_cast<bool>(setter);
    }
};
```

---

## `PropertyType`

The kind of value a property holds. The builder deduces it automatically from the
accessor's value type via [`propertyTypeOf<T>()`](Builder.md#propertytype-of-c-type--propertytype);
you never set it by hand.

| Value | Meaning | Typical C++ type |
|-------|---------|------------------|
| `Bool` | Boolean | `bool` |
| `Int` | Signed integer | `int32_t` |
| `UInt` | Unsigned integer | `uint32_t` |
| `Float` | 32-bit real | `float` |
| `Double` | 64-bit real | `double` |
| `String` | Text | `std::string` |
| `Vec2` / `Vec3` / `Vec4` | GLM vectors | `glm::vec2/3/4` |
| `Mat4` | 4×4 matrix | `glm::mat4` |
| `Color` | RGBA color | `base::Color` |
| `Enum` | Enumeration | any `enum` / `enum class` |
| `Resource` | Reserved for engine resource references | — |
| `Path` | Filesystem path | `std::filesystem::path` |
| `Object` | Reflected sub-object, edited in place | any non-scalar class with its own `TypeInfo` |

`Quat` is intentionally absent (no quaternion accessors in current types). It is
trivial to add by extending `propertyTypeOf<T>()`.

`Object` is **not** deduced by `propertyTypeOf<T>()` — it is never produced from
a C++ accessor type. Object properties are registered exclusively through
[`TypeInfoBuilder<T>::object`](Builder.md#objectname-objecttypename-getters).

### `maxObjectDepth`

```cpp
inline constexpr uint32_t maxObjectDepth = 3;
```

Maximum depth of `Object`-property chains a consumer may recurse into. A root
reflected instance is depth 0; the targets of its object properties are depth 1,
and so on. Consumers must not expand object properties beyond this limit (render
them as plain labels instead). `TypeRegistry::validateObjectDepth()` checks that
every registered type respects it.

### Default

```cpp
PropertyType t = PropertyInfo{}.type;   // PropertyType::Float
```

---

## `PropertyEditor`

A hint about which UI widget *should* edit the value. It is **orthogonal** to
the numeric constraints in `PropertyMetadata`: choosing `Slider` does not impose
a range, and setting a range does not force a slider.

| Value | Intended widget |
|-------|-----------------|
| `Default` | A sensible widget chosen by `PropertyType`. |
| `Input` | Plain numeric / text input. |
| `Slider` | Range slider. |
| `Drag` | Drag-to-adjust scalar. |
| `Checkbox` | Boolean toggle. |
| `Combo` | Dropdown, populated from `metadata.enumOptions`. |
| `Color` | Color picker. |
| `Angle` | Angle editor (degrees). |

The editor is set through the [`PropertyBuilder`](Builder.md#propertybuildert)
shortcuts, not by touching `PropertyInfo::editor` directly. `PropertyEditor` is
not extended for objects: consumers render an object property as a collapsible
group / sub-panel chosen from `PropertyType::Object` itself, so the `editor`
field of an object property always stays `Default` (`ObjectBuilder` makes it
impossible to set).

---

## `PropertyMetadata`

Display and constraint data for a property. Pure data; populated by the
chained `PropertyBuilder` methods.

| Field | Type | Meaning |
|-------|------|---------|
| `displayName` | `std::string` | Human-readable label for UI. |
| `category` | `std::string` | Grouping key (e.g. `"Light"`, `"Spot"`). |
| `tooltip` | `std::string` | Hover help text. |
| `min` | `std::optional<double>` | Lower bound. |
| `max` | `std::optional<double>` | Upper bound. |
| `step` | `std::optional<double>` | Increment for drag/slider/input. |
| `enumOptions` | `vector<pair<string,int64_t>>` | `(label, value)` pairs for `Enum` properties. |

`min`/`max`/`step` are constraints only; they do not affect `editor`.

---

## `PropertyInfo`

A complete reflected property. Instances are produced by
[`TypeInfoBuilder<T>::property`](Builder.md#typeinfobuildert) and read
through [`TypeInfo::property`](TypeInfo.md).

### Members

| Member | Type | Description |
|--------|------|-------------|
| `name` | `std::string` | Programmatic id used by `TypeInfo::property()` lookups and (by convention) matching JSON keys. |
| `type` | `PropertyType` | Deduced value kind. |
| `editor` | `PropertyEditor` | Chosen editor (default `Default`; always `Default` for object properties). |
| `metadata` | `PropertyMetadata` | Display + constraints. |
| `getter` | `std::function<std::any(const void*)>` | Reads the value; instance is a `void*`. Empty for object properties. |
| `setter` | `std::function<void(void*, const std::any&)>` | Writes the value; may be empty for read-only. Empty for object properties. |
| `objectTypeName` | `std::string` | Object properties only: `TypeRegistry` key of the sub-object type, resolved by consumers at runtime. |
| `objectGetter` | `std::function<const void*(const void*)>` | Object properties only: address of the sub-object; always set. |
| `objectMutableGetter` | `std::function<void*(void*)>` | Object properties only: mutable address of the sub-object; empty => the sub-object is read-only. |

For object properties the only meaningful metadata fields are `displayName`,
`category` and `tooltip`: `min`/`max`/`step`, `enumOptions` and `editor` are
unused (and un-settable at compile time through `ObjectBuilder`), and
`getter`/`setter` stay empty — the pointer accessors are the only access path,
since returning the sub-object by `std::any` would copy (and slice) it instead
of allowing in-place editing.

### `bool isReadOnly() const`

Returns `true` when the property has no write path. This is the *only* notion of
read-only — there is no separate flag:

- Scalar properties: read-only without a `setter`.
- Object properties: read-only without an `objectMutableGetter`.

```cpp
const reflection::PropertyInfo* p = info->property("typeString");
p->isReadOnly();   // true when registered with a getter only

const reflection::PropertyInfo* o = info->property("light");   // PropertyType::Object
o->isReadOnly();   // true when registered with a const reference getter only
```

---

## Reading and writing values

The accessors are type-erased. Pass the object as a `void*` (non-const for
writes) and `std::any_cast` the result to the concrete C++ type implied by
`type`.

```cpp
using namespace bg2e;
base::Light light;
void* obj = &light;

const reflection::TypeInfo* info =
    reflection::TypeRegistry::get().type("bg2e::base::Light");

const reflection::PropertyInfo* intensity = info->property("intensity");

float cur = std::any_cast<float>(intensity->getter(obj));      // read
intensity->setter(obj, std::any(cur + 1.0f));                  // write

// Read a const-ref accessor (matrix) -> stored by value:
//   glm::mat4 m = std::any_cast<glm::mat4>(matrixProp->getter(obj));
```

**Notes:**
- Reference accessors are stored by value in the `std::any`, so cast to the bare
  type (`glm::mat4`, not `const glm::mat4&`).
- A wrong cast throws `std::bad_any_cast`; use the pointer overload
  `std::any_cast<T>(&any)` to test safely.
- For enum properties, cast back to the original enum type, not `int64_t`.
- Object properties (`PropertyType::Object`) do not use `getter`/`setter` at
  all: address the sub-object with `objectGetter` / `objectMutableGetter` and
  edit it in place through its own reflected setters (see
  [index — Object properties](index.md#object-properties)).

---

## See also

- [Builder](Builder.md) — how properties are declared (`propertyTypeOf`, traits).
- [TypeInfo](TypeInfo.md) — where `PropertyInfo` lives.
- [quick_start](quick_start.md#recipe-3-readwrite-vs-read-only-properties) —
  practical recipes.
