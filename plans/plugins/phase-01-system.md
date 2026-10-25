# Phase 1: Implement the plugin system

## Outcome

Provide a graphics-independent registration and loading API, automatic MainLoop discovery and safe two-stage shutdown. Runtime unload must refuse or defer physical library release whenever plugin code can still be referenced.

## 1. Define API and ownership

- Add public headers under `lib/include/bg2e/plugin/` and implementations under `lib/src/bg2e/plugin/` for `Plugin`, `PluginManager`, contexts, reports, module handles and `ComponentRegistrar`.
- Make the manager own each `DynamicLibrary`, plugin instance, copied metadata, registration records and lifecycle state. A plugin must not own the library containing its own destructor.
- Expose `Plugin::loadPlugin(path)` and `Plugin::loadPlugins(directory)` as manager facades. Return managed identifiers/handles, not owning raw plugin pointers.
- Define plugin information: stable ID, display name, description, version, author/provider, URL, extensible metadata, license expression/text/URL and engine requirements.
- Define load/unload contexts carrying plugin ID/path, reason and available services. Graphics access is explicitly optional and absent during early registration.
- Define `registerComponents(ComponentRegistrar&, const PluginLoadContext&)`, `onPrepareUnload(...)` and `onUnload(...)`. Use an internal contribution-registration coordinator rather than a second public method duplicating `registerComponents`.
- Preserve a path for future extension categories without introducing dependencies from foundational namespaces to `plugin`.

## 2. Establish the binary contract

- Export the exact symbol `getPluginInstance` with C linkage and an explicit plugin export macro independent of the engine export/import macro.
- Add a versioned C descriptor query, such as `getPluginApi`, with structure size, contract version, engine SDK/ABI identity and plugin-owned destruction function.
- Validate the descriptor before obtaining/invoking the C++ instance. Reject missing entries, unsupported sizes/versions, null functions, null instances and incompatible engine requirements.
- Document a compatible SDK/toolchain ABI contract; C linkage does not make shared C++ objects compiler-independent. Initially use conservative SDK compatibility rather than promising compatibility based only on semantic version ranges.
- Cover architecture, standard-library ABI, runtime configuration, packing and relevant compiler settings. Windows must follow the repository's actual `MultiThreadedDLL` configuration rather than infer CRT mode solely from the configuration name.
- Plugins link to the application's shared engine, never an embedded duplicate engine. Do not allow static registration or graphics initialization in plugin global constructors.
- Make plugin factory/destruction C entry points catch their own exceptions; retain a defensive host boundary for plugin callbacks.

## 3. Make component registration incremental and owner-aware

- Keep `ComponentFactoryRegistry::contains()` and change registration to return `Registered`, `Duplicate` or an appropriate invalid-registration result using a non-overwriting insertion.
- Reject empty IDs and invalid factories without replacing existing entries. Preserve existing source call sites that ignore the returned result.
- Associate a generic registration-owner ID with successful entries. `scene` must not depend on `plugin`.
- Add owner-aware enumeration/removal and prevent one owner from removing another owner's entries.
- Implement a plugin-scoped registrar that records a component ID only after successful insertion. Use persistent `staticTypeName()`/`typeName()` IDs, not RTTI names.
- Provide component-loading overloads: one appends duplicate names to a supplied `std::vector<std::string>&` without printing warnings; the convenience overload delegates to it and emits one warning per duplicate attempt.
- Include other failures in a structured report without misclassifying them as duplicates. No exception is needed for normal registration conflicts.
- Continue with later registrations after duplicate or other supported per-component validation failures. A plugin with zero new types can still be loaded with warnings.
- On an unexpected callback failure that prevents initialization from completing, revoke only contributions from that load attempt and return initialization failure.

## 4. Integrate reflection and existing UI

- Extend ownership/removal to plugin reflection entries, subtype factories and executable callbacks. Never replace existing reflection metadata when the component factory was skipped as a duplicate.
- Provide one registrar operation that accepts the component factory and optional reflection metadata, so successful types appear in the existing `ComponentInspector` and Composer workflows.
- Make factory/reflection insertion atomic for an individual contribution, or revert that contribution if its second half fails.
- Prevent cached reflection pointers and UI callbacks from surviving module release. Notify/invalidate consumers when registry entries change.
- Preserve serialization/deserialization through existing scene registry entry points. Missing plugin types retain the existing unknown-component behavior.

## 5. Implement recoverable loading and diagnostics

