# Step 01 — Core types: `PropertyType::Object` and depth limit

## Goal

Extend the plain data types so a property can describe a sub-object of
another reflected type. No behavior is added beyond the updated
`isReadOnly()` rule.

## Files to modify

```
lib/include/bg2e/reflection/Property.hpp
```

No new files in this step. No other existing file is modified.

## Conventions to follow

- Keep the existing style of the file: GPL banner, `#pragma once`, nested
  `namespace bg2e {` / `namespace reflection {` blocks, plain struct with no
  `BG2E_API`.
- The module's include discipline is unchanged (std includes only in this
  header).

## Changes to `Property.hpp`

### 1. Add `Object` to `PropertyType`

Append `Object` as the **last** enumerator (do not reorder existing values):

```cpp
enum class PropertyType {
    Bool, Int, UInt, Float, Double, String,
    Vec2, Vec3, Vec4, Mat4,
    Color, Enum, Resource, Path,
    Object
};
```

`PropertyEditor` is **not** extended: consumers render an object property as
a collapsible group / sub-panel and choose that presentation from
`PropertyType::Object` itself. The `editor` field of an object property
always stays `PropertyEditor::Default` (step 02 makes it impossible to set).

### 2. Add the depth-limit constant

Directly above `PropertyMetadata`:

```cpp
// Maximum depth of Object-property chains a consumer may recurse into.
// A root reflected instance is depth 0; the targets of its object
// properties are depth 1, and so on. See TypeRegistry::objectChainDepth().
inline constexpr uint32_t maxObjectDepth = 3;
```

### 3. Extend `PropertyInfo`

```cpp
struct PropertyInfo {
    std::string name;
    PropertyType type = PropertyType::Float;
    PropertyEditor editor = PropertyEditor::Default;
    PropertyMetadata metadata;

    // Type-erased accessors. Instances are addressed as void*.
    std::function<std::any(const void*)> getter;
    std::function<void(void*, const std::any&)> setter;

    // Object properties (type == PropertyType::Object) only:
    // the sub-object is addressed by pointer and edited in place through
    // its own reflected setters; there is deliberately no parent setter.
    std::string objectTypeName;                             // TypeRegistry key of the sub-object type
    std::function<const void*(const void*)> objectGetter;   // address of the sub-object (always set)
    std::function<void*(void*)> objectMutableGetter;        // empty => sub-object is read-only

    // A property without a write path is read-only. There is no separate
    // flag: scalar properties are read-only without a setter; object
    // properties are read-only without a mutable object getter.
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

Notes:

- `getter` / `setter` stay **empty** for object properties. Returning the
  sub-object by value through `std::any` would copy (and slice) it, which is
  useless for in-place editing; the pointer accessors are the only access
  path.
- `objectTypeName` is resolved by consumers at runtime through
  `TypeRegistry::type(...)`. Registration order between translation units is
  undefined, so nothing is validated or resolved at registration time.
- For object properties the existing metadata fields that remain meaningful
  are `displayName`, `category` and `tooltip`. `min`/`max`/`step`,
  `enumOptions` and `editor` are meaningless — step 02 makes them
  un-settable at compile time rather than silently ignored.

## Acceptance criteria

- `cmake` re-configure + `cmake --build build` succeeds with no other file
  modified.
- The existing definitions (`TransformComponentReflection.cpp`,
  `LightReflection.cpp`) compile unchanged: `isReadOnly()` behavior for
  scalar properties is untouched.
