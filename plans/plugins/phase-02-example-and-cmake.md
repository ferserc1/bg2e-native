# Phase 2: Add the component plugin example and CMake utilities

Depends on phase 1. This phase implements the requested build utilities and a new example; it does not replace the raw DynamicLibrary demonstration.

## 1. Add output isolation to application helpers

- Extend `bundle_app()` and `bundle_app_sdl()` with an optional `OUTPUT_DIRECTORY`; keep current default behavior for all existing callers.
- Resolve the destination before creating resource-copy commands and Windows deduplication keys. Use the parsed `TARGET_NAME` consistently rather than relying on a caller-local `APP_TARGET_NAME`.
- For the plugin examples, pass an isolated directory on Linux/Windows only, for example `${PRODUCT_DIR}/examples/component_plugin`. Keep macOS's ordinary bundle output location.
- Preserve configuration separation: Windows Visual Studio outputs become `<isolated-root>/<configuration>/...`; do not mix Debug and Release files. Use target-file generator expressions for deployment rather than predicting the suffix.
- Audit `RUNTIME_OUTPUT_DIRECTORY`, configuration-specific overrides, debug working directory, resource copies and custom shader destinations together. Do not redirect global `PRODUCT_DIR` to isolate one example.
- Stage required Windows runtime DLLs beside the isolated executable using an explicit target dependency/deployment mechanism. The current `bundle_lib()` is macOS-only; moving the executable alone is insufficient.
- On Linux, configure/verify build RPATH and engine runtime dependency resolution from the isolated output. Keep shader and asset copies beside the executable as expected by PlatformTools.
- On macOS, retain existing `Contents/lib`, resource copies and framework handling. No additional executable-directory isolation is necessary.

## 2. Add a reusable engine plugin helper

Add `bg2e_add_plugin()` to `cmake/utils.cmake` with an explicit interface, for example:

```cmake
bg2e_add_plugin(
    TARGET_NAME component_example_plugin
    APP_TARGET component_plugin_example_app
    SOURCES plugin/ExamplePlugin.cpp plugin/ExampleComponent.cpp
)
```

- Build a loadable library target, preferably `MODULE` because consumers resolve exports dynamically; use `TARGET_FILE`/`TARGET_FILE_NAME` so platform prefixes/suffixes are never hard-coded.
- Configure engine public headers explicitly using `BG2E_INCLUDE_PATH`, plus the public-header dependency paths needed by the selected API. The engine target currently declares its include paths `PRIVATE`, so linking it alone does not supply headers.
- Link to the shared `${BG2E_LIB_TARGET}`, use C++20 and the SDK-compatible ABI/runtime options, and define the plugin's own export symbol macro. Never define the engine's producer/export macro on the plugin.
- Keep intermediate plugin binaries/import libraries in a target-specific build location. Deploy only the loadable module to the application's private plugin directory.
- Deploy to `$<TARGET_FILE_DIR:app>/plugins` on Linux/Windows and the app bundle's `Contents/Resources/plugins` on macOS.
- Use a copy/deployment target that refreshes whenever the application is built, including plugin-only edits that do not relink the executable. Ensure the deployment depends on the module, and the application depends on deployment, without forming a target dependency cycle.
- Verify generator-expression dependency behavior and CMake policies when referencing application output directories from the deployment target.
- On macOS, ensure the module resolves the same engine in `Contents/lib` using appropriate loader paths/install names; do not bundle a second engine inside `plugins`. Verify the equivalent single-engine behavior on Linux/Windows.
- Document arguments, output/configuration behavior and how non-example applications use the helper. Updating standalone-project helpers is a separate follow-up unless required by the chosen API; do not imply this helper is already distributed there.

## 3. Add the example

- Create `examples/plugins/component_plugin/` with an application `src/` directory, plugin sources and a README. Add it explicitly to `examples/CMakeLists.txt`.
- Follow `examples/plugins/dynamic_library`'s application-plus-library organization, but use the new Plugin contract and helper rather than a raw `getMessage` symbol.
- Implement at least two simple serializable components with unique persistent IDs and reflection metadata. Implement their required clone/deserialization contracts and keep registration graphics-free.
- Use one registration call per component. Include example plugin identity, description, metadata, version and explicit license declaration.
- Let MainLoop discover the module automatically through `pluginPath()`; the default path must not call explicit load to conceal a deployment error.
- In scene initialization, instantiate a registered plugin component through the registry. Provide a small serialized scene fixture or round-trip path proving plugin-defined types deserialize through normal scene loading.
- Demonstrate observable component behavior and report load/registration results through logging. If an inspector is useful, use the existing wrapped `ui::ComponentInspector`; do not manually enumerate node components into a new UI.
- Keep an empty/minimal UI delegate if required by MainLoop. No raw imgui headers or functions in public interfaces or application code.
- Ensure application-owned scenes/components are released at the correct shutdown boundary. Include optional documented runtime unload exercises without making destructive removal automatic.

## Expected deployment

```text
Linux:   bin/linux/examples/component_plugin/<app>
         bin/linux/examples/component_plugin/plugins/<module>.so
Windows: bin/windows/examples/component_plugin/<config>/<app>.exe
         bin/windows/examples/component_plugin/<config>/plugins/<module>.dll
macOS:   <app>.app/Contents/MacOS/<app>
         <app>.app/Contents/Resources/plugins/<module>.dylib-or-module-suffix
```

These are illustrative target layouts; shader/asset directories and required runtime libraries must also be staged or resolved correctly. Preserve the actual CMake module suffix rather than requiring `.dylib` on macOS.

## Acceptance checks

- Review CMake target includes: compiling the plugin must not rely on accidental directory-level engine includes.
- Confirm output isolation is established before resource deployment targets are defined; existing example locations remain unchanged.
- After explicit build authorization, build with Ninja on Linux, Visual Studio 17 2022 on Windows and Xcode on macOS. Check configuration separation on Windows.
- Launch from an unrelated working directory without custom DLL/library-path environment variables; verify engine dependencies, resources and automatic plugin discovery.
- Verify both component types are registered and the scene fixture loads. Use existing inspector integration if shown, without a manual component-list UI.
- Change only plugin code, rebuild the app target and confirm deployment refreshes. Repeat for required runtime library/resource changes.
- Run a neighboring ordinary example and confirm this plugin is not discovered there. No module is copied into the shared Linux/Windows `plugins` directory.
