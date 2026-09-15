# ActionInfo

**Header:** `<bg2e/reflection/Action.hpp>`
**Namespace:** `bg2e::reflection`

Describes a parameterless action exposed by a reflected type — a method with no
arguments, suitable for a future UI button. Only member function pointers are
supported; the wrapped method's return value is discarded, so fluent methods
(e.g. `TransformComponent* setIdentity()`) work directly.

```cpp
struct ActionInfo {
    std::string name;
    std::string displayName;
    std::string category;
    std::string tooltip;

    std::function<void(void*)> invoke;   // parameterless; instance as void*
};
```

---

## Members

| Member | Type | Description |
|--------|------|-------------|
| `name` | `std::string` | Programmatic id used by [`TypeInfo::action()`](TypeInfo.md). |
| `displayName` | `std::string` | Human-readable label (button text). |
| `category` | `std::string` | Grouping key for UI layout. |
| `tooltip` | `std::string` | Hover help text. |
| `invoke` | `std::function<void(void*)>` | Calls the underlying member function on a `void*` instance. |

Actions carry display metadata only — no `PropertyType`, editor, constraints, or
enum options (those are property concerns).

---

## Creating actions

Actions are produced by
[`TypeInfoBuilder<T>::action`](Builder.md#typeinfobuildert), which returns an
[`ActionBuilder<T>`](Builder.md#actionbuildert) for chained metadata.

```cpp
t.action("setIdentity", &scene::TransformComponent::setIdentity)
    .displayName("Set Identity")
    .category("Transform")
    .tooltip("Reset the transform to the identity matrix");
```

Accepted signatures: `R (T::*)()` and `R (T::*)() const` for **any** return type
`R` (including `T*`, `void`, `bool`, …). The `std::function<void(void*)>` wrapper
drops the result.

---

## Invoking an action

```cpp
using namespace bg2e;
scene::TransformComponent xform;

const reflection::TypeInfo* info =
    reflection::TypeRegistry::get().type("Transform");

if (const reflection::ActionInfo* a = info->action("setIdentity"))
{
    a->invoke(&xform);   // xform becomes the identity matrix
}
```

**Key points:**
- `invoke` expects a non-const `void*` pointing at a live object of the
  registered type.
- `info->action(name)` returns `nullptr` when no such action exists — check
  before invoking.
- The action is only safe on an instance of the exact type it was registered
  for; `void*` is unchecked by design.

---

## See also

- [Builder](Builder.md#typeinfobuildert) — declaring actions.
- [quick_start](quick_start.md#recipe-7-actions-parameterless-methods) — example.
