# Reflection System

The `bg2e::reflection` namespace provides a minimal, **accessor-based**
reflection system for the engine. Logical *properties* are described through
public getters/setters (never raw fields), and *actions* expose parameterless
methods or lambdas receiving the component instance, suitable for generated UI
buttons. UI-oriented metadata (display name,
category, tooltip, ranges, steps, editor choice) is kept conceptually separate
from the data type itself.

Reflection is **optional**: a type can exist, be factory-registered, and be
serializable without carrying any reflection metadata. A generic
[`TypeRegistry`](TypeRegistry.md) stores the metadata, keyed by a plain
`std::string`, and does not assume anything about `scene::Component`.

> **Status:** This is a standalone, self-contained module. It sits at Layer 0–1
> (below `scene`) and depends only on `common`, `math` (GLM), `base::Color`, and
> the standard library. It never depends on `render`, `scene`, or `ui`.

---

## Table of Contents

1. [What reflection is (and is not)](#what-reflection-is-and-is-not)
2. [Architecture](#architecture)
3. [Layering and dependencies](#layering-and-dependencies)
4. [A complete example in one screen](#a-complete-example-in-one-screen)
5. [Reading and writing values](#reading-and-writing-values)
6. [Object properties](#object-properties)
7. [Conventions](#conventions)
8. [Where to go next](#where-to-go-next)

---

## What reflection is (and is not)

**In scope**

- Describing *logical properties* defined by public accessors.
- Exposing *parameterless actions*: methods that take no arguments (their return
  value — including fluent `T*` returns — is discarded) or lambdas/functors
  invocable with `T*` that receive the component instance.
- Attaching UI metadata: display name, category, tooltip, `min`/``max`/`step`,
  editor kind, enum option pairs, and resource file filters.
- Describing **object properties**: sub-objects of another reflected type,
  edited in place, with a fixed nesting depth limit (`maxObjectDepth = 3`).
- Describing owned **polymorphic objects**, including subtype discovery,
  factories, replacement, allowlists, and derived-type reflection.
- A generic registry that any type can opt into.

**Out of scope (this phase)**

- No UI dependency. The system produces metadata; `bg2e::ui` consumes it via
  `ReflectionWidget` and `ComponentInspector`.
- No serialization changes. `Component::serialize/deserialize` is untouched and
  independent.
- No arbitrary method reflection (only parameterless actions).
- Arbitrary pointer properties are not inferred. Owned polymorphic objects are
  supported explicitly through `polymorphicObject()` and registered factories.
- No replace-the-sub-object setter on the parent: object properties are edited
  **in place** through the sub-object's own reflection metadata.
- No macros. Registration uses the template helper [`TypeRegistration<T>`](Registration.md).
- No free-function or lambda accessors in v1 — only **member function pointers**.
- `Quat` is not a property type yet (no quaternion accessors in current types).

**Key design decisions**

1. **Read-only = no write path.** There is no `readOnly` flag. For scalar
   properties, `PropertyInfo::isReadOnly()` is `!setter`; for object properties
   it is `!objectMutableGetter`; polymorphic objects are read-only when neither
   mutation nor replacement is available (see
   [Object properties](#object-properties)).
2. **Editor choice is orthogonal to constraints.** `range(min, max)` only sets
   `metadata.min`/`max`; `slider()`, `drag()`, `input()`… only set the editor. A
   property can have a range *and* a plain input, or a range *and* a slider.
3. **Type erasure with `std::any` + `std::function`.** Instances are addressed as
   `void*`; there is no inheritance or virtual accessor hierarchy.
4. **Registry key is a plain `std::string`.** Component-like types reuse the
   existing `T::staticTypeName()`; any other type registers under an explicit
   name.

---

## Architecture

The system is organized in three tiers:

1. **Value types** (`Property`, `Action`, `TypeInfo`) — plain data describing a
   reflected property, action, and type. No behavior beyond two name lookups.
2. **The registry** (`TypeRegistry`) — a leaky singleton keyed by type-name
   string, mirroring the style of `scene::ComponentFactoryRegistry`.
3. **The definition API** (`Builder`, `Registration`) — header-only templates:
   accessor traits, a `PropertyType<T>` deduction, the chained
   `TypeInfoBuilder<T>`, and the static-init `TypeRegistration<T>` helper.

```
reflection::PropertyInfo / ActionInfo / TypeInfo   (plain structs)
reflection::PropertyMetadata                        (constraints + display)
reflection::TypeRegistry                            (BG2E_API singleton)
reflection::TypeInfoBuilder<T>                      (template, header-only)
  +-- reflection::PropertyBuilder<T>                (chained metadata)
  +-- reflection::ObjectBuilder<T>                  (chained metadata, objects)
  +-- reflection::PolymorphicObjectBuilder<T>       (metadata + subtype allowlist)
  +-- reflection::ActionBuilder<T>                  (chained metadata)
reflection::TypeRegistration<T>                     (template, static-init)
reflection::{Getter,Setter}Traits, propertyTypeOf   (accessor meta-programming)
```

`PropertyInfo` carries both the scalar accessors (`getter`/`setter`, value via
`std::any`) and the object accessors (`objectGetter`/`objectMutableGetter`,
address via `void*`). Polymorphic objects use base-object pointer accessors, an
active subtype-key callback, and an optional replacement callback.

### Definition vs. consumption

Definition happens once, at static-initialization time, in a dedicated `.cpp`
file next to the module of the type it describes. Consumption happens at runtime
through the registry:

```
// Definition (static init)                  // Consumption (runtime)
TypeRegistration<T>(...)                     auto* info = TypeRegistry::get().type("Transform");
  -> TypeInfoBuilder<T>                       for (auto& p : info->properties) { ... }
  -> .property(...) / .action(...)            auto* matrix = info->property("matrix");
  -> TypeRegistry::registerType(build())       std::any v = matrix->getter(instancePtr);
```

---

## Layering and dependencies

| Depends on | Forbidden |
|------------|-----------|
| `bg2e/common.hpp` (`BG2E_API`) | `bg2e::render` |
| `bg2e/math/base.hpp` (GLM vector/matrix types) | `bg2e::scene` |
| `bg2e/base/Color.hpp` | `bg2e::db`, `bg2e::ui`, `bg2e::utils` |
| `<any>`, `<functional>`, `<optional>`, `<filesystem>`, … | (any higher-level engine namespace) |

`scene` and `ui` **may** depend on `reflection`; the reverse must never happen.
Reflection definitions for a type live in the module of that type (for example
`lib/src/bg2e/scene/reflection/` or `lib/src/bg2e/base/reflection/`), never
inside the type's own files, so reflection stays visibly optional.

---

## A complete example in one screen

The definitions shipped with the engine exercise every feature of the API
(`scene::TransformComponent`, `base::Light`, and `scene::LightComponent` — the
last one registers an object property, see
[Object properties](#object-properties)).

**A component type (`scene::TransformComponent`)** — registered under its
`staticTypeName()` key `"Transform"`; demonstrates the `const T&` getter +
`const T&` setter pair and a fluent (non-`void`) action:

```cpp
// lib/src/bg2e/scene/reflection/TransformComponentReflection.cpp
#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/TransformComponent.hpp>

using namespace bg2e;

namespace {
reflection::TypeRegistration<scene::TransformComponent> _transformReflection(
    [](reflection::TypeInfoBuilder<scene::TransformComponent>& t) {
        t.displayName("Transform");

        t.property("matrix", &scene::TransformComponent::matrix,
                             &scene::TransformComponent::setMatrix)
            .displayName("Matrix")
            .category("Transform");

        t.action("setIdentity", &scene::TransformComponent::setIdentity)
            .displayName("Set Identity")
            .category("Transform")
            .tooltip("Reset the transform to the identity matrix");
    });
}
```

**A non-component type (`base::Light`)** — registered under the explicit key
`"bg2e::base::Light"`; demonstrates virtual accessors, scalars, an enum with
option pairs, editor/constraint separation, and a getter-only (read-only)
property:

```cpp
// lib/src/bg2e/base/reflection/LightReflection.cpp (excerpt)
reflection::TypeRegistration<base::Light> _lightReflection("bg2e::base::Light",
    [](reflection::TypeInfoBuilder<base::Light>& t) {
        t.displayName("Light");

        t.property("intensity", &base::Light::intensity, &base::Light::setIntensity)
            .displayName("Intensity")
            .category("Light")
            .range(0.0, 100.0)   // constraint only...
            .slider()            // ...editor chosen independently
            .step(0.1);

        t.property("type", &base::Light::type, &base::Light::setType)
            .combo()
            .enumValue("Omni", base::Light::TypeOmni)
            .enumValue("Spot", base::Light::TypeSpot);

        t.property("shadowSamples", &base::Light::shadowSamples,
                                          &base::Light::setShadowSamples)
            .range(1.0, 64.0)
            .step(1.0);          // range + step, no slider: Default editor kept

        t.property("typeString", &base::Light::typeString);  // getter only => read-only
    });
```

---

## Reading and writing values

Accessors are type-erased:

```cpp
std::function<std::any(const void*)>        getter;   // read  property
std::function<void(void*, const std::any&)> setter;   // write property
std::function<void(void*)>                  invoke;   // run action
```

You address an instance as a `void*` and `std::any_cast` the value back to the
concrete C++ type, guided by `PropertyInfo::type`:

```cpp
auto& reg = reflection::TypeRegistry::get();
const reflection::TypeInfo* info = reg.type("bg2e::base::Light");
if (info) {
    base::Light light;
    void* obj = &light;

    const reflection::PropertyInfo* intensity = info->property("intensity");
    float value = std::any_cast<float>(intensity->getter(obj));   // read
    intensity->setter(obj, std::any(2.5f));                       // write

    const reflection::ActionInfo* act = info->action("...");       // (if any)
    if (act) act->invoke(obj);                                    // run
}
```

> Casting to the wrong type throws `std::bad_any_cast`. Match the `PropertyType`
> enum to the concrete type (see [Builder](Builder.md#propertytype-of-c-type--propertytype)).

Enums are the exception: their erased value is always `int64_t`.

---

## Object properties

A property can also be a **sub-object of another reflected type** — for example
`scene::LightComponent` exposing its `base::Light` member. Object properties are
declared with [`TypeInfoBuilder<T>::object`](Builder.md#objectname-objecttypename-getters)
and have `type == PropertyType::Object`.

The parent never replaces the sub-object: there is deliberately no setter.
Instead the sub-object is addressed **by pointer** and edited **in place**
through its own reflected setters:

- `objectTypeName` — the `TypeRegistry` key of the sub-object type.
- `objectGetter` (`const void*(const void*)`) — always present.
- `objectMutableGetter` (`void*(void*)`) — present only when a mutable reference
  getter was registered; otherwise the sub-object is read-only.

`getter`/`setter` stay empty for object properties (returning the sub-object by
`std::any` would copy and slice it). Consumption is a runtime lookup plus a
recursive walk, capped at `reflection::maxObjectDepth` (3) levels — a root
instance is depth 0:

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

`objectTypeName` is resolved at consumption time, so the static-init order of
the defining translation units does not matter. Tools and debug builds can
verify the limit with [`TypeRegistry::validateObjectDepth`](TypeRegistry.md).

Polymorphic object properties use a named base hierarchy. The registry maps
stable subtype keys to factories, RTTI, checked casts, display names, and
derived `TypeInfo` keys. See [TypeRegistry](TypeRegistry.md#polymorphic-subtype-registries).

---

## Engine registrations

The shipped metadata covers `base::Light`, `base::LinkJoint`, projection base
and concrete types, and the following scene components: Transform, Light,
Camera, Drawable, Environment, Orbit Camera, Polar Transform Controller, Fixed
Scale Transform Controller, Chain, Input Chain Joint, and Output Chain Joint.

Notable compositions are the nested `Light` and `LinkJoint` objects, Camera's
polymorphic projection, Environment's filtered image Resource, and Transform's
editable translation/rotation/scale plus read-only matrix and identity action.
Drawable and Chain currently expose display-level metadata only.

---

## Conventions

- Plain structs (`PropertyInfo`, `ActionInfo`, `TypeInfo`, `PropertyMetadata`)
  carry no `BG2E_API`. The **only** `BG2E_API` class in the module is
  [`TypeRegistry`](TypeRegistry.md).
- All templates in [Builder](Builder.md) and [Registration](Registration.md) are
  header-only.
- Includes: engine includes (`<bg2e/...>`) first, then standard library includes.
- `#pragma once` and the GPL banner on every file.
- Registry keys for component-like types reuse the existing
  `T::staticTypeName()` (e.g. `"Transform"`). Non-components use an explicit,
  collision-free name (e.g. `"bg2e::base::Light"`).

---

## Where to go next

- **[reference.md](reference.md)** — full class, struct, and enum reference.
- **[quick_start.md](quick_start.md)** — recipe-oriented practical guide.
- Per-type docs: [Property](Property.md), [Action](Action.md),
  [TypeInfo](TypeInfo.md), [TypeRegistry](TypeRegistry.md),
  [Builder](Builder.md), [Registration](Registration.md).
