# Plugin quick start

A recipe-oriented guide to `bg2e::plugin`. The [plugin tutorial](../../tutorials/plugins/plugin.md)
uses automatic discovery only; explicit loading examples belong to this API guide.

## 1. Define a plugin without manual exports or version queries

Put the class and declaration in one module implementation file:

```cpp
#include <bg2e/plugin/Plugin.hpp>
#include <bg2e/plugin/ComponentRegistrar.hpp>

namespace application {

class ApplicationPlugin final : public bg2e::plugin::Plugin {
public:
    bg2e::plugin::PluginInfo info() const override
    {
        bg2e::plugin::PluginInfo result;
        result.id = "org.example.application.components";
        result.name = "Application components";
        result.description = "Components supplied by the application module";
        result.version = "1.0.0";
        result.provider = "Application developer";
        result.licenseExpression = "GPL-3.0-or-later";
        return result;
    }

    void registerComponents(bg2e::plugin::ComponentRegistrar&,
        const bg2e::plugin::PluginLoadContext&) override
    {
        // An empty contribution list is a valid plugin.
    }
};

}

BG2E_DEFINE_PLUGIN(application::ApplicationPlugin)
```

Use your plugin's actual identity/license metadata. The declaration macro generates
the C query/factory/destruction exports and SDK fingerprint. Do not copy SDK versions
or implement `getPluginApi` yourself. The helper supplies compatible headers/options;
C++ interfaces still require a compatible compiler/standard library/runtime.

## 2. Give the module to its application

After creating the app with the repository or standalone bundle helper:

```cmake
bg2e_add_plugin(
    TARGET_NAME application_components
    APP_TARGET application
    ENGINE_TARGET bg2e
    SOURCES plugin/ApplicationPlugin.cpp)
```

Keep sources private to the module, outside the host's `src` glob. The helper stages
the module in the app's `plugins` resource directory, linked to the same shared
engine. MainLoop discovers that directory before graphics/scene initialization.
The generated standalone project already contains an empty plugin target; extend
it instead of adding a second starter. See [CMake integration](CMake.md).

## 3. Register components and supporting reflection

With a plugin-private RotationComponent and a TypeInfoBuilder for its existing
properties, register the factory and metadata in one operation:

```cpp
bg2e::reflection::TypeInfoBuilder<RotationComponent> type(
    RotationComponent::staticTypeName());
type.displayName("Rotation");
type.property("speed", &RotationComponent::speed, &RotationComponent::setSpeed)
    .drag().range(-360.0, 360.0);
registrar.registerComponent<RotationComponent>(type.build());
```

Use `registrar.registerReflection(builder.build())` for nested supporting types and
`registrar.registerSubtype<Base, Derived>(...)` for polymorphic modes. Do not use
static registration helpers or mutate the singletons directly. A duplicate leaves
the existing owner's data untouched and the plugin may continue with later types.
The committed IDs feed the Plugin cleanup helper. Details and result semantics:
[ComponentRegistrar](ComponentRegistrar.md).

## 4. Load one file explicitly when an application needs it

On the manager's main thread, without needing an initialized GPU:

```cpp
std::vector<std::string> duplicates;
const auto report = bg2e::plugin::Plugin::loadPlugin(modulePath, duplicates);
if (report.loaded())
{
    const auto id = report.handle; // Store this numeric managed ID, not a Plugin*.
    // Application-owned bookkeeping goes here.
}
```

`modulePath` is a platform library path chosen by the application. This overload
appends omitted duplicate names and prints no loader duplicate warnings. Omit the
vector to use the convenience warning behavior. Inspect `report.message` and
`report.errors` for errors, including descriptor differences. Non-plugin libraries
return `NotAPlugin`; incompatible contracts return `Incompatible`.

Explicit loads are not silently adopted by MainLoop: their owner must arrange
cleanup/unload even when other modules are discovered automatically.

## 5. Scan an explicit directory and inspect each result

```cpp
auto& manager = bg2e::plugin::PluginManager::get();
std::vector<std::string> duplicates;
const auto scan = manager.loadPlugins(pluginDirectory, duplicates, true);
for (const auto& file : scan.files)
{
    // Inspect file.status, file.handle, file.message, file.errors and duplicates.
}
```

The final true makes a missing directory empty success; omit it for an explicit
missing-directory error. Scans are nonrecursive and sorted. The aggregate status
is not a summary of every module's success: a failed individual file has its own
report and later files still load. See [PluginManager](PluginManager.md).

## 6. Clean scenes before unload

Before **any** explicit unload, stop producers/GPU work, call
`Plugin::removeComponents(id, root)` for every active scene, and release external
objects, clones, weak control blocks, callbacks and jobs. For automatic shutdown,
do that in `Application::preparePluginShutdown` while graphics are still present.

The loader trusts the programmer: missed uses may crash after the library closes.
Follow the complete [unload sequence](unloading.md); `RefuseIfInUse` only inspects
roots actually provided, and module leases are not an automatic lifetime census.
