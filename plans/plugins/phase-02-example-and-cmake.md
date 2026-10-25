# Phase 2: Add the plugin tutorial, CMake utilities and standalone support

Depends on phase 1. This phase implements the requested build utilities and a new example; it does not replace the raw DynamicLibrary demonstration.

## 1. Add output isolation to application helpers

- Extend `bundle_app()` and `bundle_app_sdl()` with an optional `OUTPUT_DIRECTORY`; keep current default behavior for all existing callers.
- Resolve the destination before creating resource-copy commands and Windows deduplication keys. Use the parsed `TARGET_NAME` consistently rather than relying on a caller-local `APP_TARGET_NAME`.
- For the plugin examples, pass an isolated directory on Linux/Windows only, for example `${PRODUCT_DIR}/examples/tutorial_plugins`. Keep macOS's ordinary bundle output location.
- Preserve configuration separation: Windows Visual Studio outputs become `<isolated-root>/<configuration>/...`; do not mix Debug and Release files. Use target-file generator expressions for deployment rather than predicting the suffix.
- Audit `RUNTIME_OUTPUT_DIRECTORY`, configuration-specific overrides, debug working directory, resource copies and custom shader destinations together. Do not redirect global `PRODUCT_DIR` to isolate one example.
- Stage required Windows runtime DLLs beside the isolated executable using an explicit target dependency/deployment mechanism. The current `bundle_lib()` is macOS-only; moving the executable alone is insufficient.
- On Linux, define build RPATH and engine runtime dependency resolution from the isolated output. Keep shader and asset copies beside the executable as expected by PlatformTools.
- On macOS, retain existing `Contents/lib`, resource copies and framework handling. No additional executable-directory isolation is necessary.

## 2. Add a reusable engine plugin helper

Add `bg2e_add_plugin()` to `cmake/utils.cmake` with an explicit interface, for example:

```cmake
bg2e_add_plugin(
    TARGET_NAME component_example_plugin
    APP_TARGET tutorial_plugins_example_app
    SOURCES plugin/ExamplePlugin.cpp plugin/ExampleComponent.cpp
)
```

- Build a loadable library target, preferably `MODULE` because consumers resolve exports dynamically; use `TARGET_FILE`/`TARGET_FILE_NAME` so platform prefixes/suffixes are never hard-coded.
- Configure engine public headers explicitly using `BG2E_INCLUDE_PATH`, plus the public-header dependency paths needed by the selected API. The engine target currently declares its include paths `PRIVATE`, so linking it alone does not supply headers.
- Link to the shared `${BG2E_LIB_TARGET}`, use C++20 and the SDK-compatible ABI/runtime options, and define the plugin's own export symbol macro. Never define the engine's producer/export macro on the plugin.
- Keep intermediate plugin binaries/import libraries in a target-specific build location. Deploy only the loadable module to the application's private plugin directory.
- Deploy to `$<TARGET_FILE_DIR:app>/plugins` on Linux/Windows and the app bundle's `Contents/Resources/plugins` on macOS.
- Use a copy/deployment target that refreshes whenever the application is built, including plugin-only edits that do not relink the executable. Ensure the deployment depends on the module, and the application depends on deployment, without forming a target dependency cycle.
- Review generator-expression dependency behavior and CMake policies when referencing application output directories from the deployment target.
- On macOS, ensure the module resolves the same engine in `Contents/lib` using appropriate loader paths/install names; do not bundle a second engine inside `plugins`. Review the equivalent single-engine dependency definitions on Linux/Windows.
- Make this a clear developer-facing utility for future user applications, not an example-specific helper. Document required/optional arguments, target dependencies, SDK headers and ABI settings, configuration handling, engine target selection and deployment paths. Support both the repository engine target and the imported shared engine used by standalone projects through the same interface or an explicitly documented thin adapter.
- Define the actual platform resolution rules: Linux module/executable RPATH must resolve the application's engine shared object; Windows links the engine import library and stages the one engine DLL beside the executable; macOS install names and loader paths resolve the engine in the application's `Contents/lib`. Do not deploy another engine into `plugins`.
- Include standalone generators and their copied utilities/templates in this phase, as detailed below.

