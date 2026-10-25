# Plugin system implementation plan

Status: proposed implementation; no engine or CMake changes are implemented by this plan.

## Goals and agreed contract

- Introduce `bg2e::plugin`, keeping `base::DynamicLibrary` as the low-level library owner.
- A plugin exports `getPluginInstance`. Use a small versioned C compatibility descriptor before invoking the C++ interface; require a compatible engine SDK, architecture, toolchain, standard library and runtime ABI.
- Provide plugin identity, name, description, version, extensible metadata and explicit license information without assuming GPL for plugins.
- Register scene components without GPU initialization. Track successfully registered component identifiers and their owner automatically in the registration operation.
- Preserve existing registrations on identifier collisions. Skip each duplicate, continue with the remaining components and report one warning per omitted duplicate. Provide a silent overload that appends duplicate names to a caller-provided vector.
- Public plugin loading functions return structured errors and never propagate exceptions for recoverable failures. Distinguish a non-plugin library from an incompatible or broken plugin.
- Support explicit files, directories and automatic loading from `PlatformTools::pluginPath()` before graphics startup.
- Support two-stage shutdown, reverse load order, and safe runtime unload with component discovery and module lifetime tracking.
- Add an application plus plugin example with automatic discovery, engine headers configured on the plugin target, and no manually implemented node-component listing UI.
- Isolate plugin example executables and their `plugins` directories on Linux/Windows. macOS uses each application's existing bundle boundary.

## Phases

| Phase | Deliverable | Dependency |
| --- | --- | --- |
| [1. System implementation](phase-01-system.md) | Plugin API, loader, component ownership, diagnostics, lifecycle and unload safety | None |
| [2. Component plugin example and CMake utilities](phase-02-example-and-cmake.md) | Reusable plugin helper, isolated application output support and automatic-loading example | Phase 1 |
| [3. Isolate the existing dynamic-library example](phase-03-dynamic-library-isolation.md) | Remove the raw library example's shared-directory contamination | Phase 2 utility support |

## Repository findings

- `ComponentFactoryRegistry::contains()` already exists. `registerComponent()` currently overwrites entries; it needs insertion results and owner-aware removal.
- `ui::ComponentInspector` builds its add-component list from reflection types intersected with registered component factories. Factory registration alone is insufficient for the current editor workflow.
- `Node` owns components through `shared_ptr`; removing them from a tree does not prove they are destroyed. Cloning, detached nodes, undo history, weak control blocks and stored callbacks also affect safe unload.
- `MainLoop::runInternal()` calls `ensureRuntimeAvailable()` and `prepare()` before initialization. `prepare()` acquires a backend; loading must precede it. The execution object itself is currently constructed by `run()` before entering `runInternal()`.
- `DrawGraphicsExecution::cleanup()` combines delegate/render-loop, UI and engine teardown. Explicit lifecycle boundaries are needed for plugin callbacks and component resource destruction.
- `bundle_app()` and `bundle_app_sdl()` in `cmake/utils.cmake` hard-code `PRODUCT_DIR`; neither accepts a per-application output path. Some implementation references also rely on the caller's `APP_TARGET_NAME` instead of the parsed target argument.
- `bundle_resources()` mostly derives destinations from `TARGET_FILE_DIR`, but establishes Windows resource-copy targets while the application helper runs. Changing output properties afterward is insufficient: the resource deduplication key may already describe the old directory.
- `bundle_lib()` only copies libraries on macOS. Windows isolated applications need runtime DLL staging; Linux needs a verified runtime search path/dependency arrangement.
- `examples/plugins/dynamic_library` currently builds a raw `getMessage` library and copies it to the application's `plugins` directory. Because the executable uses the shared output directory on Linux/Windows, the copied library is shared by other examples.
- `examples/CMakeLists.txt` explicitly adds example subdirectories; a new example requires an explicit entry.
- The repository instructions mention `skills.md`, but that file was not found during inspection. Recheck before implementation if it becomes available.

## Execution constraints

- Documents and generated source content use English.
- This change only creates planning documents. The future CMake work is explicitly requested for these examples and utilities; unrelated build-system changes are outside scope.
- Do not compile unless the user explicitly requests compilation. Validation below describes future acceptance checks, not checks already performed.
- Keep changes out of `bg2e::gpu` and unrelated rendering APIs. Use the repository's current `draw` integration points discovered in source.
- Native plugin faults such as invalid memory access, `abort()` or crashing static initializers cannot be contained by an in-process exception boundary. Recoverable loader/API errors must not terminate the application; process isolation is outside this design.
