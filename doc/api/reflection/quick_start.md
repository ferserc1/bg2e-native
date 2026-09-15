# bg2e::reflection Quick Start Guide

A practical, recipe-oriented guide to `bg2e::reflection`. Each recipe is a small
self-contained task: how to *define* reflection for a type, and how a consumer
*reads* it back at runtime. Working definitions ship in
`lib/src/bg2e/scene/reflection/` and `lib/src/bg2e/base/reflection/`.

---

## Table of Contents

1. [Include the module](#include-the-module)
2. [Recipe 1: Register a component type](#recipe-1-register-a-component-type)
3. [Recipe 2: Register a non-component type](#recipe-2-register-a-non-component-type)
4. [Recipe 3: Read/write vs. read-only properties](#recipe-3-readwrite-vs-read-only-properties)
5. [Recipe 4: Scalar with a slider (editor vs. constraint)](#recipe-4-scalar-with-a-slider-editor-vs-constraint)
6. [Recipe 5: Enum with combo options](#recipe-5-enum-with-combo-options)
7. [Recipe 6: Colors and paths](#recipe-6-colors-and-paths)
8. [Recipe 7: Actions (parameterless methods)](#recipe-7-actions-parameterless-methods)
9. [Recipe 8: Consume the registry (generic UI loop)](#recipe-8-consume-the-registry-generic-ui-loop)
10. [Recipe 9: Type-erased read / write](#recipe-9-type-erased-read--write)
11. [Recipe 10: Register a type manually (no static init)](#recipe-10-register-a-type-manually-no-static-init)
12. [Recipe 11: Exposing a sub-object (object property)](#recipe-11-exposing-a-sub-object-object-property)
13. [Common pitfalls](#common-pitfalls)

---

## Include the module

For **defining** reflection you need the registration/builder header. For
**consuming** it at runtime you need the registry (and the value types).

```cpp
#include <bg2e/reflection/all.hpp>          // everything (recommended)
```

Individual headers:

```cpp
#include <bg2e/reflection/Registry.hpp>      // TypeRegistry + TypeInfo + Property/Action
#include <bg2e/reflection/Registration.hpp>  // TypeRegistration + TypeInfoBuilder (implies Registry)
```

**Key points:**
- `Builder.hpp` and `Registration.hpp` are header-only templates.
- `registration.hpp` pulls in `Builder.hpp`, which pulls in `Registry.hpp`, so a
  definition file only needs `Registration.hpp`.
- The module has no dependency on `scene`, `render`, or `ui`.

---

## Recipe 1: Register a component type

Component-like types already expose `staticTypeName()` (via
`BG2E_COMPONENT_TYPE_NAME`). Use the single-argument `TypeRegistration`
constructor so the registry key is that name.

```cpp
// lib/src/bg2e/scene/reflection/MyComponentReflection.cpp
#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/MyComponent.hpp>

using namespace bg2e;

namespace {
reflection::TypeRegistration<scene::MyComponent> _myCompReflection(
    [](reflection::TypeInfoBuilder<scene::MyComponent>& t) {
        t.displayName("My Component");
        t.property("count", &scene::MyComponent::count, &scene::MyComponent::setCount)
            .displayName("Count")
            .category("General");
    });
}
```

**Key points:**
- The `TypeRegistration` object is a namespace-scope static; its constructor runs
  during static initialization and registers the type.
- Registry key == `scene::MyComponent::staticTypeName()`.
- The file lives next to the type's module, **not** inside the component's own
  `.hpp`/`.cpp` — reflection is opt-in.
- The CMake `file(GLOB_RECURSE ...)` picks the file up automatically; a CMake
  re-configure is enough (no CMake edits).

---

## Recipe 2: Register a non-component type

Types without `staticTypeName()` register under an explicit, collision-free key
(typically the fully-qualified C++ name).

```cpp
// lib/src/bg2e/base/reflection/LightReflection.cpp (excerpt)
reflection::TypeRegistration<base::Light> _lightReflection("bg2e::base::Light",
    [](reflection::TypeInfoBuilder<base::Light>& t) {
        t.displayName("Light");
        t.property("intensity", &base::Light::intensity, &base::Light::setIntensity)
            .displayName("Intensity")
            .category("Light");
    });
```

**Key points:**
- The two-argument constructor takes an explicit registry key first.
- Virtual accessors (all of `base::Light`) work transparently — member pointers
  to virtuals dispatch correctly.
- A non-component uses a namespaced string key (`"bg2e::base::Light"`) to avoid
  clashing with component keys such as `"LightSource"`.

---

## Recipe 3: Read/write vs. read-only properties

Pass **two** accessors for a read/write property; pass **only a getter** for a
read-only one. There is no `readOnly` flag — the absence of a setter *is* the
signal.

```cpp
t.property("color", &base::Light::color, &base::Light::setColor);   // read/write
t.property("typeString", &base::Light::typeString);                 // getter-only => read-only
```

Inspect it at runtime:

```cpp
const reflection::PropertyInfo* p = info->property("typeString");
if (p->isReadOnly()) {
    // no setter was registered -> render a non-editable field
}
```

**Key points:**
- `PropertyInfo::isReadOnly()` returns `!setter`.
- A getter-only property still exposes its value through `getter`.
- The getter and setter must resolve to the same normalized value type, or you
  get a compile-time `static_assert`.

---

## Recipe 4: Scalar with a slider (editor vs. constraint)

Constraints (`range`, `min`, `max`, `step`) and editor choice (`slider`,
`drag`, `input`, …) are **independent** calls. The same numeric range can back a
slider or a plain input.

```cpp
// Slider with a range:
t.property("intensity", &base::Light::intensity, &base::Light::setIntensity)
    .displayName("Intensity")
    .range(0.0, 100.0)   // constraint only
    .slider()            // editor choice, separate
    .step(0.1);

// Same range, but a plain number input (no slider forced):
t.property("shadowSamples", &base::Light::shadowSamples, &base::Light::setShadowSamples)
    .range(1.0, 64.0)
    .step(1.0);          // editor stays Default
```

**Key points:**
- `range(min, max)` sets `metadata.min` and `metadata.max` only.
- `slider()` sets `editor = Slider` only.
- Order doesn't matter; each call touches a different field.
- Available editor shortcuts: `input()`, `slider()`, `drag()`, `checkbox()`,
  `combo()`, `colorEditor()`, `angle()`, or `editor(PropertyEditor)` directly.

---

## Recipe 5: Enum with combo options

Enum properties are deduced as `PropertyType::Enum`. Attach `(label, value)`
pairs for a future combo widget; both the `int64_t` overload and the
templated `EnumT` overload are accepted, so plain and scoped enums both work.

```cpp
t.property("type", &base::Light::type, &base::Light::setType)
    .displayName("Type")
    .combo()
    .enumValue("Omni",        base::Light::TypeOmni)
    .enumValue("Spot",        base::Light::TypeSpot)
    .enumValue("Directional", base::Light::TypeDirectional)
    .enumValue("Disabled",    base::Light::TypeDisabled);
```

**Key points:**
- `enumValue` appends to `metadata.enumOptions` (a `vector<pair<string,int64_t>>`).
- The stored value is an `int64_t` — the underlying enum is `static_cast` for
  you.
- `combo()` only sets the editor; the enum values are independent metadata.
- To read the selected enum back, `std::any_cast` the getter result to the
  concrete enum type (e.g. `base::Light::LightType`).

---

## Recipe 6: Colors and paths

`base::Color` maps to `PropertyType::Color`, `std::filesystem::path` to
`PropertyType::Path`. Color and path are recognized as *distinct* value types
with dedicated editor hints.

```cpp
t.property("color", &base::Light::color, &base::Light::setColor)
    .displayName("Color")
    .colorEditor();          // PropertyEditor::Color

// A path property (asset reference), assuming an accessor pair:
t.property("texture", &MyType::texturePath, &MyType::setTexturePath)
    .displayName("Texture");  // PropertyType::Path is deduced automatically
```

**Key points:**
- `base::Color` is a plain `struct { float r, g, b, a; }`, not `glm::vec4`.
- `colorEditor()` is spelled that way to avoid clashing with the `Color` type;
  it sets `editor = PropertyEditor::Color`.
- GLM types come from `bg2e/math/base.hpp`, `base::Color` from `base/Color.hpp`
  — both are already included by `Builder.hpp`.

---

## Recipe 7: Actions (parameterless methods)

`action(name, memberFn)` wraps any parameterless member function. The return
value is discarded, so fluent methods (returning `T*`) are usable directly as
actions.

```cpp
t.action("setIdentity", &scene::TransformComponent::setIdentity)
    .displayName("Set Identity")
    .category("Transform")
    .tooltip("Reset the transform to the identity matrix");
```

Invoke it:

```cpp
const reflection::ActionInfo* a = info->action("setIdentity");
if (a) a->invoke(&transformComponent);
```

**Key points:**
- `ActionInfo::invoke` is `std::function<void(void*)>`.
- Works with `R (T::*)()` and `R (T::*)() const` for any `R` (including `T*`).
- Actions have display metadata (`displayName`, `category`, `tooltip`) but no
  editor or constraints.

---

## Recipe 8: Consume the registry (generic UI loop)

A future generic inspector walks `TypeInfo` and renders each property grouped by
category. The system only produces metadata — no widgets are generated here.

```cpp
using namespace bg2e;

void describeComponent(const std::string& typeName, void* instance)
{
    const reflection::TypeInfo* info = reflection::TypeRegistry::get().type(typeName);
    if (!info) return;                     // reflection is optional

    std::cout << info->displayName << " (" << info->typeName << ")\n";

    for (const auto& p : info->properties)
    {
        std::cout << "  [" << p.metadata.category << "] "
                  << p.metadata.displayName
                  << (p.isReadOnly() ? " (read-only)" : "");

        if (p.type == reflection::PropertyType::Enum) {
            for (const auto& [label, value] : p.metadata.enumOptions)
                std::cout << " " << label << "=" << value;
        }
        std::cout << "\n";
    }

    for (const auto& a : info->actions)
        std::cout << "  action: " << a.displayName << "\n";
}
```

**Key points:**
- `TypeRegistry::type()` returns `nullptr` for a type with no reflection — always
  check.
- `contains(name)` and `typeNames()` help you enumerate everything registered.
- Group by `metadata.category`; label with `metadata.displayName`; clamp numeric
  fields with `metadata.min`/`max`/`step`.

---

## Recipe 9: Type-erased read / write

Values travel as `std::any`. The getter yields `std::any`; the setter consumes
one. Cast to the concrete type implied by `PropertyInfo::type`.

```cpp
auto& reg = reflection::TypeRegistry::get();
const reflection::TypeInfo* info = reg.type("bg2e::base::Light");
if (!info) return;

base::Light light;
void* obj = &light;

// Read intensity (PropertyType::Float -> float)
const reflection::PropertyInfo* f = info->property("intensity");
float cur = std::any_cast<float>(f->getter(obj));

// Write intensity
f->setter(obj, std::any(cur * 2.0f));

// Read a read-only getter-only property
const reflection::PropertyInfo* s = info->property("typeString");  // std::string
std::string ts = std::any_cast<std::string>(s->getter(obj));

// Enum value (cast back to the concrete enum)
const reflection::PropertyInfo* tp = info->property("type");
base::Light::LightType type = std::any_cast<base::Light::LightType>(tp->getter(obj));
```

**Key points:**
- Use `std::any_cast<T>` with the *exact* underlying type. Wrong casts throw
  `std::bad_any_cast`.
- Reference accessors are stored by value, so `const glm::mat4& matrix()` reads
  back as `std::any_cast<glm::mat4>` (not `const glm::mat4&`).
- To test without throwing, use `std::any_cast<T>(&value)` (pointer form) which
  returns `nullptr` on mismatch.

---

## Recipe 10: Register a type manually (no static init)

If you want to control registration timing (or register a throwaway type), build
a `TypeInfoBuilder<T>` directly and register it yourself. `TypeRegistration` is
only a convenience wrapper around this.

```cpp
reflection::TypeInfoBuilder<base::Light> b("bg2e::base::Light");
b.displayName("Light");
b.property("intensity", &base::Light::intensity, &base::Light::setIntensity)
    .displayName("Intensity");
reflection::TypeRegistry::get().registerType(b.build());
```

**Key points:**
- `build()` returns the assembled `const TypeInfo&`.
- `registerType(TypeInfo)` **replaces** any existing entry with the same
  `typeName` (re-registration is the last writer).
- Prefer `TypeRegistration<T>` for shipped types; manual registration is for
  tests or dynamic scenarios.

---

## Recipe 11: Exposing a sub-object (object property)

A property can be a by-value member of another reflected type, edited **in
place** through that type's own metadata. Register it with `object()` instead
of `property()`, passing the sub-object type's registry key explicitly. Real
definition: `scene::LightComponent` exposing its `base::Light` member.

```cpp
// lib/src/bg2e/scene/reflection/LightComponentReflection.cpp (excerpt)
reflection::TypeRegistration<scene::LightComponent> _lightComponentReflection(
    [](reflection::TypeInfoBuilder<scene::LightComponent>& t) {
        t.displayName("Light Source");

        // light() has const and non-const overloads => static_cast disambiguation.
        t.object("light", "bg2e::base::Light",
                 static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
                 static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light))
            .displayName("Light")
            .category("Light")
            .tooltip("Light parameters, edited in place");
    });
```

Read-only variant: pass only the **const** reference getter — with no mutable
getter there is no write path, so the sub-object is read-only:

```cpp
t.object("light", "bg2e::base::Light",
         static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light))
    .displayName("Light");   // isReadOnly() == true
```

A consumer recurses into the sub-object at runtime:

```cpp
const reflection::PropertyInfo& prop = /* type == PropertyType::Object */;
const auto* objectInfo = reflection::TypeRegistry::get().type(prop.objectTypeName);
const void* sub    = prop.objectGetter(instance);
void*       subMut = prop.isReadOnly() ? nullptr : prop.objectMutableGetter(instance);
// recurse into objectInfo->properties addressing `sub` / `subMut`,
// but not beyond reflection::maxObjectDepth (3) levels.
```

**Key points:**
- The sub-object type must itself be registered under the given key
  (`"bg2e::base::Light"`); the key is resolved at consumption time, so
  static-init order between the two `.cpp` files does not matter. An
  unregistered key is not an error — show a fallback label.
- Only `displayName()`, `category()` and `tooltip()` chain onto `object()`:
  `ObjectBuilder<T>` has no `slider()`/`range()`/`enumValue()`/…, so
  `t.object(...).slider()` **fails to compile** by design.
- Scalar types must use `property()`; `object()` static-asserts on scalar
  property types and vice versa.
- There is no replace-the-sub-object setter; the parent's `getter`/`setter`
  stay empty for object properties.
- Depth limit: `reflection::maxObjectDepth` is 3 (root instance = depth 0);
  `TypeRegistry::validateObjectDepth()` checks it once everything is
  registered.

---

## Common pitfalls

| Symptom | Cause | Fix |
|---------|-------|-----|
| `unsupported property type` compile error | Accessor value type not in the [map](reference.md#propertytype--c-type-map) | Add a `propertyTypeOf` branch or reflect a supported type. |
| `getter and setter must use the same value type` | Getter returns `X` but setter takes `Y` | Make the accessor pair symmetric (after `ValueT` normalization). |
| `bad_any_cast` at runtime | `std::any_cast` with a type different from the stored value | Cast to the normalized value type the getter returns (by value). |
| Reflection never appears | Static-init object was in a TU the linker dropped | Register from a `.cpp` that is always linked, or call manual `registerType`. |
| Wrong key lookup returns `nullptr` | Used C++ name where `staticTypeName()` differs (e.g. `"Transform"` not `"TransformComponent"`) | Query by the actual registry key. |
| Setter looks missing for a `const&` getter | Getter-only overload was used | Pass the setter as the third argument. |

**Design reminders:**
- Editor choice never implies a constraint, and a constraint never forces an
  editor — keep the calls separate and explicit.
- Reflection is optional: never assume `type()` returns non-null.
- Only member function pointers are supported in v1 (no free functions or
  lambdas as accessors).
