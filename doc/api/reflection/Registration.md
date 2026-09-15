# Registration

**Header:** `<bg2e/reflection/Registration.hpp>`
**Namespace:** `bg2e::reflection`

The static-initialization registration helper: the macro-free analogue of
`RegisterComponent<T>` + `BG2E_SCENE_REGISTER_COMPONENT`. A namespace-scope
`TypeRegistration<T>` object runs during static init, builds a
[`TypeInfoBuilder<T>`](Builder.md#typeinfobuildert) from a `define` lambda, and
registers the result in the [`TypeRegistry`](TypeRegistry.md).

```cpp
template<typename T>
class TypeRegistration {
public:
    // For component-like types: key = T::staticTypeName().
    explicit TypeRegistration(std::function<void(TypeInfoBuilder<T>&)> define);

    // For arbitrary types: explicit registry key.
    TypeRegistration(std::string typeName, std::function<void(TypeInfoBuilder<T>&)> define);
};
```

---

## Constructors

### `TypeRegistration(define)`

Registers under `T::staticTypeName()`. Use this for component-like types that
carry the `BG2E_COMPONENT_TYPE_NAME` macro.

```cpp
reflection::TypeRegistration<scene::TransformComponent> _xform(
    [](reflection::TypeInfoBuilder<scene::TransformComponent>& t) {
        t.displayName("Transform");
        t.property("matrix", &scene::TransformComponent::matrix,
                             &scene::TransformComponent::setMatrix);
    });
```

### `TypeRegistration(typeName, define)`

Registers under an explicit key. Use this for non-component types (no
`staticTypeName()`), or whenever the key differs from the type name.

```cpp
reflection::TypeRegistration<base::Light> _light("bg2e::base::Light",
    [](reflection::TypeInfoBuilder<base::Light>& t) {
        t.displayName("Light");
    });
```

Both constructors run the `define` callback against a fresh builder and then
call `TypeRegistry::get().registerType(builder.build())`.

---

## How and where to use it

Reflection definitions live in a **separate `.cpp` file** next to the module of
the type they describe — never inside the type's own header/`.cpp` — so
reflection stays visibly optional and components stay clean.

```
lib/src/bg2e/scene/reflection/TransformComponentReflection.cpp
lib/src/bg2e/base/reflection/LightReflection.cpp
```

A minimal definition file:

```cpp
#include <bg2e/reflection/Registration.hpp>   // pulls Builder + Registry
#include <bg2e/scene/MyComponent.hpp>

using namespace bg2e;

namespace {
reflection::TypeRegistration<scene::MyComponent> _myComp(
    [](reflection::TypeInfoBuilder<scene::MyComponent>& t) {
        t.displayName("My Component");
        t.property("count", &scene::MyComponent::count, &scene::MyComponent::setCount)
            .displayName("Count")
            .category("General");
    });
}
```

**Key points:**
- The object is an internal-linkage static (in an anonymous namespace); its
  constructor performs the registration.
- The CMake `file(GLOB_RECURSE ...)` picks the `.cpp` up automatically. A CMake
  **re-configure** is enough; no build-file edits are needed.
- Only `Registration.hpp` needs including in a definition file — it depends on
  `Builder.hpp`, which depends on `Registry.hpp`.

---

## Static-initialization safety

`TypeRegistry::get()` lazily creates the singleton on first use, so registering
from a static-init constructor is safe regardless of the order in which the
`TypeRegistration` objects across translation units are constructed (same
rationale as `ComponentFactoryRegistry`). The singleton is never destroyed, so
there is no static-destruction-order hazard.

```cpp
// After static init, at runtime:
auto* info = reflection::TypeRegistry::get().type("Transform");        // non-null
auto* light = reflection::TypeRegistry::get().type("bg2e::base::Light"); // non-null
```

---

## Linking caveat

A static-init object only runs if its translation unit is linked into the binary.
When reflection definitions live in the engine shared library (as they do here),
they are part of the library's initialization and register themselves when the
library loads. If you instead place definitions in a TU the linker might discard
(unused, header-only, or a static lib archive member), force it or register
manually — see
[quick_start Recipe 10](quick_start.md#recipe-10-register-a-type-manually-no-static-init).

---

## Deferred extensions (not implemented)

- A `BG2E_REFLECTION_REGISTER(...)` macro if the boilerplate becomes noisy.
- Lambda / free-function accessors (e.g. to reflect `scene::LightComponent` by
  delegating to its `light()` sub-object).

---

## See also

- [Builder](Builder.md) — what `define` operates on.
- [TypeRegistry](TypeRegistry.md) — where registration lands.
- [quick_start](quick_start.md#recipe-1-register-a-component-type) — recipes 1 & 2.
