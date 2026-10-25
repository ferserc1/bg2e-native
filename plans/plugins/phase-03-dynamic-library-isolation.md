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

## Completion review

Review the source and deployment definitions only. Do not add tests, compile, configure CMake, launch applications or delete historical binaries. Record completion in `progress.md` and stop for feedback.
