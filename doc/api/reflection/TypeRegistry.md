# TypeRegistry

**Header:** `<bg2e/reflection/Registry.hpp>`
**Namespace:** `bg2e::reflection`

The generic reflection registry: a leaky singleton keyed by a type-name string,
mirroring the style of `scene::ComponentFactoryRegistry`. It stores one
[`TypeInfo`](TypeInfo.md) per key and knows nothing about `scene::Component` or
any engine type beyond `reflection::TypeInfo`. This is the **only** `BG2E_API`
class in the module.

```cpp
class BG2E_API TypeRegistry {
public:
    static TypeRegistry& get();

    void registerType(TypeInfo info);

    const TypeInfo * type(const std::string& typeName) const;
    bool contains(const std::string& typeName) const;
    std::vector<std::string> typeNames() const;

    template<typename Base, typename Derived>
    void registerSubtype(std::string baseTypeName, std::string key,
                         std::string displayName, std::string typeName = {});

    const SubtypeInfo* subtype(const std::string& baseTypeName,
                               const std::string& key) const;
    std::vector<SubtypeInfo> subtypes(const std::string& baseTypeName) const;
    std::string subtypeKey(const std::string& baseTypeName,
                           const std::type_info& dynamicType) const;
    const void* subtypeObject(const std::string& baseTypeName,
                              const std::string& key,
                              const void* baseObject) const;
    void* subtypeObject(const std::string& baseTypeName,
                        const std::string& key,
                        void* baseObject) const;
    template<typename Base>
    std::shared_ptr<Base> createSubtype(const std::string& baseTypeName,
                                        const std::string& key) const;

    uint32_t objectChainDepth(const std::string& typeName) const;
    bool validateObjectDepth(std::vector<std::string>* offenders = nullptr) const;

protected:
    TypeRegistry() = default;

    std::unordered_map<std::string, TypeInfo> _registry;
    static TypeRegistry * _registrySingleton;
};
```

---

## Singleton lifecycle

`get()` lazily creates a single `TypeRegistry` with `new` and never deletes it.
The constructor is `protected`, so the instance is reachable only through
`get()`. This is the established repo pattern (same as
`ComponentFactoryRegistry`) and avoids static-destruction-order problems when
types register themselves during static initialization via
[`TypeRegistration<T>`](Registration.md).

```cpp
reflection::TypeRegistry& reg = reflection::TypeRegistry::get();
```

**Key points:**
- The singleton is created on first use — safe to call during static init.
- Never delete or copy the instance; go through `get()`.
- Type metadata is keyed by `TypeInfo::typeName`; subtype metadata is stored
  per base-hierarchy key in registration order.

---

## Methods

Type metadata and polymorphic subtype metadata are independent and may be
registered in either order.

### `void registerType(TypeInfo info)`

Registers — or **replaces** — the metadata stored under `info.typeName`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `info` | `TypeInfo` | Fully built metadata record. Moved into the registry. |

Re-registration uses `_registry[name] = std::move(info)`, so the last writer
wins. This keeps hot-edit and manual-registration workflows simple, but invalidates
any `TypeInfo`/`PropertyInfo` pointers previously obtained for that key — re-look
them up afterwards.

### `const TypeInfo * type(const std::string& typeName) const`

Returns a pointer to the stored `TypeInfo`, or `nullptr` when the type has no
reflection metadata. Reflection is optional by design, so callers must handle
`nullptr`.

```cpp
const reflection::TypeInfo* info = reflection::TypeRegistry::get().type("Transform");
if (!info) { /* type is not reflected */ }
```

### `bool contains(const std::string& typeName) const`

Returns `true` when a key is registered.

```cpp
if (reflection::TypeRegistry::get().contains("bg2e::base::Light")) { /* ... */ }
```

### `std::vector<std::string> typeNames() const`

Returns all registered keys. Order is unspecified (unordered-map iteration).

```cpp
for (const auto& name : reflection::TypeRegistry::get().typeNames())
    std::cout << name << "\n";
```

### `uint32_t objectChainDepth(const std::string& typeName) const`

Depth of the deepest `Object`-property chain reachable from `typeName`, using
the [depth semantics](Property.md#maxobjectdepth) where a root instance is
depth 0: a type with one object property whose target has none has chain depth
`1`. `0` means the type has no object properties (or is not registered).
Only by-value `Object` edges contribute; polymorphic selections are handled
separately and do not change this validation depth.

The check is a DFS over object-property edges following `objectTypeName` keys.
A visited-path set guards against reference cycles (which by-value nesting
cannot produce, but future pointer-based nesting could), and unregistered
`objectTypeName` keys simply terminate the chain — they are **not** an error
(reflection is optional; a UI shows a fallback label).

```cpp
// LightSource -> bg2e::base::Light (no object properties) == 1
uint32_t d = reflection::TypeRegistry::get().objectChainDepth("LightSource");
```

### `bool validateObjectDepth(std::vector<std::string>* offenders = nullptr) const`

Returns `true` when every registered type respects
[`maxObjectDepth`](Property.md#maxobjectdepth). When `offenders` is non-null it
is filled with the names of the types whose chains exceed the limit.

Because static-init order between translation units is undefined, depth cannot
be validated at registration time; tooling and debug builds should call this
once **after** all definitions are registered (e.g. at startup) and log any
violations:

```cpp
std::vector<std::string> offenders;
if (!reflection::TypeRegistry::get().validateObjectDepth(&offenders)) {
    for (const auto& name : offenders) { /* log depth-limit violation */ }
}
```

---

## Choosing a key

| Type kind | Key source | Example |
|-----------|-----------|---------|
| Component-like (`BG2E_COMPONENT_TYPE_NAME`) | `T::staticTypeName()` | `"Transform"` |
| Any other type | explicit name passed to `TypeRegistration` | `"bg2e::base::Light"` |

The registry treats both identically — a plain string. Prefer a namespaced key
for non-components to avoid clashing with component names.

---

## Polymorphic subtype registries

`registerSubtype<Base, Derived>()` associates a stable key and display name
with a concrete type in a named base hierarchy. The default overload requires
`Derived` to be default constructible; the factory overload supports custom
construction and must return `std::shared_ptr<Derived>`. `Derived` must inherit
from a polymorphic `Base`.

`subtypes(base)` preserves registration order. Re-registering the same key
replaces its entry in place. `subtype()` looks up metadata, `subtypeKey()` maps
an object's dynamic RTTI type back to its key, and `createSubtype<Base>()`
invokes the type-safe factory. The const and mutable `subtypeObject()` overloads
perform the registered checked downcast for derived-type reflection. Unknown
keys, factory mismatches, and failed casts return null/empty values.

---

## There is no `typeOf<T>()`

v1 deliberately omits a `typeOf<T>()` template helper. Callers that know the
static type query the registry directly:

```cpp
auto* info = reflection::TypeRegistry::get().type(T::staticTypeName());
```

Add a typed helper only when a real consumer needs it.

---

## See also

- [Builder](Builder.md) — assembles a `TypeInfo` to register.
- [Registration](Registration.md) — static-init wrapper around `registerType`.
- [quick_start](quick_start.md#recipe-8-consume-the-registry-generic-ui-loop) —
  consuming at runtime.
