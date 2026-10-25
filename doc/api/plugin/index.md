# Plugin API

`bg2e::plugin` loads native modules linked to the application's shared engine,
checks SDK/ABI compatibility before invoking their C++ interface, and registers
scene components and reflection without initializing graphics.

```cpp
#include <bg2e/plugin/all.hpp>
```

Plugin authors implement `Plugin::info` and `Plugin::registerComponents`, then use
`BG2E_DEFINE_PLUGIN` once to generate the descriptor, factory and destruction
exports. SDK headers and CMake supply compatibility values automatically.
MainLoop discovers modules in `PlatformTools::pluginPath()` before graphics startup.

## Module areas

| API | Documentation | Purpose |
|-----|---------------|---------|
| `Plugin`, `BG2E_DEFINE_PLUGIN` | [Plugin](Plugin.md) | Author interface, generated exports, load facades and scene cleanup. |
| `PluginManager` | [PluginManager](PluginManager.md) | Explicit loading, scans, managed IDs, session ownership and unloading. |
| `ComponentRegistrar` | [ComponentRegistrar](ComponentRegistrar.md) | Owned component/reflection/subtype registration and stop callbacks. |
| Metadata, contexts, reports, IDs and leases | [Types](Types.md) | Information exchanged with the host and structured outcomes. |
| `PluginApi`, SDK headers | [SDK and ABI](Sdk.md) | C bootstrap descriptor, exact compatibility checks and native-loader limits. |
| `PluginComponentsVisitor` | [PluginComponentsVisitor](PluginComponentsVisitor.md) | Collect/remove plugin components, including disabled nodes. |
| MainLoop callbacks/services | [Lifecycle](lifecycle.md) | Discovery, preparation, application cleanup and finalization order. |
| `bg2e_add_plugin`, SDK distribution | [CMake integration](CMake.md) | Repository and standalone targets sharing one engine instance. |

## Unload ownership

**The programmer must remove plugin components from every active scene and release
all other plugin uses before unload.** The loader does not count every component,
clone, callback or weak control block and does not permanently retain a module
merely because it registered types. Violating this contract causes undefined
behavior and may crash the application.

Read [Unloading and scene cleanup](unloading.md) before explicit unloading or
implementing `Application::preparePluginShutdown` for automatic plugins.

## Where to go next

- [Quick start](quick_start.md) — author a plugin, register types and load explicitly.
- [API reference](reference.md) — headers, signatures, enums and related topics.
- [Plugin tutorial](../../tutorials/plugins/plugin.md) — move the reflection tutorial's components into an automatically discovered module.
- [Raw DynamicLibrary tutorial](../../tutorials/plugins/dynamic_library.md) — the separate single-symbol ownership example.