- Define explicit outcomes including loaded, loaded-with-warnings, already-loaded, not-a-plugin, incompatible ABI/version, filesystem failure, library-load failure and initialization failure.
- Catch `DynamicLibrary` exceptions and callback failures inside public loading boundaries. Return errors with path and available plugin identity; avoid exceptions escaping into MainLoop or UI.
- Use error-code filesystem operations where appropriate. Handle missing/unreadable paths, invalid path kinds, directory iteration errors, unsupported files, missing exports and duplicate canonical paths/plugin IDs.
- Enumerate directory candidates non-recursively in deterministic order using platform library extensions. Continue after individual failures and preserve per-file reports.
- Treat a missing default directory as an empty successful scan; report other filesystem failures. A library without `getPluginInstance` is `NotAPlugin`, not a fatal startup error.
- Copy metadata and diagnostics into host-owned memory. Clear plugin-origin exception objects and callbacks before unloading their code.
- Design a minimal allocation-free fallback error state before declaring public loading entry points `noexcept`; optional strings/vector details may be unavailable under allocation failure. Do not turn allocation failure into `std::terminate` by merely adding `noexcept`.

## 6. Track uses and provide scene cleanup

- Add `PluginComponentsVisitor` under the integration module. Collect matching node/component/type entries, including disabled nodes; remove components in a separate pass through normal Node callbacks.
- Track module use beyond scene membership: detached components, clones, undo/clipboard references, factories, reflection callbacks, asynchronous work and shared/weak ownership control blocks.
- Establish an enforceable common creation/ownership contract covering registry construction, direct plugin construction and `clone()`. A counter attached only to `createDefault()` or decremented in the base destructor is insufficient.
- Choose and document the concrete host-owned lifetime mechanism before enabling physical runtime unload. Retain the module through completion of derived destructors, plugin deleters and control-block destruction; perform the final release later at a host safe point.
- Provide `RefuseIfInUse` and `RemoveComponents` policies. The latter operates on supplied scene roots and asks the application to release auxiliary references; return pending/in-use status if uses remain.
- Block new creation while unloading, cancel/join owned jobs and remove callbacks. No unsafe forced unload option.
- Serialize load/unload and live registry mutation on the main thread. Coordinate background parsing/loading as well as GPU work.

## 7. Add resource paths and MainLoop lifecycle boundaries

- Add `PlatformTools::pluginPath()` alongside shader/asset paths: executable sibling `plugins` on Linux/Windows; `Contents/Resources/plugins` in macOS bundles.
- Load the default directory on every application run before backend acquisition, runtime initialization and graphics callbacks. Move execution construction behind discovery if guaranteeing load before the `Engine` member is constructed.
- Install the plugin-session cleanup guard before discovery so partial loading and subsequent startup failures use the same cleanup path.
- Keep explicit loading usable without MainLoop. Define session adoption of previously loaded plugins and ownership of explicit plugins so repeated scans/runs neither duplicate registrations nor silently unload another owner's module.
- Add a stopped-execution boundary: join workers, block new work and drain GPU users before plugin preparation and component destruction.
- Invoke `onPrepareUnload` in reverse order before global graphics services are destroyed; let applications release scene/history/UI references while those services remain valid.
- Retire plugin registrations and resources before engine cleanup. Split or hook `DrawGraphicsExecution::cleanup()` to expose these stages without duplicating cleanup.
- For automatic plugins, run `onUnload` after execution cleanup, destroy plugin instances using their exported destruction functions and release libraries in reverse order when safe.
- During runtime unload, the graphics engine can still exist for both callbacks. Contexts report actual service availability and shutdown reason.
- Make callbacks one-shot and cleanup idempotent across normal exit, startup failure and exceptions. A failed cleanup must not prevent safe attempts to clean remaining plugins.
- If external uses survive application shutdown, report and retain their modules until process termination rather than unload executable code still in use.

## Acceptance checks

Perform source-level review first. Run executable/build checks only after explicit authorization to compile.

- Early explicit load and automatic load both work without an initialized GPU; default scan precedes backend acquisition.
- Two duplicate component attempts produce two warnings; the vector overload appends both names and emits no warnings. Later unique components still register.
- Duplicate types keep the original creator/reflection data and are never removed when the conflicting plugin unloads.
- Non-plugin libraries, missing symbols, null instances, incompatible contracts, unreadable directories and throwing callbacks return errors without terminating the application.
- A mid-initialization failure revokes only that attempt's contributions; a directory scan proceeds to other files.
- Plugin components deserialize and are available through the existing inspector without a custom component list.
- Discovery includes disabled nodes. Detachment alone cannot release a module held by clones, external ownership, weak control blocks or queued callbacks.
- Runtime unload waits/refuses safely, and final release occurs outside plugin destructor stacks.
- Verify callback order and service availability for normal exit, runtime unload, partial graphics initialization and failed startup; automatic libraries close in reverse order when no uses remain.
