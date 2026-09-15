# Reflection API Reference

Class, struct, and enum reference for the `bg2e::reflection` namespace.

Every symbol lives under `bg2e::reflection`. The umbrella header
`<bg2e/reflection/all.hpp>` includes the whole module.

---

## Enums

| Enum | Header | Description |
|------|--------|-------------|
| [PropertyType](Property.md#propertytype) | `reflection/Property.hpp` | Kind of a reflected value: `Bool`, `Int`, `UInt`, `Float`, `Double`, `String`, `Vec2`, `Vec3`, `Vec4`, `Mat4`, `Color`, `Enum`, `Resource`, `Path`, `Object`. |
| [PropertyEditor](Property.md#propertyeditor) | `reflection/Property.hpp` | UI editor hint (orthogonal to constraints): `Default`, `Input`, `Slider`, `Drag`, `Checkbox`, `Combo`, `Color`, `Angle`. Not extended for `Object` properties. |

## Constants

| Constant | Header | Description |
|----------|--------|-------------|
| [maxObjectDepth](Property.md#maxobjectdepth) | `reflection/Property.hpp` | `inline constexpr uint32_t` = 3. Maximum depth of `Object`-property chains a consumer may recurse into (root instance = depth 0). Checked by `TypeRegistry::validateObjectDepth()`. |

## Structs

| Struct | Header | Description |
|--------|--------|-------------|
| [PropertyMetadata](Property.md#propertymetadata) | `reflection/Property.hpp` | UI metadata for a property: `displayName`, `category`, `tooltip`, `min`/`max`/`step`, and `enumOptions`. |
| [PropertyInfo](Property.md#propertyinfo) | `reflection/Property.hpp` | A single reflected property: name, type, editor, metadata, and type-erased accessors: `getter`/`setter` for scalars; `objectTypeName`/`objectGetter`/`objectMutableGetter` for `Object` properties. `isReadOnly()` is `!setter` for scalars, `!objectMutableGetter` for objects. |
| [ActionInfo](Action.md) | `reflection/Action.hpp` | A single reflected action: `name`, `displayName`, `category`, `tooltip`, and a parameterless `invoke` (`void(void*)`). |
| [TypeInfo](TypeInfo.md) | `reflection/TypeInfo.hpp` | Reflected description of a type: `typeName`, `displayName`, and vectors of `PropertyInfo` / `ActionInfo`, with `property(name)` and `action(name)` lookups. |

## Classes

| Class | Header | Description |
|-------|--------|-------------|
| [TypeRegistry](TypeRegistry.md) | `reflection/Registry.hpp` | Leaky-singleton registry keyed by type-name string. `BG2E_API`. `registerType`, `type`, `contains`, `typeNames`, `objectChainDepth`, `validateObjectDepth`. |
| [TypeInfoBuilder\<T\>](Builder.md#typeinfobuildert) | `reflection/Builder.hpp` | Header-only chained builder that assembles a `TypeInfo` from member-fn-pointer accessors. `property()`, `object()`, `action()`. |
| [PropertyBuilder\<T\>](Builder.md#propertybuildert) | `reflection/Builder.hpp` | Chained metadata builder returned by `TypeInfoBuilder<T>::property`. Sets display/category/tooltip, range/step, editor, enum options. |
| [ObjectBuilder\<T\>](Builder.md#objectbuildert) | `reflection/Builder.hpp` | Chained metadata builder returned by `TypeInfoBuilder<T>::object`. Sets display/category/tooltip **only** — editor/constraint/enum operations do not exist on this type (compile error by design). |
| [ActionBuilder\<T\>](Builder.md#actionbuildert) | `reflection/Builder.hpp` | Chained metadata builder returned by `TypeInfoBuilder<T>::action`. Sets display/category/tooltip. |
| [TypeRegistration\<T\>](Registration.md) | `reflection/Registration.hpp` | Header-only static-init helper. Builds a `TypeInfoBuilder<T>` and registers it. Two constructors: `staticTypeName()` key or explicit key. |

## Traits and type deduction

| Item | Header | Description |
|------|--------|-------------|
| [ValueT\<T\>](Builder.md#value-normalization) | `reflection/Builder.hpp` | `std::remove_cv_t<std::remove_reference_t<T>>` — collapses `T`, `const T`, `T&`, `const T&` to `T`. |
| [GetterTraits\<Class, MemberFn\>](Builder.md#accessor-traits) | `reflection/Builder.hpp` | Extracts the value type from `R (C::*)() const` or `R (C::*)()`. |
| [SetterTraits\<Class, MemberFn\>](Builder.md#accessor-traits) | `reflection/Builder.hpp` | Extracts the value type from `void (C::*)(V)`. |
| [propertyTypeOf\<T\>()](Builder.md#propertytype-of-c-type--propertytype) | `reflection/Builder.hpp` | `constexpr PropertyType` deduction from a C++ type; `static_assert`s on unsupported types. |
| [isScalarPropertyType\<T\>()](Builder.md#propertytype-of-c-type--propertytype) | `reflection/Builder.hpp` | `constexpr bool`: whether `ValueT<T>` is a scalar property type (the `object()` static-assert uses it to reject scalars). |

---

## Type-erased accessor signatures

These are the concrete types stored in `PropertyInfo` and `ActionInfo`:

```cpp
struct PropertyInfo {
    std::function<std::any(const void*)>        getter;  // instance as void* -> value as std::any
    std::function<void(void*, const std::any&)> setter;  // instance as void*, value as std::any

    // Object properties (type == PropertyType::Object) only; getter/setter
    // stay empty for them:
    std::string objectTypeName;                             // TypeRegistry key of the sub-object type
    std::function<const void*(const void*)> objectGetter;   // address of the sub-object (always set)
    std::function<void*(void*)> objectMutableGetter;        // empty => sub-object is read-only
};

struct ActionInfo {
    std::function<void(void*)> invoke;                    // parameterless; return value discarded
};
```

## Accessor signatures accepted by the builders

The template `property(...)` / `object(...)` / `action(...)` overloads accept
only member function pointers with these shapes:

| Kind | Accepted signatures | Notes |
|------|--------------------|-------|
| Getter | `R (T::*)() const`, `R (T::*)()` with `R ∈ {V, const V&}` | Virtual getters work unchanged. |
| Setter | `void (T::*)(V)` with `V ∈ {U, const U&}` | Must resolve to the same `ValueT` as the getter. |
| Object getter | `const U& (T::*)() const` (read-only) and `U& (T::*)()` (editable) | `U` must be a non-scalar class. Overloaded getters need `static_cast` disambiguation. |
| Action | `R (T::*)()`, `R (T::*)() const` (any `R`) | Result is discarded; fluent `T*` methods are valid. |

## `PropertyType` ↔ C++ type map

`propertyTypeOf<T>()` maps the normalized value type (`ValueT<T>`) as follows:

| `PropertyType` | C++ type |
|----------------|----------|
| `Bool` | `bool` |
| `Int` | `int32_t` |
| `UInt` | `uint32_t` |
| `Float` | `float` |
| `Double` | `double` |
| `String` | `std::string` |
| `Vec2` / `Vec3` / `Vec4` | `glm::vec2` / `glm::vec3` / `glm::vec4` |
| `Mat4` | `glm::mat4` |
| `Color` | `base::Color` |
| `Path` | `std::filesystem::path` |
| `Enum` | any enumeration type (`std::is_enum_v`) |
| `Resource` | reserved (engine resource references) |
| `Object` | not deduced from a C++ type; registered explicitly via `object()` |

Any other type triggers a compile-time error
(`bg2e::reflection: unsupported property type`).

---

## Header catalog

| Header | Contents |
|--------|----------|
| `bg2e/reflection/Property.hpp` | `PropertyType`, `PropertyEditor`, `PropertyMetadata`, `PropertyInfo`, `maxObjectDepth` |
| `bg2e/reflection/Action.hpp` | `ActionInfo` |
| `bg2e/reflection/TypeInfo.hpp` | `TypeInfo` |
| `bg2e/reflection/Registry.hpp` | `TypeRegistry` |
| `bg2e/reflection/Builder.hpp` | accessor traits, `propertyTypeOf`, `isScalarPropertyType`, `PropertyBuilder`, `ObjectBuilder`, `ActionBuilder`, `TypeInfoBuilder` |
| `bg2e/reflection/Registration.hpp` | `TypeRegistration` |
| `bg2e/reflection/all.hpp` | umbrella (includes all of the above) |
