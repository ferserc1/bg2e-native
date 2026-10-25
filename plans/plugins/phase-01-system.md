# Phase 1: Implement the plugin system

## Outcome

Provide a graphics-independent registration and loading API, automatic MainLoop discovery and safe two-stage shutdown. Runtime unload follows an explicit programmer-owned cleanup contract. The programmer must remove plugin components from every active scene and release all external uses before requesting physical unload; violating this contract causes undefined behavior and may crash.

## 1. Define API and ownership

- Add public headers under `lib/include/bg2e/plugin/` and implementations under `lib/src/bg2e/plugin/` for `Plugin`, `PluginManager`, contexts, reports, module handles and `ComponentRegistrar`.
- Make the manager own each `DynamicLibrary`, plugin instance, copied metadata, registration records and lifecycle state. A plugin must not own the library containing its own destructor.
- Expose `Plugin::loadPlugin(path)` and `Plugin::loadPlugins(directory)` as manager facades. Return managed identifiers/handles, not owning raw plugin pointers.
- Define plugin information: stable ID, display name, description, version, author/provider, URL, extensible metadata, license expression/text/URL and engine requirements.
- Define load/unload contexts carrying plugin ID/path, reason and available services. Graphics access is explicitly optional and absent during early registration.
- Define `registerComponents(ComponentRegistrar&, const PluginLoadContext&)`, `onPrepareUnload(...)` and `onUnload(...)`. Use an internal contribution-registration coordinator rather than a second public method duplicating `registerComponents`.
- Preserve a path for future extension categories without introducing dependencies from foundational namespaces to `plugin`.

## 2. Establish the binary contract

- Provide `BG2E_DEFINE_PLUGIN(PluginClass)` (final spelling documented in the public API) to generate `getPluginApi`, `getPluginInstance` and plugin-owned destruction with the correct C linkage, calling convention, visibility and exception boundaries. Plugin authors implement their class and plugin metadata, not exported query functions or compatibility constants.
- Publish SDK version and ABI identity in SDK headers shipped with the engine. Keep plugin semantic version independent from contract and SDK/ABI versions. Generate any build-specific ABI configuration through the supported CMake helpers; do not ask authors to copy version strings.
- Export the exact symbol `getPluginInstance` with C linkage and an explicit plugin export macro independent of the engine export/import macro.
- Add the versioned C descriptor query `getPluginApi`, with structure size, contract version, engine SDK/ABI identity and plugin-owned destruction function.
- Validate the descriptor before obtaining/invoking the C++ instance. Reject missing entries, unsupported sizes/versions, null functions, null instances and incompatible engine requirements.
- Document a compatible SDK/toolchain ABI contract; C linkage does not make shared C++ objects compiler-independent. Initially use conservative SDK compatibility rather than promising compatibility based only on semantic version ranges.
- Define the descriptor as a C-compatible, fixed-width bootstrap contract with explicit size/version rules and no STL types. Reject unsupported contract versions before interpreting later fields.
- Compare the descriptor against the running engine's own identity: C contract version, conservative SDK ABI identity and module ABI configuration. Specify exact supported matches; semantic version ranges alone do not establish C++ ABI compatibility. Return structured mismatch details before invoking the C++ factory.
- Cover architecture, standard-library ABI, runtime configuration, packing and relevant compiler settings. Windows must follow the repository's actual `MultiThreadedDLL` configuration rather than infer CRT mode solely from the configuration name.
- Native loading resolves dependencies and may execute static initializers before descriptor validation. The host cannot prevent arbitrary plugin static initialization or contain native faults; the descriptor check protects subsequent API use, not the act of mapping an arbitrary library.
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

## 6. Provide scene cleanup and the explicit unload contract

- Add `PluginComponentsVisitor` under the integration module. Collect matching node/component/type entries, including disabled nodes; remove components in a separate pass through normal Node callbacks.
- Store successfully registered persistent component IDs on the base Plugin instance. Append only after the factory and optional reflection are fully committed; omit duplicates, invalid registrations and rolled-back contributions.
- Expose `Plugin::removeComponents(root)` plus an ID-based facade for hosts using managed plugin IDs. The caller must invoke cleanup for every active scene before explicit unload.
- Adopt the programmer-owned lifetime contract: the caller must also destroy detached objects, clones, history/clipboard/selection references, plugin-created weak control blocks, reflection caches, queued executable callbacks and plugin-origin exceptions, and cancel/join plugin jobs before unload. This applies to direct construction and cloning as well as registry creation.
- Do not implement an automatic complete lifetime census or permanently pin modules merely because they registered components/reflection. Unknown surviving uses are a contract violation and may crash when the library closes.
- Destroy owned registry metadata and callbacks while their module is still loaded; cached pointers and executable copies must be discarded by their consumers before removal. Keep revision notifications for live UI consumers.
- Keep `RefuseIfInUse` and `RemoveComponents` as checks/cleanup for supplied roots only. Neither proves that all scenes or external references were cleaned. Explicit module leases may defer physical release until a later host safe point; they do not keep a retired Plugin instance alive.
- Block new registry creation while unloading, cancel/join owned jobs, retire registrations, invoke lifecycle callbacks and destroy the Plugin instance before releasing the native library. Failed cleanup/readers/known leases must not be bypassed by a forced unload option.
- Serialize load/unload and live registry mutation on the main thread. The caller guarantees stopped producers, drained GPU work and no plugin stack active during physical release. Document the contract prominently in public headers and `doc/api/plugin/unloading.md`.

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
- The application must fulfill the same cleanup contract for automatic plugins in `Application::preparePluginShutdown`. Unknown uses surviving automatic shutdown are a contract violation; do not reintroduce process-lifetime retention as a fallback. Explicit leases or failed cleanup can still defer native release.

## Completion review

Review the source against every requirement above without adding tests, compiling, launching examples or executing build/configuration commands. Record the completed step and remaining work in `progress.md`, then stop for user feedback as specified in `summary.md`.
