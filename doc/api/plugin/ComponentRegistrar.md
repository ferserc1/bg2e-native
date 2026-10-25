# ComponentRegistrar

**Header:** `<bg2e/plugin/ComponentRegistrar.hpp>`

**Namespace:** `bg2e::plugin`

A noncopyable, plugin-scoped registration interface supplied only during
`Plugin::registerComponents`. Do not store the registrar or use it from another
thread or after that callback returns. Use it instead of namespace-scope
`RegisterComponent`, `TypeRegistration` or direct singleton mutation in a plugin.

## Public API

```cpp
using Result = scene::ComponentFactoryRegistry::RegistrationResult;

Result registerComponent(const std::string& type,
    scene::ComponentFactoryRegistry::DefaultCreator factory,
    std::optional<reflection::TypeInfo> metadata = std::nullopt);

template <typename T>
Result registerComponent(std::optional<reflection::TypeInfo> metadata = std::nullopt);

Result registerReflection(reflection::TypeInfo metadata);

template <typename Base, typename Derived>
Result registerSubtype(std::string base, std::string key,
    std::string displayName, std::string type = {});

Result onStop(std::function<void()> callback);
ModuleHandle module() const;
```

## Register a component and its reflection together

For a plugin-private component implementing the normal scene Component contract:

```cpp
bg2e::reflection::TypeInfoBuilder<RotationComponent> type(
    RotationComponent::staticTypeName());
type.displayName("Rotation");
type.property("speed", &RotationComponent::speed, &RotationComponent::setSpeed)
    .category("Rotation").drag().range(-360.0, 360.0);
registrar.registerComponent<RotationComponent>(type.build());
```

The template creates a default factory returning `new T()` under
`T::staticTypeName()`. The explicit overload accepts a `std::function<scene::Component*()>`.
It must return a new detached component of the requested persistent type; normal
scene deserialization validates detached ownership and the returned type.
Registration stores the callable without constructing an instance. Keep factory
creation compatible with early graphics-free loading and the eventual host use.

If metadata is provided, `metadata.typeName` must match the factory ID. A collision
in either factory or supplied reflection prevents the contribution from replacing
existing data. Factory/reflection insertion is atomic for that contribution;
failure of its second half reverts its first half. The Plugin's component ID list
is appended only after full success. `staticTypeName()`/`typeName()` IDs are
persistent keys; RTTI names are not registration IDs.

Factory-only registration is supported, but the engine ComponentInspector's Add
Component list intersects factories with reflection metadata. Supply paired
metadata for inspector/editor workflows. Ordinary scene registry entry points
remain the creation/deserialization surface; the plugin's component implementation
must still implement any serialization/resource behavior it needs. Registration
does not synthesize serialization.

## Results, duplicates and rollback

| Result | Effect |
|--------|--------|
| `Registered` | The contribution was inserted and attributed to this plugin's generic registration owner. |
| `Duplicate` | Existing contribution preserved; this attempt adds its name to the load report. The conflicting plugin does not own or remove the original entry. |
| `Invalid` | Empty ID/factory, mismatched metadata or unavailable registrar state; a structured diagnostic is appended. |

A plugin can continue registering later contributions after `Duplicate` or
`Invalid`. A load with such diagnostics returns `LoadedWithWarnings`; zero new
component types is also valid. Unexpected exceptions escaping the registration
callback roll back that load attempt's owned contributions. Plugin code must
release external uses it created before throwing.

The loading convenience overload logs one warning per duplicate attempt, including
repeated attempts at the same ID. Loading with a caller-provided duplicate vector
appends names silently. Auxiliary reflection/subtype conflicts are also reported;
subtype labels use `base:key`. These are not invalid-registration errors.

## Supporting reflection and polymorphic subtypes

Register nested value objects and hierarchy metadata through the same owner:

```cpp
registrar.registerReflection(values.build());
registrar.registerReflection(base.build());
registrar.registerReflection(smooth.build());
registrar.registerSubtype<SampleMode, SmoothMode>(
    "tutorial.SampleMode", "smooth", "Smooth", "tutorial.SmoothMode");
```

Here `values`, `base` and `smooth` are TypeInfoBuilder instances for plugin-private
supporting types. Subtypes use a default-constructible Derived and a polymorphic
Base. Empty `type` falls back to the subtype key; empty display name falls back to
the key. The registrar has no custom-subtype-factory overload. Reflection builders
retain their existing property/object/subtype behavior; see
[Builder](../reflection/Builder.md).

Owned type/subtype insertion never replaces another owner's metadata. Registry
owner removal destroys these executable callbacks before native release.
Reflection pointers and executable copies are invalid after removal; discard
external caches under the [unload contract](unloading.md).

## Stop callbacks and module leases

`onStop` accepts a nonempty callback, invoked once when preparation begins, before
`onPrepareUnload`. Use it to cancel/join plugin-owned jobs and disconnect external
subscriptions. Callbacks run in registration order within a plugin; session
shutdown visits plugins in reverse load order. A callback failure is recorded and
does not skip later callbacks; failed cleanup prevents physical module release.

`module()` returns a copyable [ModuleHandle](Types.md#modulehandle) lease for known
external work. This does not automatically track components or give permission
to leave objects alive after retirement. All jobs and native references remain
subject to programmer-owned cleanup.
