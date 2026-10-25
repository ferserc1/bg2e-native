# Plugin API reference

Every symbol below lives under `bg2e::plugin` except the declaration/configuration
macros. `<bg2e/plugin/all.hpp>` includes the public plugin module. Bootstrap helpers
under `plugin::detail` are implementation details, not author extension points.

## Classes and author entry point

| Symbol | Header | Reference |
|--------|--------|-----------|
| `Plugin` | `plugin/Plugin.hpp` | [Author callbacks, static loading and scene cleanup](Plugin.md). |
| `PluginManager` | `plugin/PluginManager.hpp` | [Loading, queries, registry reader leases, sessions, unload and collection](PluginManager.md). |
| `ComponentRegistrar` | `plugin/ComponentRegistrar.hpp` | [Factory/reflection/subtype contribution registration and stop callbacks](ComponentRegistrar.md). |
| `ModuleHandle` | `plugin/Types.hpp` | [Copyable known-use module lease](Types.md#modulehandle). |
| `PluginComponentsVisitor`, `PluginComponentsVisitor::Entry` | `plugin/PluginComponentsVisitor.hpp` | [Two-pass scene collection/removal](PluginComponentsVisitor.md). |
| `BG2E_DEFINE_PLUGIN(PluginClass)` | `plugin/Plugin.hpp` | [Generated C descriptor/factory/destruction exports](Plugin.md#bg2e_define_plugin). |

## Metadata and contexts

| Symbol | Header | Contract |
|--------|--------|----------|
| `PluginId`, `SessionId` | `plugin/Types.hpp` | [Process-local uint64 identities](Types.md#identifiers); persistent plugin identity is a separate string. |
| `PluginInfo` | `plugin/Types.hpp` | [Identity, version, provider, extensible metadata, license and optional SDK requirement](Types.md#plugininfo). |
| `PluginServices` | `plugin/Types.hpp` | [Borrowed optional graphics pointer and actual stopped-boundary flag](Types.md#services-and-contexts). |
| `PluginLoadContext` | `plugin/Types.hpp` | Numeric handle, persistent identity, module path and services. |
| `PluginUnloadContext` | `plugin/Types.hpp` | Load context plus `UnloadReason`. |
| `UnloadOptions` | `plugin/Types.hpp` | [Policy, services, optional roots and host reference-release hook](Types.md#unloadoptions-and-unloadreport). |

## Reports and enums

| Symbol | Reference |
|--------|-----------|
| `LoadReport`, `LoadStatus` | [Per-file load status/ID/diagnostics and loaded() semantics](Types.md#loadreport-and-loadstatus). |
| `ScanReport` | [Directory-operation status plus individual file reports](Types.md#scanreport). |
| `UnloadReport`, `UnloadStatus` | [Physical/logical retirement and failure outcomes](Types.md#unloadoptions-and-unloadreport). |
| `UnloadPolicy` | `RefuseIfInUse`, `RemoveComponents`; both have caller-supplied root scope only. |
| `UnloadReason` | `Runtime`, `Shutdown`, `StartupFailure`, `InitializationFailure`. |
| `ComponentRegistrar::Result` | Alias of scene registry `RegistrationResult`: `Registered`, `Duplicate`, `Invalid`. |

All report/context enums and structs above are declared in `plugin/Types.hpp`,
except the registrar result alias. Recoverable public loading/unload boundaries
return statuses; scene cleanup/query/session setup operations have their separate
throwing/thread contracts documented on their class pages.

## Bootstrap descriptor and SDK

| Symbol | Header | Purpose |
|--------|--------|---------|
| `PluginApi` | `plugin/Sdk.hpp` | [C-compatible descriptor layout and exact field matching](Sdk.md#pluginapi-bootstrap-descriptor). |
| `QueryPluginApi`, `CreatePlugin` | `plugin/Sdk.hpp` | C-bootstrap function pointer types used by the loader. |
| `engineSdk()` | `plugin/Sdk.hpp` | Engine-owned immutable descriptor fingerprint for diagnostics. |
| `sharedEngineIdentity()` | `plugin/Sdk.hpp` | Identity address of the resolved shared engine instance. |
| `BG2E_PLUGIN_CONTRACT_VERSION` | `plugin/Sdk.hpp` | Descriptor contract version, currently 1. |
| `BG2E_SDK_VERSION`, `BG2E_SDK_ABI_ID` | `plugin/Sdk.hpp` | SDK release and C++ ABI identities. |
| `BG2E_SDK_BUILD_ABI` | Generated `plugin/BuildConfig.hpp` | Additional ABI-option digest; source fallback supports custom SDK builds. |
| `BG2E_PLUGIN_EXPORT`, `BG2E_PLUGIN_CALL` | `plugin/Sdk.hpp` | Plugin visibility and bootstrap calling convention, independent of engine exports. |
| `BG2E_PLUGIN_BUILD` | CMake target definition | Windows engine-import selection for plugin targets. |

Authors use the declaration macro and paired SDK rather than implementing the
bootstrap exports manually. C linkage does not establish compiler-independent
C++ compatibility or protect against native initializer faults.

## Integration topics

- [Lifecycle](lifecycle.md) — MainLoop discovery and two-stage shutdown.
- [Unloading](unloading.md) — mandatory programmer-owned scene/reference cleanup.
- [CMake](CMake.md) — shared-engine resolution, module deployment and standalone SDK.
- [PlatformTools](../base/PlatformTools.md) — default plugin resource directory.
- [Quick start](quick_start.md) and [plugin tutorial](../../tutorials/plugins/plugin.md).
