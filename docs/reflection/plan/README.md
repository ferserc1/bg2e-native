# bg2e::reflection — Object (Nested) Properties: Implementation Plan (Summary)

Extends the existing `bg2e::reflection` module so that a property can be a
**sub-object of another reflected type** — for example `scene::LightComponent`
exposing its `base::Light _light` member through `light()`. A future generic
UI can then recurse into the sub-object and edit it **in place** through the
sub-object's own reflected setters.

Each step has its own file with detailed instructions (`01-*.md` … `04-*.md`)
and leaves the project in a compilable state.

## Goals

- A new property kind, `PropertyType::Object`, for reflected sub-objects.
- In-place editing: the parent never replaces the sub-object; the sub-object
  is mutated through its own reflection metadata.
- Compile-time API safety: builder operations that are meaningless for
  objects (`slider()`, `range()`, `enumValue()`, …) must **not compile**,
  instead of being silently accepted.
- A fixed, documented nesting depth limit: **3 levels**.
- A first real definition: `scene::LightComponent` → `base::Light`.

## Explicitly out of scope (this phase)

- No UI, ImGui, or widget generation. This phase produces metadata plus a
  documented consumption contract only.
- No serialization changes. `Component::serialize/deserialize` untouched.
- No pointer/smart-pointer sub-objects (`std::shared_ptr<T>`, polymorphic
  object references). Only by-value members accessed by reference getters.
- No replace-the-sub-object setter on the parent.

## Key design decisions

1. **The verb is `object()`, not `nested()`.** "Nested" describes a visual
   layout; the API describes the *type* of the property. The naming applies
   consistently to the new `PropertyInfo` fields: `objectTypeName`,
   `objectGetter`, `objectMutableGetter`.

2. **In-place editing only.** An object property has **no `setter`** — the
   parent cannot replace the sub-object. Access is through address-returning
   accessors:
   - `objectGetter` (`const void*(const void*)`) — always present.
   - `objectMutableGetter` (`void*(void*)`) — present only when a mutable
     reference getter is registered; otherwise the sub-object is read-only.
   The "read-only = no write path" convention is preserved:
   `PropertyInfo::isReadOnly()` returns `!objectMutableGetter` for objects,
   `!setter` for everything else.

3. **Compile-time API safety via a separate builder type.** `object()`
   returns an `ObjectBuilder<T>` that exposes **only** `displayName()`,
   `category()` and `tooltip()`. `editor()`, `slider()`, `range()`,
   `enumValue()`, … simply do not exist on that type, so
   `t.object(...).slider()` is a compile error, not a silent no-op.

4. **Depth limit: `reflection::maxObjectDepth = 3`.** A root reflected
   instance (e.g. a component) is depth 0; its object properties' targets are
   depth 1, and so on. Consumers must not recurse beyond `maxObjectDepth`.
   Because static-init order between translation units is undefined, depth
   cannot be validated at registration time; instead `TypeRegistry` gains
   consumption-time helpers (`objectChainDepth()`, `validateObjectDepth()`)
   so tools/debug builds can verify the limit.

5. **Explicit registry key.** `object(name, "bg2e::base::Light", …)` takes
   the nested type's registry key explicitly, consistent with the
   explicit-name constructor of `TypeRegistration`. The key is resolved by
   runtime lookup, so registration order between `LightReflection.cpp` and
   `LightComponentReflection.cpp` does not matter.

6. **Opt-in, never silent.** `propertyTypeOf<T>()` keeps `static_assert`ing
   on unsupported class types inside `property()`. Object properties are
   registered exclusively through `object()`, which `static_assert`s the
   opposite (the type must be a class and must **not** be a scalar property
   type). Nothing is silently reclassified.

## Layering and build integration

- All changes stay inside the existing reflection module
  (`lib/include/bg2e/reflection/`, `lib/src/bg2e/reflection/`) plus one new
  definition file in `lib/src/bg2e/scene/reflection/`.
- No new dependencies; the module still sits at Layer 0–1 and never depends
  on `render`, `scene`, or `ui`. `LightComponentReflection.cpp` lives in the
  `scene` module (which *may* depend on `reflection`), same as the existing
  `TransformComponentReflection.cpp`.
- `lib/CMakeLists.txt` uses `file(GLOB_RECURSE ...)`: **no CMake edits are
  needed**. A CMake re-configure is required so the glob picks up the new
  `.cpp`. Do not modify any CMake files.
- `AGENTS.md` needs no update: module layering and dependencies are
  unchanged.

## Target usage (illustrative)

```cpp
// lib/src/bg2e/scene/reflection/LightComponentReflection.cpp (see step 04)
reflection::TypeRegistration<scene::LightComponent> _lightComponentReflection(
    [](reflection::TypeInfoBuilder<scene::LightComponent>& t) {
        t.displayName("Light Source");

        // light() has const and non-const overloads => static_cast disambiguation
        t.object("light", "bg2e::base::Light",
                 static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
                 static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light))
            .displayName("Light")
            .category("Light")
            .tooltip("Light parameters, edited in place");

        // t.object(...).slider();  // <-- compile error: meaningless for objects
    });
```

Consumption contract (documented for the future UI; not implemented here):

```cpp
if (prop.type == reflection::PropertyType::Object) {
    const auto* objectInfo = reflection::TypeRegistry::get().type(prop.objectTypeName);
    const void* sub = prop.objectGetter(instance);
    void* subMut = prop.isReadOnly() ? nullptr : prop.objectMutableGetter(instance);
    if (objectInfo && sub && depth < reflection::maxObjectDepth) {
        // recurse: iterate objectInfo->properties addressing the sub-object
    }
    // objectInfo == nullptr => type not registered: show a fallback label
}
```

## Steps

| Step | File | Contents | Depends on |
|------|------|----------|------------|
| 1 | [01-core-types.md](01-core-types.md) | `PropertyType::Object`, `maxObjectDepth`, `PropertyInfo` object fields, `isReadOnly()` update | — |
| 2 | [02-builder-api.md](02-builder-api.md) | Scalar-type trait refactor, `ObjectBuilder<T>`, `TypeInfoBuilder<T>::object()` overloads | 1 |
| 3 | [03-registry-validation.md](03-registry-validation.md) | `TypeRegistry::objectChainDepth()` / `validateObjectDepth()` | 1 |
| 4 | [04-lightcomponent-and-docs.md](04-lightcomponent-and-docs.md) | `LightComponentReflection.cpp` definition + `doc/api/reflection/` updates | 1–3 |

## Verification

No test framework exists in the repo; verification is compile-based and
performed by the user. After each step:

```sh
cmake -S . -B build -G Ninja -DVULKAN_SDK=$VULKAN_SDK   # re-run: glob picks up new files
cmake --build build
```

Step 02 includes a documented **negative check**: a snippet using
`t.object(...).slider()` that must *fail* to compile (verified once by
pasting it into a definition file, then removed). Step 04 provides the first
real instantiation of the new templates, which is where template errors
would surface.
