# Plugin CMake integration

**Helpers:** `cmake/plugins.cmake`, included by repository `cmake/utils.cmake`
and generated-project `cmake/standalone_utils.cmake`.

CMake 3.23 or later is required. The detailed shared-helper guide is
[cmake/PLUGINS.md](../../../cmake/PLUGINS.md); this page connects it to the public
plugin SDK contract.

## Repository application and module

```cmake
set(APP_TARGET_NAME my_app)
if(APPLE)
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
else()
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME}
        OUTPUT_DIRECTORY "${PRODUCT_DIR}/examples/my_app")
endif()

bg2e_add_plugin(
    TARGET_NAME my_components
    APP_TARGET ${APP_TARGET_NAME}
    ENGINE_TARGET bg2e
    SOURCES plugin/ApplicationPlugin.cpp plugin/RotationComponent.cpp
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/plugin")
```

The app helper collects `src/*.cpp` from the calling directory. Keep plugin-only
sources outside `src`, and list them in the module's `SOURCES` instead. The engine
and app targets must already exist; the source-root build supplies the helpers.
`bundle_app` and `bundle_app_sdl` both accept `OUTPUT_DIRECTORY`, resolving it
before runtime/resource deployment is established. Omit it to preserve the normal
output location; macOS normally relies on each app's bundle boundary.

| Plugin argument | Requirement |
|-----------------|-------------|
| `TARGET_NAME` | Required unique module target. |
| `APP_TARGET` | Required existing application executable. |
| `SOURCES` | Required module implementation sources, relative to the calling directory or absolute. |
| `ENGINE_TARGET` | Optional existing SHARED engine target. Defaults to `BG2E_LIB_TARGET`, otherwise `bg2e`. Must be the same shared engine the app links. |
| `INCLUDE_DIRECTORIES` | Optional additional private module include roots. |

The helper supplies C++20, engine public headers, Vulkan/SDL/GLM dependency include
roots and the binary-paired generated SDK include root. Windows plugin targets
receive `BG2E_PLUGIN_BUILD` and the engine's actual MSVC runtime property. ABI-sensitive
options propagate from the shared target. No engine producer macro is defined and
no static duplicate engine is linked into the plugin.

## Output and refresh behavior

A module uses a target-specific intermediate directory under the build tree.
`<module>_deploy` depends on the module and copies its actual `TARGET_FILE`/filename;
the app depends on deployment. It refreshes on every app-target build, including
module-only edits that do not relink the executable. CMP0112 NEW prevents app-output
generator expressions from introducing a dependency cycle back to the app.

| Platform | Module destination | Engine resolution |
|----------|--------------------|-------------------|
| Linux | `<executable-dir>/plugins` | Module `$ORIGIN/..` and application `$ORIGIN` resolve the staged shared engine beside the executable. |
| Windows | `<executable-dir>/plugins` | Module links the engine import library; the app directory contains the engine DLL and staged non-system engine DLL dependencies. |
| macOS | `<app>.app/Contents/Resources/plugins` | Module loader paths resolve the same engine in `Contents/lib` using `@rpath`; no second engine goes under `plugins`. |

Configuration directories follow actual target outputs, keeping Windows Debug and
Release separate. Module suffixes come from CMake; do not require `.dylib` for a
macOS MODULE library. Runtime staging refreshes engine-only changes and deduplicates
shared application output writers. Assets/shaders retain the normal PlatformTools
resource locations. A caller using a raw executable instead of a bundle helper
must still configure its application headers, links and resources; additional
plugin-specific third-party dependencies remain its deployment responsibility.

## SDK profile and standalone projects

The source engine's `bg2e_configure_plugin_sdk(engine_target)` emits:

```text
PRODUCT_DIR/sdk/include/bg2e/plugin/BuildConfig.hpp
PRODUCT_DIR/sdk/bg2e-sdk.cmake
```

`BG2E_PLUGIN_ABI_OPTIONS` is an optional list of additional ABI-sensitive compiler
options, applied publicly to the engine and inherited by consumers. Its digest
enters the descriptor's build-options field. Keep these switches consistent rather
than independently changing public-layout flags on app/plugin targets. Compiler,
standard-library and runtime macro fingerprints are checked separately.

Both `scripts/create_standalone_project.*` generators copy the public SDK, generated
profile, shared helpers and contract documentation with the precompiled engine.
They produce `plugin/src/ApplicationPlugin.cpp`, a valid empty plugin, and this
existing app-target declaration in the generated `app/CMakeLists.txt`:

```cmake
bg2e_add_plugin(
    TARGET_NAME ${PROJECT_NAME}_plugin
    APP_TARGET ${APP_TARGET_NAME}
    ENGINE_TARGET bg2e
    SOURCES ../plugin/src/ApplicationPlugin.cpp)
```

To extend that starter, add private component sources to the list and use the
registrar in ApplicationPlugin::registerComponents. Zero components is valid.
The app starter's shutdown hook already cleans its scene; extend it for every
additional root and external ownership introduced by the application.

The imported shared engine has these SDK properties:

| Property | Meaning |
|----------|---------|
| `BG2E_SDK_INCLUDE_DIRECTORY` | Include root containing the generated `bg2e/plugin/BuildConfig.hpp`. |
| `BG2E_PLUGIN_ABI_OPTIONS` | Declared compatible additional options from the SDK manifest. |
| `INTERFACE_COMPILE_OPTIONS` | Propagates those options to consumers. |
| `MSVC_RUNTIME_LIBRARY` | Actual compatible runtime choice from the engine's manifest. |

`BG2E_INCLUDE_PATH` identifies the public engine headers. A custom imported engine
must supply the same properties/profile and link the same shared engine used by
the app. Missing generated profiles are diagnosed by the helper. Copy the actual
binary's headers/profile together; inventing an ABI string cannot establish
compatibility. File presence alone cannot prove an older binary matches current
source headers.

Both library updaters refresh the profile and helper alongside the binary/public
headers without overwriting user application/plugin sources. Distribution installs
also ship SDK/profile/helper files. See [packaging notes](../../../scripts/PACKAGING.md)
and [SDK compatibility](Sdk.md).
