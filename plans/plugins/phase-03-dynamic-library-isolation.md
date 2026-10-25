# Phase 3: Isolate the existing raw dynamic-library example

Depends on the application-output support from phase 2.

## Problem

`examples/plugins/dynamic_library/CMakeLists.txt` copies `dylib_example_lib` into `$<TARGET_FILE_DIR:dylib_example_app>/plugins` on Linux/Windows. The application helper currently places that executable alongside the ordinary examples, so this raw `getMessage` library contaminates their automatic discovery directory.

The macOS destination is already inside the individual application's `Contents/Resources/plugins` directory and needs no additional executable isolation.

## Changes

1. Keep this example focused on `base::DynamicLibrary` and `getMessage`; do not convert its library into an engine Plugin or use the engine-plugin helper for a library without that contract.
2. Pass an isolated `OUTPUT_DIRECTORY`, such as `${PRODUCT_DIR}/examples/dynamic_library`, to `bundle_app_sdl()` on Linux/Windows only. Preserve macOS's existing bundle output behavior.
3. Keep the raw library's intermediate outputs in its own build directory and retain configuration-aware target-file expressions for deployment.
4. Keep the existing application-private `plugins` copy destination, now anchored to the isolated executable. Preserve refresh-on-every-app-build behavior for plugin-only changes.
5. Replace manual platform resource-root construction in `src/main.cpp` with `PlatformTools::pluginPath()` from phase 1.
6. Use the phase 2 runtime dependency/resource arrangement so the moved application starts normally on Windows/Linux without relying on its previous neighboring DLLs or working directory.
7. Document that MainLoop may classify this example's raw library as `NotAPlugin` during automatic scanning; that is recoverable and must not prevent the button from explicitly loading `getMessage`. Avoid treating it as a successfully registered engine plugin.
8. Audit the old shared `plugins` location for stale copies from earlier builds. Document a precise manual migration step for the exact historical example binary. Do not recursively delete the shared directory or unrelated plugins, and do not add a broad configure-time cleanup command.

## Acceptance checks

- Static review confirms both plugin examples have distinct Linux/Windows runtime roots and neither deploys a module to the ordinary examples' shared `plugins` directory.
- After explicit compilation authorization, build and run `dylib_example_app` on each platform; verify its existing Load library button still resolves `getMessage` and displays the returned message.
- Automatic scanning of its private raw library returns `NotAPlugin` without throwing or closing the app.
- Rebuild after a raw library-only edit and verify the deployed library is refreshed.
- Verify shaders, assets and required engine/SDL runtime dependencies remain available after isolation, including Windows configurations and launch from another working directory.
- Verify macOS still uses `Contents/Resources/plugins` inside this application's bundle.
- Launch an ordinary example and the component-plugin example; neither discovers the raw library from this example once any explicitly identified historical stale copy has been removed.
