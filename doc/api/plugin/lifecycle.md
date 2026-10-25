# Plugin lifecycle

**Headers:** `<bg2e/plugin/Plugin.hpp>`, `<bg2e/plugin/PluginManager.hpp>`

**Host hook:** `<bg2e/app/Application.hpp>`

**Namespace:** `bg2e::plugin` (with `bg2e::app` integration)

## Automatic discovery

The application installs its delegates with `Application::init` before calling
MainLoop::run. MainLoop creates the plugin session cleanup guard **before** scanning
`PlatformTools::pluginPath()`. Discovery precedes graphics-execution construction,
backend acquisition, SDL/window setup, graphics initialization and scene creation.
Therefore registration must not need a GPU, and application code must not expect
automatic plugin components to exist during its earlier `Application::init`.
Create them during later scene initialization, as the plugin tutorial does.

| Platform | Default directory |
|----------|-------------------|
| Linux/Windows | `plugins` beside the executable, including its actual Windows configuration directory. |
| macOS | `<app>.app/Contents/Resources/plugins`. |

Scans are deterministic, nonrecursive and use platform suffixes. A missing default
directory is empty success. Recoverable file failures are reported without stopping
other candidates. Raw libraries without the factory are `NotAPlugin`; no successful
engine-plugin registration is implied. See [PluginManager](PluginManager.md).

Repeated scans do not duplicate active canonical paths/plugin identities. A scan
does not adopt explicit modules into its session. Repeated runs have new session
IDs; cleanup of one session must not silently unload an explicit owner's module.
There is no guarantee of live binary replacement/hot reload while an older native
mapping or external loader reference remains.

## Callback/service matrix

| Callback/boundary | Graphics | Work state and responsibility |
|-------------------|----------|-------------------------------|
| `info()` / `registerComponents` | Absent | Metadata/registration only. Load context has null graphics and default `stopped=false`. |
| Registrar `onStop` callbacks | No context argument | Called once to cancel/join owned jobs and revoke subscriptions; acceptingWork is already false. |
| Automatic `onPrepareUnload` | Engine pointer only if initialized | MainLoop has joined its workers, stopped queued work and attempted to drain GPU users; check the actual `services.stopped` value. |
| `Application::preparePluginShutdown` | Graphics services still exist when initialized | Application removes plugin components from all roots and releases external references. |
| Automatic `onUnload` | Null | Execution/graphics cleanup has finished. Do not rely on graphics here. |
| Runtime preparation/finalization | Caller-supplied optional pointer | Runtime unload requires a genuinely stopped boundary; the engine may still exist for both callbacks. |
| Initialization rollback callbacks | Null | Reason `InitializationFailure`; only partial contributions may exist. |

Context service pointers are borrowed; they do not grant ownership or extend
engine lifetime. If device draining fails, a preparation context can report
`stopped=false`; do not infer safety from the callback name alone.

## Automatic shutdown order

For the plugins first discovered in the current MainLoop session:

1. Block new MainLoop queued updates/loading, join async workers, and discard
   pending host callbacks/loader completions while native code remains available.
2. Stop plugin-owned work via registered stop callbacks; drain GPU work.
3. Invoke `onPrepareUnload` in reverse load order.
4. Call `Application::preparePluginShutdown` so the application clears UI/selection,
   removes plugin components from **every** active root and releases external uses.
5. Clean render delegates and UI while the engine still exists. Retire owned
   factories, reflection/subtype entries and executable registry callbacks.
6. Clean/destroy the graphics execution and window.
7. Finalize automatic plugins in reverse load order with `onUnload`, destroy their
   instances through module-owned destruction functions, then collect native loader
   references at the host safe point when known leases/cleanup permit.

The outer session guard also covers failed startup after partial discovery and
execution failures. Stop/preparation/finalization are one-shot; retirement is
idempotent. Failures do not skip attempts to clean the remaining modules. A failed
plugin cleanup prevents its physical native release; known leases can delay release.
These protections are not a detector for application references it never disclosed.

An [unload-contract](unloading.md) violation can crash even during application
object destruction after MainLoop::run returns. Automatic plugins are not pinned
for process lifetime merely because they registered types. Application cleanup is
required, not optional bookkeeping.

## Explicit ownership and custom hosts

Explicit file/directory loads work without MainLoop or graphics. They have no
automatic session owner. Their caller owns preparation, scene/external-reference
cleanup and unload, including when the application otherwise uses MainLoop.
API loading/unload examples are in [PluginManager](PluginManager.md) and
[Unloading](unloading.md).

Custom hosts can use the manager session operations instead of MainLoop. They must
install a cleanup guard before discovery, stop producers/readers, supply actual
service availability, perform application cleanup before retiring registries,
finalize after execution teardown and collect outside all plugin call/destructor
stacks. Do not call session-management methods recursively from plugin callbacks.
