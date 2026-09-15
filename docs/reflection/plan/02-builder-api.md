# Step 02 — Builder API: `object()` and `ObjectBuilder<T>`

## Goal

Add the registration API for object properties to `Builder.hpp`:

- `TypeInfoBuilder<T>::object(...)` overloads (read-only and editable).
- A dedicated `ObjectBuilder<T>` chained-metadata type that exposes **only**
  `displayName()`, `category()` and `tooltip()` — meaningless operations
  (`slider()`, `range()`, `enumValue()`, …) must not compile.
- A small trait refactor so `object()` can statically reject scalar property
  types.

## Files to modify

```
lib/include/bg2e/reflection/Builder.hpp
```

Header-only, consistent with the rest of the definition API. No new files.

## 1. Trait refactor: scalar detection without duplication

`propertyTypeOf<T>()` currently ends in a `static_assert` for unsupported
types. `object()` needs the opposite check ("must **not** be a scalar
property type") without duplicating the `if constexpr` chain. Refactor the
deduction to go through an internal optional-returning function:

```cpp
namespace detail {

template<typename T>
constexpr std::optional<PropertyType> scalarPropertyTypeOf()
{
    using U = ValueT<T>;
    if constexpr (std::is_same_v<U, bool>)                  return PropertyType::Bool;
    else if constexpr (std::is_same_v<U, int32_t>)          return PropertyType::Int;
    else if constexpr (std::is_same_v<U, uint32_t>)         return PropertyType::UInt;
    else if constexpr (std::is_same_v<U, float>)            return PropertyType::Float;
    else if constexpr (std::is_same_v<U, double>)           return PropertyType::Double;
    else if constexpr (std::is_same_v<U, std::string>)      return PropertyType::String;
    else if constexpr (std::is_same_v<U, glm::vec2>)        return PropertyType::Vec2;
    else if constexpr (std::is_same_v<U, glm::vec3>)        return PropertyType::Vec3;
    else if constexpr (std::is_same_v<U, glm::vec4>)        return PropertyType::Vec4;
    else if constexpr (std::is_same_v<U, glm::mat4>)        return PropertyType::Mat4;
    else if constexpr (std::is_same_v<U, base::Color>)      return PropertyType::Color;
    else if constexpr (std::is_same_v<U, std::filesystem::path>) return PropertyType::Path;
    else if constexpr (std::is_enum_v<U>)                   return PropertyType::Enum;
    else                                                    return std::nullopt;
}

} // namespace detail

template<typename T>
constexpr bool isScalarPropertyType()
{
    return detail::scalarPropertyTypeOf<T>().has_value();
}

template<typename T>
constexpr PropertyType propertyTypeOf()
{
    constexpr auto type = detail::scalarPropertyTypeOf<T>();
    static_assert(type.has_value(), "bg2e::reflection: unsupported property type");
    return *type;
}
```

This preserves the existing `propertyTypeOf` behavior (including the
`static_assert`) exactly; it only makes the deduction reusable. Add
`<optional>` to the includes.

## 2. `ObjectBuilder<T>`

A separate chained-metadata type, declared next to `PropertyBuilder<T>`. It
deliberately exposes only the three metadata operations that make sense for
an object property:

```cpp
template<typename T> class TypeInfoBuilder;

template<typename T>
class ObjectBuilder {
public:
    ObjectBuilder(TypeInfoBuilder<T>& builder, size_t index)
        : _builder(builder), _index(index) {}

    ObjectBuilder& displayName(std::string v) { info().metadata.displayName = std::move(v); return *this; }
    ObjectBuilder& category(std::string v)    { info().metadata.category = std::move(v); return *this; }
    ObjectBuilder& tooltip(std::string v)     { info().metadata.tooltip = std::move(v); return *this; }

    // No editor(), slider(), drag(), range(), min(), max(), step(),
    // enumValue(), ... : they are meaningless for object properties and
    // must fail to compile instead of being silently ignored.

private:
    PropertyInfo& info() { return _builder.propertyAt(_index); }

    TypeInfoBuilder<T>& _builder;
    size_t _index;
};
```

## 3. `TypeInfoBuilder<T>::object(...)` overloads

Two overloads, mirroring the getter-only / getter+setter split of
`property()`:

```cpp
// Read-only object property: const reference getter only.
// Getter signature: const U& (T::*)() const
template<typename Getter>
ObjectBuilder<T> object(std::string name, std::string objectTypeName, Getter getter)
{
    using U = typename GetterTraits<T, Getter>::ValueType;
    static_assert(std::is_class_v<U> && !isScalarPropertyType<U>(),
        "bg2e::reflection: object() requires a non-scalar class type; use property() for scalars");

    PropertyInfo p;
    p.name = std::move(name);
    p.type = PropertyType::Object;
    p.objectTypeName = std::move(objectTypeName);
    p.objectGetter = [getter](const void * instance) -> const void * {
        auto object = static_cast<const T*>(instance);
        return static_cast<const void*>(&(object->*getter)());
    };
    _info.properties.push_back(std::move(p));
    return ObjectBuilder<T>(*this, _info.properties.size() - 1);
}

// Editable object property: const + mutable reference getters.
// Signatures: const U& (T::*)() const  and  U& (T::*)()
template<typename ConstGetter, typename MutableGetter>
ObjectBuilder<T> object(std::string name, std::string objectTypeName,
                        ConstGetter constGetter, MutableGetter mutableGetter)
{
    using U = typename GetterTraits<T, ConstGetter>::ValueType;
    using MU = typename GetterTraits<T, MutableGetter>::ValueType;
    static_assert(std::is_same_v<U, MU>,
        "bg2e::reflection: const and mutable object getters must return the same type");
    static_assert(std::is_class_v<U> && !isScalarPropertyType<U>(),
        "bg2e::reflection: object() requires a non-scalar class type; use property() for scalars");

    PropertyInfo p;
    p.name = std::move(name);
    p.type = PropertyType::Object;
    p.objectTypeName = std::move(objectTypeName);
    p.objectGetter = [constGetter](const void * instance) -> const void * {
        auto object = static_cast<const T*>(instance);
        return static_cast<const void*>(&(object->*constGetter)());
    };
    p.objectMutableGetter = [mutableGetter](void * instance) -> void * {
        auto object = static_cast<T*>(instance);
        return static_cast<void*>(&(object->*mutableGetter)());
    };
    _info.properties.push_back(std::move(p));
    return ObjectBuilder<T>(*this, _info.properties.size() - 1);
}
```

Notes:

- The existing `GetterTraits` specializations (`R (Class::*)() const` and
  `R (Class::*)()`) already cover reference-returning getters, and `ValueT`
  strips the reference, so no new traits are needed.
- The lambdas take the **address** of the returned reference; the
  sub-object must be a by-value member (or otherwise outlive the parent).
  By design there is no overload accepting a value-returning getter: the
  reference requirement falls out of the `&(object->*getter)()` expression
  itself (a value return would fail to compile).
- `PropertyBuilder<T>` and `ActionBuilder<T>` are untouched. `property()`
  still `static_assert`s on unsupported class types via `propertyTypeOf`;
  `object()` asserts the opposite. Nothing is silently reclassified.

## 4. Overload disambiguation (document, do not "fix")

`scene::LightComponent` has two `light()` overloads (const and non-const),
so `&scene::LightComponent::light` is ambiguous. Callers disambiguate with
`static_cast`:

```cpp
t.object("light", "bg2e::base::Light",
         static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
         static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light));
```

This is the standard idiom for overloaded member functions and matches how
the two-getter overload is meant to be used; document it in
`doc/api/reflection/Builder.md` (step 04) with this exact example. Do not
add helper machinery to hide the cast.

## 5. Negative check (compile-failure verification)

After implementing, temporarily paste the following into an existing
definition `.cpp` and confirm the build **fails**:

```cpp
t.object("bad", "bg2e::base::Light",
         static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light))
    .slider();          // must NOT compile: no slider() on ObjectBuilder
```

Also confirm the mirror-image misuse fails:

```cpp
t.property("bad", &scene::LightComponent::light);  // must NOT compile: unsupported property type
```

Remove the snippets afterwards. This is a one-time manual check; there is no
test framework to encode it in.

## Acceptance criteria

- `cmake` re-configure + `cmake --build build` succeeds with the existing
  definitions unchanged.
- The two negative-check snippets above fail to compile, each with the
  expected error origin (`ObjectBuilder` has no `slider`; `propertyTypeOf`
  static_assert).
