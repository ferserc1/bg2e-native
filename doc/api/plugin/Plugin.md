# Plugin

**Header:** `<bg2e/plugin/Plugin.hpp>`

**Namespace:** `bg2e::plugin`

The author interface owned by [PluginManager](PluginManager.md). The manager owns
the native library, instance, copied metadata and contribution owner ID. Application
loading returns a managed numeric ID, not an owning `Plugin*`.

## Public API

```cpp
virtual ~Plugin();
virtual PluginInfo info() const = 0;
virtual void registerComponents(ComponentRegistrar&, const PluginLoadContext&) = 0;
virtual void onPrepareUnload(const PluginUnloadContext&);
virtual void onUnload(const PluginUnloadContext&);

const std::vector<std::string>& componentTypes() const noexcept;
size_t removeComponents(scene::Node* root) const;
static size_t removeComponents(PluginId plugin, scene::Node* root);

static LoadReport loadPlugin(const std::filesystem::path& path) noexcept;
static LoadReport loadPlugin(const std::filesystem::path& path,
    std::vector<std::string>& duplicates) noexcept;
static ScanReport loadPlugins(const std::filesystem::path& directory) noexcept;
static ScanReport loadPlugins(const std::filesystem::path& directory,
    std::vector<std::string>& duplicates) noexcept;
```

| Member | Contract |
|--------|----------|
| `info()` | Supplies [PluginInfo](Types.md#plugininfo); the manager copies it after instance creation. Nonempty `id` and `name` are required. |
| `registerComponents(...)` | Runs once per successful load attempt. Register contributions through the supplied registrar; graphics are absent. An empty contribution list is valid. |
| `onPrepareUnload(...)` | Optional one-shot preparation callback while graphics may still be available. Check the actual context. |
| `onUnload(...)` | Optional one-shot finalization after registration retirement. Automatic shutdown has already cleaned the graphics execution. |
| `componentTypes()` | Reference to IDs whose factory and optional reflection registrations both succeeded. It excludes duplicates, invalid entries and reverted insertions. Valid only while the instance lives. |
| `removeComponents(root)` | Includes disabled nodes, collects matching IDs, then calls normal Node removal callbacks. Returns completed removal calls; null root returns zero. It releases its temporary owning references before returning. |
| Static `removeComponents(id, root)` | Manager facade for application code using numeric IDs; rejects invalid/retired IDs, wrong thread, reentrancy and active registry readers. Cleanup can throw; do not unload after incomplete cleanup. |
| Static loading methods | Forward to the manager's explicit-load APIs. Directory facades use the default missing-directory-as-error behavior. |

The no-vector loading overloads warn once per omitted duplicate attempt. The vector
overloads append names without printing duplicate warnings; they do not clear the
caller's vector. See [reports](Types.md#loadreport-and-loadstatus) and
[registration](ComponentRegistrar.md).

Use `removeComponents` for every active scene, then destroy external objects,
weak control blocks and callbacks before unload. The helper removes membership,
not external ownership; see the [caller contract](unloading.md).

## BG2E_DEFINE_PLUGIN

Use once at namespace scope in one implementation file after defining a
publicly default-constructible concrete Plugin class:

```cpp
BG2E_DEFINE_PLUGIN(my_application::ApplicationPlugin)
```

The generated C-linkage exports are:

| Export | Behavior |
|--------|----------|
| `getPluginApi()` | Returns the module's immutable `PluginApi` descriptor, including SDK/header fingerprint and destruction function. |
| `getPluginInstance()` | Allocates the concrete class, returns its Plugin base pointer as `void*`, catches factory exceptions and returns null on failure. |
| `bg2eDestroyPlugin(void*)` | Deletes through the virtual Plugin interface inside module code, with a defensive exception boundary. |

`BG2E_PLUGIN_EXPORT` controls plugin symbol visibility independently from the
engine's import/export macro; `BG2E_PLUGIN_CALL` selects the bootstrap calling
convention (`__cdecl` on Windows). The [CMake helper](CMake.md) defines
`BG2E_PLUGIN_BUILD` so Windows imports engine classes while exporting plugin entry
points. Do not define the engine's producer macro on the module.

The macro creates no instance or registrations during module static initialization.
Do not manually implement or copy descriptor versions, query functions or deletion
entry points. Do not invoke these exports directly from application code; loading
must validate the descriptor before creating a C++ instance. C linkage applies to
bootstrap exports, not to compiler-independent C++ object interchange.

Keep constructors free of registration and graphics work; place registration in
`registerComponents`. Native faults or terminating destructors cannot be recovered
by these C++ exception boundaries. Details: [SDK contract](Sdk.md) and
[lifecycle](lifecycle.md).