## 3. Generate standalone applications with an empty application plugin

- Update both `scripts/create_standalone_project.sh` and `scripts/create_standalone_project.ps1`, their `scripts/templates/` inputs and `cmake/standalone_utils.cmake`. Keep behavior and generated layout consistent across platforms.
- Generate an application plugin source/template and a real plugin target included in the default application build. It implements the Plugin class, identity/license metadata and the declaration macro, with an empty `registerComponents` callback. It registers zero components successfully and is ready for the author to add components later.
- Use the reusable plugin utility against the imported precompiled shared engine. Ship all required public headers, SDK/ABI identity headers, helper modules and templates; no dependency on the source checkout may remain in the generated project.
- Deploy the empty module to the application's `plugins` resource directory so MainLoop discovers it automatically. Preserve per-configuration separation and platform engine resolution described above.
- Review `scripts/update_libraries.sh`, `scripts/update_libraries.ps1` and packaging scripts for SDK header/ABI identity consistency. Refresh the identity paired with the copied engine when updating libraries; make only changes required by plugin SDK distribution, with no unrelated packaging refactor.
- Explain the generated plugin target, where components can be added and the SDK/toolchain compatibility requirements in the standalone project instructions and `scripts/PACKAGING.md` as appropriate.

## 4. Add the reflection-based plugin tutorial application

- Create `examples/tutorials/plugins/` with application sources, plugin sources, CMake integration and a README. Add it to the actual tutorial example aggregation discovered in the repository.
- Base the application on `examples/tutorials/reflection`, reproducing the same scene and retaining camera controls, picking, multiple selection, gizmos, materials, lighting, graphics settings and the other application controls.
- Define `RotationComponent` and `ReflectionSamplesComponent` inside the plugin, including their supporting reflected value types, polymorphic subtypes, factories and actions. Preserve their behavior: Rotation is initially attached to the original model; Reflection Samples is available through Add Component.
- Register all plugin contributions explicitly through the owner-aware registrar during `registerComponents`, without global/static registration side effects or the tutorial's one-time static registration helpers. Track supporting reflection types and subtype factories as well as the two components.
- The host must not compile/link these implementations, call their registration functions or directly construct their concrete classes. Create the initial Rotation component through the shared component registry using its stable ID. Plugin implementation headers remain private to the module.
- Remove the application-owned manual reflection inspector window and `TutorialNodeEditor` implementation, its state, callbacks and editing queue where used only by that window. Do not remove unrelated graphics settings or their existing wrappers.
- Retain the engine inspector window and its existing NodeEditor integration, including `lib/include/bg2e/ui/ComponentInspector.hpp` behavior. Both plugin components must participate in its normal Add Component/property/action workflows; do not implement another manual component list.
- Preserve the original tutorial's component serialization policy (currently excluded from serialization); do not add a scene fixture or round-trip exercise to this tutorial. General plugin serialization support remains a system requirement in phase 1.
- Let MainLoop load the module automatically from `pluginPath()` before scene creation. Use the plugin CMake helper to deploy it to `plugins`; do not add an explicit load call as a fallback.
- Include plugin identity, description, metadata, version and explicit license declaration. Release application scene, selection and UI references at the lifecycle boundary required by phase 1.
- This example supersedes the earlier proposed component-plugin/message demonstration. It is not based on the raw dynamic-library application's button/message UI; that existing example remains separate.

## Expected deployment

```text
Linux:   bin/linux/examples/tutorial_plugins/<app>
         bin/linux/examples/tutorial_plugins/plugins/<module>.so
Windows: bin/windows/examples/tutorial_plugins/<config>/<app>.exe
         bin/windows/examples/tutorial_plugins/<config>/plugins/<module>.dll
macOS:   <app>.app/Contents/MacOS/<app>
         <app>.app/Contents/Resources/plugins/<module>.dylib-or-module-suffix
```

These are illustrative target layouts; shader/asset directories and required runtime libraries must also be staged or resolved correctly. Preserve the actual CMake module suffix rather than requiring `.dylib` on macOS.

## Completion review

Review CMake, scripts and source text against the requirements without adding tests, compiling, configuring CMake, running generators or launching applications. Record completion in `progress.md` and stop for feedback.
