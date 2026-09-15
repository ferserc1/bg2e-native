# Builder

**Header:** `<bg2e/reflection/Builder.hpp>`
**Namespace:** `bg2e::reflection`

The ergonomic, macro-free definition API. Header-only templates that turn
member-function-pointer accessors into a [`TypeInfo`](TypeInfo.md). Includes the
accessor traits, the `propertyTypeOf<T>()` deduction, and the five builder
classes: [`TypeInfoBuilder<T>`](#typeinfobuildert),
[`PropertyBuilder<T>`](#propertybuildert),
[`ObjectBuilder<T>`](#objectbuildert),
[`PolymorphicObjectBuilder<T>`](#polymorphicobjectbuildert), and
[`ActionBuilder<T>`](#actionbuildert).

---

## Value normalization

```cpp
template<typename T>
using ValueT = std::remove_cv_t<std::remove_reference_t<T>>;
```

Collapses `T`, `const T`, `T&`, `const T&` to `T`. Accessor value types are
normalized before comparison and before `propertyTypeOf<T>()`.

---

## Accessor traits

The traits recognize the idiomatic accessor signatures found in the repo. Only
member function pointers are supported.

```cpp
template<typename Class, typename MemberFn> struct GetterTraits;   // partial specs below
template<typename Class, typename MemberFn> struct SetterTraits;

// GetterTraits specializations:
//   R (Class::*)() const  -> ValueType = ValueT<R>
//   R (Class::*)()         -> ValueType = ValueT<R>

// SetterTraits specialization:
//   R (Class::*)(V)        -> ValueType = ValueT<V>
```

| Kind | Accepted signatures | Example |
|------|--------------------|---------|
| Getter | `R (T::*)() const`, `R (T::*)()` with `R ∈ {V, const V&}` | `const glm::mat4& matrix() const` |
| Setter | `R (T::*)(V)` with `V ∈ {U, const U&}`; any return type | `void setMatrix(const glm::mat4&)` |

Virtual accessors work unchanged (member pointers to virtuals dispatch
dynamically). Free functions and lambdas are intentionally excluded in v1.

If a getter and a setter resolve to different normalized value types,
`property(...)` fires a `static_assert`:
`bg2e::reflection: getter and setter must use the same value type`.

---

## `PropertyType` of C++ type → `PropertyType`

```cpp
template<typename T>
constexpr PropertyType propertyTypeOf();
```

Maps `ValueT<T>` to a [`PropertyType`](Property.md#propertytype):

| C++ type | `PropertyType` |
|----------|----------------|
| `bool` | `Bool` |
| `int32_t` | `Int` |
| `uint32_t` | `UInt` |
| `float` | `Float` |
| `double` | `Double` |
| `std::string` | `String` |
| `glm::vec2` / `glm::vec3` / `glm::vec4` | `Vec2` / `Vec3` / `Vec4` |
| `glm::mat4` | `Mat4` |
| `base::Color` | `Color` |
| `std::filesystem::path` | `Path` |
| any enum (`std::is_enum_v`) | `Enum` |

Any other type triggers a compile-time error:
`bg2e::reflection: unsupported property type`. This is a dependent
`static_assert`, so it only fires when the type is actually used.

The deduction goes through an internal `detail::scalarPropertyTypeOf<T>()` that
returns `std::optional<PropertyType>`, on top of which
`isScalarPropertyType<T>()` is defined:

```cpp
template<typename T>
constexpr bool isScalarPropertyType();
```

`object()` uses it to reject scalar types (see
[`object(name, objectTypeName, ...)`](#objectname-objecttypename-getters)):
`property()` static-asserts the type *is* scalar, `object()` asserts it is a
class and *not* scalar. Nothing is silently reclassified.

---

## `TypeInfoBuilder<T>`

```cpp
template<typename T>
class TypeInfoBuilder {
public:
    explicit TypeInfoBuilder(std::string typeName);

    TypeInfoBuilder& displayName(std::string name);

    template<typename Getter, typename Setter>
    PropertyBuilder<T> property(std::string name, Getter getter, Setter setter);

    template<typename Getter>
    PropertyBuilder<T> property(std::string name, Getter getter);

    template<typename Getter>
    ObjectBuilder<T> object(std::string name, std::string objectTypeName, Getter getter);

    template<typename ConstGetter, typename MutableGetter>
    ObjectBuilder<T> object(std::string name, std::string objectTypeName,
                            ConstGetter constGetter, MutableGetter mutableGetter);

    template<typename Base, typename ConstGetter, typename MutableGetter, typename Replacer>
    PolymorphicObjectBuilder<T> polymorphicObject(std::string name,
        std::string baseTypeName, ConstGetter, MutableGetter, Replacer);

    template<typename Base, typename ConstGetter>
    PolymorphicObjectBuilder<T> polymorphicObject(std::string name,
        std::string baseTypeName, ConstGetter);

    template<typename Method>
        requires std::is_member_function_pointer_v<Method>
    ActionBuilder<T> action(std::string name, Method method);

    template<typename F>
        requires (std::is_invocable_v<F, T*> && !std::is_member_pointer_v<F>)
    ActionBuilder<T> action(std::string name, F fn);

    const TypeInfo& build() const;

    PropertyInfo& propertyAt(size_t index);   // used by PropertyBuilder / ObjectBuilder
    ActionInfo&   actionAt(size_t index);     // used by ActionBuilder
};
```

Assembles a [`TypeInfo`](TypeInfo.md) for the type `T`. You typically do not
instantiate it directly — a [`TypeRegistration<T>`](Registration.md) supplies it
to your `define` lambda.

### `TypeInfoBuilder(std::string typeName)`

Sets the registry key (`_info.typeName`). `TypeRegistration` passes either
`T::staticTypeName()` or the explicit key you provide.

### `TypeInfoBuilder& displayName(std::string name)`

Sets the human-readable name for the type.

### `property(name, getter, setter)`

Declares a **read/write** property. Deduces the value type from both accessors,
static-asserts they match, sets `type = propertyTypeOf<value>()`, and stores
type-erased `getter`/`setter` closures that cast the `void*` to `T*` and call
the member pointer. The value is stored in the returned `std::any` by value
(even for `const&` accessors). Returns a [`PropertyBuilder<T>`](#propertybuildert)
positioned at the new element.

For enum properties, the erased getter returns `int64_t` and the setter accepts
`int64_t`; conversion to the concrete enum happens inside these closures. This
makes enum values editable without knowing their C++ type at runtime.

### `property(name, getter)`

Declares a **read-only** property (getter only, so `setter` stays empty and
`isReadOnly()` is `true`). Same deduction otherwise.

### `object(name, objectTypeName, getters)`

Declares a property whose value is a **sub-object of another reflected type**
(`PropertyType::Object`), edited in place through the sub-object's own
reflection metadata. Two overloads mirror the getter-only / getter+setter split
of `property()`:

```cpp
// Read-only object property: const reference getter only.
// Getter signature: const U& (T::*)() const
template<typename Getter>
ObjectBuilder<T> object(std::string name, std::string objectTypeName, Getter getter);

// Editable object property: const + mutable reference getters.
// Signatures: const U& (T::*)() const  and  U& (T::*)()
template<typename ConstGetter, typename MutableGetter>
ObjectBuilder<T> object(std::string name, std::string objectTypeName,
                        ConstGetter constGetter, MutableGetter mutableGetter);
```

`objectTypeName` is the `TypeRegistry` key of the sub-object type (e.g.
`"bg2e::base::Light"`). It is stored as-is and resolved by consumers at
runtime, so registration order between translation units does not matter.
The overloads take the **address** of the reference returned by the getter:
the sub-object must be a by-value member (or otherwise outlive the parent).
There is deliberately no value-returning getter overload — taking the address
of a returned value would not compile.

**Static-assert contract:** the getter's value type must be a class and must
**not** be a scalar property type (`isScalarPropertyType<U>() == false`), or:
`bg2e::reflection: object() requires a non-scalar class type; use property()
for scalars`. Scalar types must keep using `property()`. The two-getter
overload additionally asserts both getters return the same type.

**Overloaded getters:** when the const and non-const overloads of an accessor
share a name (as in `scene::LightComponent::light()`), `&T::light` is
ambiguous and callers disambiguate with `static_cast` — the standard idiom
for overloaded member functions:

```cpp
t.object("light", "bg2e::base::Light",
         static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
         static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light));
```

Returns an [`ObjectBuilder<T>`](#objectbuildert) positioned at the new element.

### `polymorphicObject<Base>(...)`

Declares an owned polymorphic object. The editable overload accepts const and
mutable getters returning `const Base*` / `Base*` (or compatible callables),
plus a replacer callable that accepts the owner and `std::shared_ptr<Base>`.
The getter-only overload is read-only. The active concrete type and replacement
instances are resolved through the hierarchy registered in `TypeRegistry`.
The returned builder can restrict the selector to an ordered set of subtype keys.

### `action(name, method)` / `action(name, fn)`

Declares a parameterless [`ActionInfo`](Action.md). Two constrained overloads:

- **Member function pointer** (`requires std::is_member_function_pointer_v<Method>`):
  wraps the member pointer in a `std::function<void(void*)>` that discards the
  return value, so fluent methods (`T*`-returning) work.
- **Callable** (`requires std::is_invocable_v<F, T*> && !std::is_member_pointer_v<F>`):
  wraps any lambda or functor invocable with `T*`. The closure receives the
  component instance, so complex actions that do not belong in the component
  itself (e.g. opening a file dialog) can live in the reflection registration
  code. The `is_member_pointer_v` exclusion is required because member pointers
  are also invocable with `T*` via `std::invoke` and would otherwise be
  ambiguous.

```cpp
t.action("setIdentity", &scene::TransformComponent::setIdentity);

t.action("resetAndNotify", [](scene::TransformComponent* comp) {
    comp->setIdentity();
    // ... arbitrary logic outside the component ...
});
```

Both return an [`ActionBuilder<T>`](#actionbuildert).

### `const TypeInfo& build() const`

Returns the assembled record. `TypeRegistration` passes it to
`TypeRegistry::registerType`.

### `propertyAt` / `actionAt`

Index accessors used internally by the chained builders so they mutate the right
element without holding a dangling reference into the vector (the vectors can
reallocate on the next `property()`/`action()` call).

---

## `PropertyBuilder<T>`

```cpp
template<typename T>
class PropertyBuilder {
public:
    PropertyBuilder& displayName(std::string v);
    PropertyBuilder& category(std::string v);
    PropertyBuilder& tooltip(std::string v);

    PropertyBuilder& range(double minV, double maxV);
    PropertyBuilder& min(double v);
    PropertyBuilder& max(double v);
    PropertyBuilder& step(double v);

    PropertyBuilder& editor(PropertyEditor e);
    PropertyBuilder& input();
    PropertyBuilder& slider();
    PropertyBuilder& drag();
    PropertyBuilder& checkbox();
    PropertyBuilder& combo();
    PropertyBuilder& colorEditor();
    PropertyBuilder& angle();

    PropertyBuilder& enumValue(std::string label, int64_t value);
    PropertyBuilder& resource(std::string kind,
        std::vector<std::string> extensions = {}, bool projectRelative = false);
    template<typename EnumT>
    PropertyBuilder& enumValue(std::string label, EnumT value);
};
```

Returned by `TypeInfoBuilder<T>::property`. Every method returns `*this` for
chaining and writes into the property it points at (builder + index).

**Constraints (metadata only):**
`range`, `min`, `max`, `step` set `metadata.min/max/step`. They never touch the
editor.

**Editor choice (orthogonal to constraints):**
`input`, `slider`, `drag`, `checkbox`, `combo`, `colorEditor`, `angle` set
`editor`. `editor(PropertyEditor)` sets it explicitly. `colorEditor()` is the
method name to avoid clashing with the `Color` type.

**Enum options:**
`enumValue(label, value)` appends to `metadata.enumOptions`. The templated
overload `static_cast`s any enum to `int64_t`, so both plain and scoped enums
work.

**Resources:** `resource()` promotes a `std::string` or
`std::filesystem::path` property to `PropertyType::Resource`, records its
original storage type and file-picker metadata, and throws `std::logic_error`
if used on any other property type.

```cpp
t.property("type", &base::Light::type, &base::Light::setType)
    .displayName("Type")
    .combo()
    .enumValue("Omni", base::Light::TypeOmni);
```

---

## `ObjectBuilder<T>`

```cpp
template<typename T>
class ObjectBuilder {
public:
    ObjectBuilder& displayName(std::string v);
    ObjectBuilder& category(std::string v);
    ObjectBuilder& tooltip(std::string v);
};
```

Returned by `TypeInfoBuilder<T>::object`. A **separate** chained-metadata type
that exposes only the three operations that make sense for an object property.
There is deliberately no `editor()`, `slider()`, `drag()`, `range()`, `min()`,
`max()`, `step()`, or `enumValue()` on this type: those operations do not
exist, so meaningless calls are a **compile error by design** instead of being
silently ignored:

```cpp
// Does NOT compile: ObjectBuilder<T> has no slider()
t.object("light", "bg2e::base::Light", ...).slider();
```

```cpp
t.object("light", "bg2e::base::Light", ...)
    .displayName("Light")
    .category("Light")
    .tooltip("Light parameters, edited in place");
```

---

## `PolymorphicObjectBuilder<T>`

```cpp
template<typename T>
class PolymorphicObjectBuilder {
public:
    PolymorphicObjectBuilder& displayName(std::string v);
    PolymorphicObjectBuilder& category(std::string v);
    PolymorphicObjectBuilder& tooltip(std::string v);
    PolymorphicObjectBuilder& subtype(std::string key);
};
```

Returned by `TypeInfoBuilder<T>::polymorphicObject`. `subtype(key)` appends to
the ordered allowlist for that property. If no keys are appended, consumers
expose every subtype registered for the base hierarchy.

## `ActionBuilder<T>`

```cpp
template<typename T>
class ActionBuilder {
public:
    ActionBuilder& displayName(std::string v);
    ActionBuilder& category(std::string v);
    ActionBuilder& tooltip(std::string v);
};
```

Returned by `TypeInfoBuilder<T>::action`. Chains display metadata onto the action
it points at (builder + index). No editor or constraints — those are property
concerns.

```cpp
t.action("setIdentity", &scene::TransformComponent::setIdentity)
    .displayName("Set Identity")
    .category("Transform");
```

---

## Memory-safety detail

`PropertyBuilder`, `ObjectBuilder`, `PolymorphicObjectBuilder` and
`ActionBuilder` hold a **reference to the
owning `TypeInfoBuilder` plus an element index**, not a reference to the
`PropertyInfo` / `ActionInfo` inside the vector. This avoids a dangling
reference if the next `.property()` / `.object()` / `.action()` call grows the
vector and triggers a reallocation. Every chained method re-fetches the element
by index via `propertyAt` / `actionAt`.

---

## See also

- [Registration](Registration.md) — wraps this builder for static-init.
- [Property](Property.md) / [Action](Action.md) — produced metadata types.
- [quick_start](quick_start.md) — full worked examples.
