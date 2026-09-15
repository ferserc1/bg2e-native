# ActionInfo

**Header:** `<bg2e/reflection/Action.hpp>`
**Namespace:** `bg2e::reflection`

Describes a parameterless action exposed by a reflected type, rendered as buttons
by `ui::ReflectionWidget`. Two kinds of callables are supported: member function
pointers (the wrapped method's return value is discarded, so fluent methods like
`TransformComponent* setIdentity()` work directly) and lambdas/functors invocable
with `T*`, which receive the component instance.

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

Accepted signatures:

- **Member function pointers**: `R (T::*)()` and `R (T::*)() const` for **any**
  return type `R` (including `T*`, `void`, `bool`, …). The
  `std::function<void(void*)>` wrapper drops the result.
- **Callables**: any lambda or functor invocable with `T*`
  (`std::is_invocable_v<F, T*>`). The closure receives the component instance,
  which allows complex actions to live in the reflection registration code
  instead of the component:

```cpp
t.action("resetTwice", [](scene::TransformComponent* comp) {
        comp->setIdentity();
        comp->setIdentity();
    })
    .displayName("Reset Twice")
    .category("Transform");
```

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
- [quick_start](quick_start.md#recipe-7-actions-parameterless-methods-and-lambdas) — example.
