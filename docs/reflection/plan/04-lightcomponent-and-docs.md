# Step 04 — `LightComponent` definition and documentation

## Goal

Register the first real object property — `scene::LightComponent` exposing
its `base::Light` sub-object — and update the user-facing reflection docs.

## Files to create

```
lib/src/bg2e/scene/reflection/LightComponentReflection.cpp
```

## Files to modify

```
doc/api/reflection/index.md
doc/api/reflection/Property.md
doc/api/reflection/Builder.md
doc/api/reflection/TypeRegistry.md
doc/api/reflection/quick_start.md
doc/api/reflection/reference.md
```

No CMake edits: the `lib` glob picks up the new `.cpp` after a re-configure.

## 1. `LightComponentReflection.cpp`

Follows the established convention: a dedicated `.cpp` in the module of the
described type, anonymous namespace, static-init `TypeRegistration`.

```cpp
// GPL banner (copy from TransformComponentReflection.cpp)

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/LightComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::LightComponent> _lightComponentReflection(
    [](reflection::TypeInfoBuilder<scene::LightComponent>& t) {
        t.displayName("Light Source");

        // light() has const and non-const overloads => static_cast disambiguation.
        // The sub-object is edited in place through base::Light's own
        // reflected setters (registered under "bg2e::base::Light").
        t.object("light", "bg2e::base::Light",
                 static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
                 static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light))
            .displayName("Light")
            .category("Light")
            .tooltip("Light parameters, edited in place");

        // Getter-only => read-only (world-space values derived from the node).
        t.property("position", &scene::LightComponent::position)
            .displayName("Position")
            .category("Light");

        t.property("direction", &scene::LightComponent::direction)
            .displayName("Direction")
            .category("Light");
    });

}
```

Notes:

- The registry key `"bg2e::base::Light"` must match the explicit key used in
  `lib/src/bg2e/base/reflection/LightReflection.cpp`. It is resolved at
  consumption time, so the static-init order of the two translation units
  does not matter.
- Depth check: `LightSource` → `bg2e::base::Light` is a chain of 1, well
  under `maxObjectDepth = 3`; `base::Light` itself has no object properties.

## 2. Documentation updates (`doc/api/reflection/`)

### `index.md`

- Move object properties into **In scope**: "Describing object properties:
  sub-objects of another reflected type, edited in place, with a fixed
  nesting depth limit (`maxObjectDepth = 3`)."
- Keep in **Out of scope**: pointer/smart-pointer sub-objects and
  replace-the-sub-object setters.
- Add a short "Object properties" section with the consumption snippet from
  the plan README (runtime lookup of `objectTypeName`, `objectGetter` /
  `objectMutableGetter`, depth cap) and a note that static-init order does
  not matter because the key is resolved at runtime.
- Update the architecture diagram line for `PropertyInfo` to mention the
  object accessors.

### `Property.md`

- Document `PropertyType::Object`, `maxObjectDepth`, and the three new
  `PropertyInfo` fields (`objectTypeName`, `objectGetter`,
  `objectMutableGetter`).
- Document the extended `isReadOnly()` rule (objects: read-only without a
  mutable object getter).
- State explicitly that `getter`/`setter`, `editor`, `min`/`max`/`step` and
  `enumOptions` are unused for object properties.

### `Builder.md`

- New section for `object()` with both overloads and the `static_cast`
  disambiguation example for overloaded getters (`LightComponent::light`).
- Document `ObjectBuilder<T>`: only `displayName()`, `category()`,
  `tooltip()`; everything else is a **compile error by design** (show the
  `t.object(...).slider()` negative example).
- Document the `static_assert` contract: `object()` requires a non-scalar
  class type; scalar types must use `property()`.

### `TypeRegistry.md`

- Document `objectChainDepth()` and `validateObjectDepth()` and the depth
  semantics (root = 0).

### `quick_start.md`

- Add a recipe: "Exposing a sub-object (object property)" based on the
  `LightComponent` example, including the read-only variant (const getter
  only).

### `reference.md`

- Add the new enum value, constant, fields, builder methods and registry
  methods to the corresponding reference sections.

## 3. Acceptance criteria

- `cmake -S . -B build -G Ninja -DVULKAN_SDK=$VULKAN_SDK` (re-configure so
  the glob picks up `LightComponentReflection.cpp`) + `cmake --build build`
  succeeds.
- `TypeRegistry::get().type("LightSource")` is registered with one
  `PropertyType::Object` property named `light` whose `objectTypeName` is
  `"bg2e::base::Light"` and which is **not** read-only
  (`isReadOnly() == false`).
- `TypeRegistry::get().validateObjectDepth()` returns `true`.
- The six documentation files listed above are updated and consistent with
  the implementation.
