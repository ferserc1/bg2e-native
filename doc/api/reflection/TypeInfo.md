# TypeInfo

**Header:** `<bg2e/reflection/TypeInfo.hpp>`
**Namespace:** `bg2e::reflection`

The complete reflection record for one type: its registry key, display name, and
the ordered lists of its [`PropertyInfo`](Property.md#propertyinfo) and
[`ActionInfo`](Action.md) entries. Plain data with two name-based lookups.

```cpp
struct TypeInfo {
    std::string typeName;
    std::string displayName;
    std::vector<PropertyInfo> properties;
    std::vector<ActionInfo> actions;

    const PropertyInfo * property(const std::string& name) const;
    const ActionInfo *   action(const std::string& name) const;
};
```

---

## Members

| Member | Type | Description |
|--------|------|-------------|
| `typeName` | `std::string` | Registry key. For components this is `staticTypeName()` (e.g. `"Transform"`); for other types an explicit name (e.g. `"bg2e::base::Light"`). |
| `displayName` | `std::string` | Human-readable label for the type. |
| `properties` | `std::vector<PropertyInfo>` | Reflected properties, in declaration order. |
| `actions` | `std::vector<ActionInfo>` | Reflected actions, in declaration order. |

`properties` and `actions` are ordered by declaration, so a generic UI can render
them in a stable sequence and group them by `metadata.category`.

---

## Methods

### `const PropertyInfo * property(const std::string& name) const`

Linear scan over `properties` for a matching `name`. Returns a pointer to the
found element, or `nullptr` if absent.

```cpp
const reflection::PropertyInfo* p = info->property("intensity");
if (p) { /* read/write via p->getter / p->setter */ }
```

### `const ActionInfo * action(const std::string& name) const`

Linear scan over `actions` for a matching `name`. Returns a pointer to the found
element, or `nullptr` if absent.

```cpp
const reflection::ActionInfo* a = info->action("setIdentity");
if (a) a->invoke(&instance);
```

Lookups use a simple linear scan by design — property and action lists are small.

---

## Obtaining a TypeInfo

`TypeInfo` is normally produced by a [`TypeInfoBuilder<T>`](Builder.md#typeinfobuildert)
and stored in the [`TypeRegistry`](TypeRegistry.md). Consumers retrieve it by
name:

```cpp
using namespace bg2e;

const reflection::TypeInfo* info =
    reflection::TypeRegistry::get().type("Transform");

if (info) {
    for (const auto& prop : info->properties) { /* ... */ }
    for (const auto& act   : info->actions)    { /* ... */ }
}
```

You can also build one manually and register it — see
[quick_start Recipe 10](quick_start.md#recipe-10-register-a-type-manually-no-static-init).

---

## Type-erasure contract

The pointers returned by `property()` / `action()` refer to the elements owned by
the `TypeInfo` inside the registry. Do **not** hold them across a
`registerType()` call for the same key: re-registration replaces the stored
`TypeInfo` and invalidates prior pointers. Re-look-up after (re-)registration.

---

## See also

- [Property](Property.md) / [Action](Action.md) — element types.
- [TypeRegistry](TypeRegistry.md) — storage and lookup by key.
