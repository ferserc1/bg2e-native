# Application and plugin CMake helpers

The reusable implementation is `plugins.cmake`, included by both `utils.cmake`
and `standalone_utils.cmake`. Generated standalone projects receive this same file,
its runtime deployment script and the SDK manifest belonging to their engine.
CMake 3.23 or later is required.

## Application output

Repository applications use `bundle_app` or `bundle_app_sdl`; generated external
projects use `standalone_bundle_app`. Each accepts `TARGET_NAME`, optional
`SHADERS_SRC` and optional `OUTPUT_DIRECTORY`:

```cmake
bundle_app_sdl(TARGET_NAME my_app OUTPUT_DIRECTORY "${PRODUCT_DIR}/examples/my_app")
```

Omitting the output argument preserves the helper's existing default. It is
resolved before resource-copy targets are created. Multi-configuration generators
retain separate configuration directories; Windows debugger working directories
follow the actual executable. macOS applications normally keep their existing
bundle output and omit this argument.

The application depends on an always-run runtime staging target, which also
refreshes engine-only edits. Applications sharing an output directory share one
writer. Linux loads the staged engine beside the executable through `$ORIGIN`;
Windows copies the engine DLL and resolves/stages its non-system DLL dependencies
using the selected SDK and linker tools. macOS stages the engine/SDL in
`Contents/lib`, with `@rpath` engine identity and executable-relative runtime search.
Shader/asset paths remain anchored to the executable or bundle Resources.

## Native plugin target

Create the application first, then declare its module:

```cmake
bg2e_add_plugin(
    TARGET_NAME my_components
    APP_TARGET my_app
    ENGINE_TARGET bg2e
    SOURCES plugin/ApplicationPlugin.cpp plugin/RotationComponent.cpp
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/plugin")
```

| Argument | Contract |
|----------|----------|
| `TARGET_NAME` | Required unique module target name. |
| `APP_TARGET` | Required existing executable target whose private plugin directory receives the module. |
| `SOURCES` | Required list of plugin-only sources, relative to the calling source directory or absolute. |
| `ENGINE_TARGET` | Optional existing shared engine target; defaults to `BG2E_LIB_TARGET`, otherwise `bg2e`. It must be the same shared engine linked by the application. Static engine copies are rejected. |
| `INCLUDE_DIRECTORIES` | Optional additional private include roots for plugin sources. |

The helper explicitly supplies engine public headers, Vulkan/SDL/GLM public
dependencies and the paired generated SDK include root; it does not rely on the
engine's private include directories. C++20 is required. `BG2E_PLUGIN_BUILD` makes
Windows import engine symbols while the plugin declaration macro exports its own
C entry points. The module inherits the shared target's ABI compile options and
uses its declared MSVC runtime property, including the repository's release CRT
choice in Debug builds.

Each plugin is a `MODULE` library with its intermediate binaries/import libraries
under a target-specific build directory. The app depends on `<module>_deploy`,
which depends on the module and runs every time the app is built, even when only
plugin code changes. Output paths use target-file generator expressions; CMP0112
is NEW in these helper definitions so deployment does not depend back on the app.

| Platform | Deployed module and engine resolution |
|----------|----------------------------------------|
| Linux | `<executable-dir>/plugins/<actual-module-name>`; module RPATH `$ORIGIN/..` finds the same engine beside the executable. |
| Windows | `<executable-dir>/plugins/<actual-module-name>`; engine import library is used at link time and the application directory contains the one runtime DLL. Configuration suffixes follow target outputs. |
| macOS | `<app>.app/Contents/Resources/plugins/<actual-module-name>`; `@loader_path/../../lib` and `@executable_path/../lib` resolve the engine in `Contents/lib`. |

No second engine is put inside `plugins`. The helper also stages the engine for
applications created without a bundle helper. Callers using a raw executable
must configure their own application headers, links and resources. Additional
third-party dependencies introduced by a plugin remain the application's deployment
responsibility; the engine deployment helper handles engine dependencies.

## Engine SDK identity

`bg2e_configure_plugin_sdk(engine_target)` is used when creating the source-built
shared engine. It emits `PRODUCT_DIR/sdk/include/bg2e/plugin/BuildConfig.hpp` and
`PRODUCT_DIR/sdk/bg2e-sdk.cmake`. The generated include root precedes source headers
for the engine and its consumers. `BG2E_PLUGIN_ABI_OPTIONS` is an optional CMake
list of additional ABI-sensitive compiler switches applied publicly to the engine
and therefore inherited by applications/plugins. Its digest enters the SDK ABI
fingerprint. Prefer this list over independently adding ABI-sensitive switches to
individual targets. Compiler, standard library and runtime/configuration macros
are checked separately by the descriptor; the digest is not a claim of universal
C++ binary compatibility.

Standalone generation copies this profile with public headers and the precompiled
binary. Its imported `bg2e` target declares `BG2E_SDK_INCLUDE_DIRECTORY`,
`BG2E_PLUGIN_ABI_OPTIONS`, `INTERFACE_COMPILE_OPTIONS` and `MSVC_RUNTIME_LIBRARY`
from that manifest. A custom imported engine must provide the same properties and
`BG2E_INCLUDE_PATH`; use its actual binary's SDK profile. Do not manually invent
version/ABI strings. Missing profiles fail with a diagnostic rather than silently
building against the source fallback. Replacing the engine also requires replacing
its headers and profile together; the library updaters do that.

## Standalone starter

Both `scripts/create_standalone_project.*` generators produce an application plus
`<project-name>_plugin`, an empty valid plugin in `plugin/src/ApplicationPlugin.cpp`.
The generated `app/CMakeLists.txt` already uses the helper. Add components and
reflection through its registrar and extend the app's shutdown cleanup when adding
external component ownership. See the generated `plugin/README.md` and the engine's
[programmer-owned unload contract](../doc/api/plugin/unloading.md).
