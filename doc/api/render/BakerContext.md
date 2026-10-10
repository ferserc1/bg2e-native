# BakerContext

**Header:** `<bg2e/render/BakerContext.hpp>`
**Namespace:** `bg2e::render`

Non-copyable base for shared bake-scene services. Its constructor is protected;
use `IntegratedBakerContext` or `StandaloneBakerContext`, owned by `std::shared_ptr`.
Factories use the inherited `std::enable_shared_from_this<BakerContext>`;
calling a factory on a context without shared ownership can throw `std::bad_weak_ptr`.

| Member | Return | Contract |
|---|---|---|
| `engine() const` | `Engine*` | Non-owning engine pointer. |
| `rootNode() const` | `scene::Node*` | Non-owning root fixed at construction. |
| `virtual ~BakerContext()` | — | Polymorphic destruction. |

The protected constructor rejects null engine/root with `std::invalid_argument`
and an engine without ray-tracing support with `std::runtime_error`. The engine
must remain alive through baker/context destruction and deferred resource cleanup.
The integrated context does not retain the root; standalone retains its `Scene`.
A baker retains its context and target node.

Sources: [header](../../../lib/include/bg2e/render/BakerContext.hpp),
[implementation](../../../lib/src/bg2e/render/BakerContext.cpp).
See [context architecture](../../architecture/lightmaps/contexts.md).
