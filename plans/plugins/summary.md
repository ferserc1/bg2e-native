# Plugin system implementation plan

Status: implementation phases 1–4 are complete; see [progress.md](progress.md) for review status, user feedback and validation limits.

## Goals and agreed contract

- Introduce `bg2e::plugin`, keeping `base::DynamicLibrary` as the low-level library owner.
- A public declaration macro generates `getPluginApi`, `getPluginInstance` and destruction exports. SDK headers provide versions and ABI identity automatically; plugin authors supply their class and metadata. Validate the C descriptor before using C++ objects, comparing contract version, SDK ABI identity and module ABI settings. Native static initializers remain outside host control.
- Provide plugin identity, name, description, version, extensible metadata and explicit license information without assuming GPL for plugins.
- Register scene components without GPU initialization. Track successfully registered component identifiers and their owner automatically in the registration operation.
- Preserve existing registrations on identifier collisions. Skip each duplicate, continue with the remaining components and report one warning per omitted duplicate. Provide a silent overload that appends duplicate names to a caller-provided vector.
- Public plugin loading functions return structured errors and never propagate exceptions for recoverable failures. Distinguish a non-plugin library from an incompatible or broken plugin.
- Support explicit files, directories and automatic loading from `PlatformTools::pluginPath()` before graphics startup.
- Support two-stage shutdown and reverse load order. Runtime/automatic unload trust an explicit programmer-owned cleanup contract: clean every scene through the Plugin helper, release external objects/weak control blocks/callbacks and stop jobs before physical unload. Unknown surviving uses may crash; registration alone must not permanently pin a module. See `doc/api/plugin/unloading.md`.
- Add `examples/tutorials/plugins` based on the reflection tutorial: the same scene and controls, with Rotation and Reflection Samples implemented in the plugin. Remove the custom manual reflection inspector, retaining the engine inspector/NodeEditor window. Load automatically from `plugins`.
- Provide clear reusable CMake utilities for user applications and standalone projects. Both standalone generators create an empty application plugin target ready for future components, sharing the application engine.
- Add API documentation under `doc/api/plugin` and the focused tutorial at `doc/tutorials/plugins/plugin.md`. Explicit loading is documented only in the API documentation.
- Isolate plugin example executables and their `plugins` directories on Linux/Windows. macOS uses each application's existing bundle boundary.

## Phases

| Phase | Deliverable | Dependency |
| --- | --- | --- |
| [1. System implementation](phase-01-system.md) | Plugin API, loader, component ownership, diagnostics, lifecycle and unload safety | None |
| [2. Plugin tutorial, CMake utilities and standalone generators](phase-02-example-and-cmake.md) | Reusable helpers, shared-engine resolution, empty standalone plugin and reflection-based tutorial | Phase 1 |
| [3. Isolate the existing dynamic-library example](phase-03-dynamic-library-isolation.md) | Remove the raw library example's shared-directory contamination | Phase 2 utility support |
| [4. API and tutorial documentation](phase-04-documentation.md) | Public API reference and step-by-step automatic-loading plugin tutorial | Phases 1–3 |

## Repository findings (planning baseline)

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
- Reflection documentation currently lives at `doc/tutorials/refection/` (existing spelling), not `doc/tutorials/reflection/`. Link to actual files without renaming unrelated documentation. The reflection example excludes its two tutorial components from serialization.
- Standalone generators copy `cmake/standalone_utils.cmake`, public headers and shared templates; the generated root CMake imports the precompiled engine as `bg2e`. Plugin support must be distributed through those paths too.
- The repository instructions mention `skills.md`, but that file was not found during inspection. Recheck before implementation if it becomes available.

## Execution constraints

- Documents and generated source content use English.
- The initial planning change created only planning documents. Implementation was subsequently authorized phase by phase, including CMake work for these examples/utilities and plugin SDK distribution; unrelated build-system changes remain outside scope.
- Do not add tests, test targets, compilation steps or runtime verification exercises to this implementation plan. Do not compile, configure CMake, run generators or launch examples during implementation. Completion review is source/document inspection only; separate future user requests can authorize other work.
- Keep changes out of `bg2e::gpu` and unrelated rendering APIs. Use the repository's current `draw` integration points discovered in source.
- Native plugin faults such as invalid memory access, `abort()` or crashing static initializers cannot be contained by an in-process exception boundary. Recoverable loader/API errors must not terminate the application; process isolation is outside this design.

## Step-by-step execution and resumption

- Treat each numbered phase in the table as one implementation step. Implement one phase per user instruction to continue, then stop for feedback; do not automatically proceed to the next phase. Internal numbered sections are tasks within that phase.
- Before resuming, read this summary, the relevant phase document and `plans/plugins/progress.md` next to this file. Inspect the current worktree to reconcile any in-progress work rather than repeating completed changes.
- Maintain `progress.md` throughout implementation and before every stop or interruption. State the last fully implemented step, the current step and unfinished tasks, relevant changed files, decisions/limitations and the exact next action. Distinguish implemented from accepted work; never report builds or tests that were not performed.
- After completing a phase, record that it awaits user feedback. If the user reports a problem, correct that phase and update progress before moving on. If the user says to continue implementation, treat the preceding implemented phase as correct/accepted and implement the next phase without asking for confirmation again.
- Initial state: planning revised, no implementation phase completed. Plan editing itself does not count as phase 1.
