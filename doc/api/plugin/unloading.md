# Unloading and scene cleanup

**Headers:** `<bg2e/plugin/Plugin.hpp>`, `<bg2e/plugin/PluginManager.hpp>`

**Namespace:** `bg2e::plugin`

## Programmer responsibility

**Before unloading a plugin, the programmer must remove its components from every
active scene, destroy all remaining objects and executable references supplied by
that plugin, and stop its work.** Calling `unload` asserts that this cleanup has
been completed. The host does not discover all scenes or count every native use.
If any use survives physical unload, calling it or destroying it can execute
unmapped code: the result is undefined behavior and the application may crash.

This contract applies equally to instances constructed by the component registry,
direct construction, clones and plugin-defined polymorphic reflection subtypes.
It also applies to automatic shutdown, not just explicit unloading.

Before requesting unload:

1. Stop frame production and background scene parsing, cancel/join plugin jobs,
   and drain GPU work. Perform cleanup on the manager's main thread, outside all
   plugin call stacks, while services needed by component destructors still exist.
2. Call `Plugin::removeComponents(id, root)` for **every** active scene root.
   Include additional scene/view roots owned by tools or editors.
3. Destroy detached nodes/components and clones, and release selection, undo/redo,
   clipboard, cached UI objects and all external owning references. Release weak
   references too when their control blocks/deleters were instantiated in plugin
   code; destruction of such a block after unload may execute plugin code.
4. Discard cached reflection/subtype pointers and copied properties, actions,
   factories, `std::function` callbacks and plugin-origin exception objects.
   Disconnect subscriptions and destroy queued callbacks. Release explicit
   `ModuleHandle` leases when their work is complete.
5. Request unload from host code. Do not invoke unload or `collect` from plugin
   code that would return through the library being closed, from a plugin
   destructor, or while another thread is executing plugin code.

A plugin's `onPrepareUnload`, `onUnload` and registered stop callbacks must fulfill
the same contract for objects, jobs and subscriptions owned internally by the
plugin. A plugin that throws during registration must not leave external native
uses behind when its failed load is rolled back.

## Remove registered components from a scene

```cpp
// Relevant declarations from the public classes:
class Plugin {
public:
    const std::vector<std::string>& componentTypes() const noexcept;
    size_t removeComponents(scene::Node* root) const;
    static size_t removeComponents(PluginId plugin, scene::Node* root);
};

class PluginManager {
public:
    size_t removeComponents(PluginId plugin, scene::Node* root);
    std::vector<std::string> componentTypes(PluginId plugin) const;
};
```

The instance accessor returns a reference valid only while that Plugin instance
exists; the manager accessor returns a host-owned copy of the identifiers.

The registrar appends a persistent component ID to the plugin's list only after
its factory and optional reflection registration both succeed. Duplicate,
invalid and rolled-back contributions are omitted. A conflicting plugin never
owns the original registration and its cleanup must not remove that component.

The helper walks the root and its descendants, including disabled nodes. It first
collects matching components, then removes them via `Node::removeComponent`, so
normal removal callbacks run without mutating the traversal. Its temporary owning
references are released before returning. It returns the number of removal calls
completed; a null root is an empty operation for a valid loaded plugin.

**This helper removes scene membership, not external ownership.** It does not
clear detached nodes, history or references stored elsewhere. Do not create,
clone or attach more plugin objects after cleanup begins. The caller must keep
the plugin loaded until cleanup has returned for all scenes and all component
and control-block destructors have finished.

The ID-based helper rejects unknown/retired plugins, wrong-thread calls, reentrant
manager callbacks and active background registry readers. Allocation or component
removal callback failures can throw; do not proceed with unloading after failed
scene cleanup. The instance method can be used by plugin lifecycle callbacks at
their host-provided stopped boundary.

## Explicit unload sequence

The following host-side sequence assumes that frame/background producers are
already stopped, GPU work is drained, and both roots belong to the application:

```cpp
#include <bg2e/plugin/all.hpp>
#include <bg2e/scene/Node.hpp>

// id is the PluginId obtained when the plugin was loaded.
bg2e::plugin::Plugin::removeComponents(id, firstSceneRoot.get());
bg2e::plugin::Plugin::removeComponents(id, secondSceneRoot.get());

// Application-specific operation, not an engine API: destroy external plugin
// objects, history, weak control blocks, cached metadata and queued callbacks.
releaseExternalPluginReferences();

bg2e::plugin::UnloadOptions options;
options.services.graphics = graphicsEngine; // nullptr when unavailable
options.services.stopped = true;            // caller guarantee, not a wait request
const auto report = bg2e::plugin::PluginManager::get().unload(id, options);
```

`releaseExternalPluginReferences` and the root/engine variables above stand for
application-owned code and state. The cleanup operation itself must not leave a
plugin-defined callback/deleter alive in its own captures after the library closes.

The manager stops further registry creation, invokes preparation/stop callbacks,
removes owned component/reflection/subtype registrations, invokes `onUnload`,
destroys the plugin instance using its exported function, and closes the native
library after those callbacks and destructors have returned. Reflection pointers
and executable copies become invalid when their registrations are removed; a
registry revision change is not a replacement for discarding external caches.

| Result | Meaning |
|--------|---------|
| `Unloaded` | The manager released its native loader reference. The plugin instance and registrations are gone. |
| `InUse` | `RefuseIfInUse` found matching components in the optional supplied `roots`. It cannot check roots or references the programmer omitted. |
| `Busy` | The caller did not provide a stopped boundary, registry readers remain active, or a manager operation is reentrant. |
| `Pending` | Registrations and the plugin instance are gone, but explicit `ModuleHandle` leases still delay physical release. Release those leases, then call `collect()` at a host safe point. |
| `Failed` | Cleanup failed; the module is not physically released. Inspect the failure and do not treat it as a successful unload. |

`RemoveComponents` remains an optional convenience policy for the roots supplied
in `UnloadOptions`. It is not a substitute for cleaning every active scene and
releasing external references. `RefuseIfInUse` is likewise a check of supplied
roots only, not an automatic lifetime census. With the default empty `roots`,
the manager trusts that the programmer has already called the cleanup helper.
An optional `releaseReferences` hook must be host-owned and release its own
plugin-owned captures as well as application state before returning.

A `ModuleHandle` delays code unmapping; it does not keep the plugin instance or its
services alive after retirement. Do not use outstanding leases as permission to
keep calling plugin objects after `unload`. No forced-unload option bypasses
known leases, readers or failed cleanup.

## Automatic MainLoop shutdown

MainLoop unloads only the plugins first discovered in its own session. Explicitly
loaded plugins remain the responsibility of their explicit owner.

For automatic plugins, implement `Application::preparePluginShutdown` to call the
cleanup helper for each loaded plugin and each active root, then release external
plugin references. This hook runs after plugin preparation with workers stopped
and graphics services still present. Delegates/UI are cleaned next, registrations
are retired, the engine is cleaned, and plugin finalization/native release follow
in reverse load order.

References retained by application delegates, history or weak control blocks
must not survive this boundary. The loader does not preserve component libraries
for the rest of the process as a fallback for missing application cleanup.

## Related API

- [Plugin](Plugin.md) and [PluginManager](PluginManager.md).
- [Contexts, reports, policies and module leases](Types.md).
- [Full lifecycle/service availability](lifecycle.md).
- [PluginComponentsVisitor](PluginComponentsVisitor.md).
