# SDK and binary compatibility

**Header:** `<bg2e/plugin/Sdk.hpp>`

**Generated configuration:** `<bg2e/plugin/BuildConfig.hpp>`

**Namespace:** `bg2e::plugin`

## Versions have different meanings

| Identity | Current definition | Purpose |
|----------|--------------------|---------|
| C bootstrap contract | `BG2E_PLUGIN_CONTRACT_VERSION` = `1u` | Interpretation of the descriptor before invoking C++ objects. |
| SDK version | `BG2E_SDK_VERSION` = `"0.1.0"` | SDK/header release identity. |
| SDK C++ ABI | `BG2E_SDK_ABI_ID` = `"bg2e-plugin-sdk-2"` | Compatible engine C++ interfaces/layouts; must change when that contract changes. |
| SDK build options | `BG2E_SDK_BUILD_ABI` | Generated digest of additional ABI-sensitive CMake options. |
| Plugin version | `PluginInfo::version` | Author's plugin release; unrelated to SDK/bootstrap compatibility. |

These are source/header definitions, not values an author should copy into each
plugin. `BG2E_DEFINE_PLUGIN` obtains them from that module's SDK headers and
compiler environment. [CMake integration](CMake.md) puts the binary-paired generated
BuildConfig header first. The source fallback is `"default"` for custom non-CMake
builds; supported CMake helpers require the generated profile.

## PluginApi bootstrap descriptor

The native-layout descriptor uses fixed-width integer fields, immutable borrowed
C-string literals, native pointers and a destruction function pointer. It has no
STL members and is declared with packing 8. Architecture/pointer-size matching is
still required. Version 1 requires exact structure size and contract version;
it does not implement a size-prefix extension or version-range negotiation.

```cpp
struct PluginApi {
    uint32_t structureSize;
    uint32_t contractVersion;
    const char* sdkVersion;
    const char* sdkAbi;
    const char* architecture;
    const char* compiler;
    const char* standardLibrary;
    const char* runtime;
    const char* buildOptions;
    uint32_t pointerSize;
    uint32_t packing;
    uint32_t flags;
    uint32_t iteratorDebugLevel;
    uint64_t cppVersion;
    uint32_t stringSize;
    uint32_t sharedPointerSize;
    uint32_t compilerAbi;
    uint32_t hardeningMode;
    const void* sharedEngine;
    void (BG2E_PLUGIN_CALL *destroy)(void*);
};

using QueryPluginApi = const PluginApi* (BG2E_PLUGIN_CALL *)();
using CreatePlugin = void* (BG2E_PLUGIN_CALL *)();

const PluginApi& engineSdk();
const void* sharedEngineIdentity() noexcept;
```

`engineSdk()` exposes the engine's immutable fingerprint for diagnostics, not a
module declaration template to return instead of the module's own fingerprint.
`sharedEngineIdentity()` supplies an identity address from the resolved shared
engine. A module linked to another resolved engine instance fails that comparison.
The descriptor's destruction function belongs to the plugin; the engine descriptor
is not a plugin factory/destruction contract.

The generated query lazily initializes descriptor data only; it does not allocate
a Plugin instance or register component types. Applications must use the loader
rather than calling bootstrap functions directly.

## Exact comparisons before C++ use

The loader obtains the C factory symbol but does not invoke it until descriptor
validation succeeds. It checks the query, nonnull descriptor, exact size/version,
then compares the following against the running engine:

| Fields | Compatibility data |
|--------|--------------------|
| `sdkVersion`, `sdkAbi` | Exact SDK and C++ ABI identity strings. |
| `architecture`, `pointerSize` | Architecture label (`arm64`, `x86_64`, `x86`) and native pointer size; unsupported engine fingerprints are rejected. |
| `compiler`, `compilerAbi` | Exact Clang/GCC version string or MSVC full version; GNU-compatible ABI version when defined. |
| `standardLibrary` | libc++ version/ABI, libstdc++ date/C++11 ABI setting, or MSVC STL version/update. |
| `runtime`, `iteratorDebugLevel` | Platform/CRT macros and MSVC iterator ABI. Windows uses actual `_DLL`/`_DEBUG` compiler definitions, not just a build directory's name. |
| `packing` | Offset of a 64-bit member in a packing probe under the module's ambient settings. |
| `flags` | RTTI, exceptions, libstdc++ debug/assertions and unstable libc++ ABI flags. |
| `cppVersion` | Exact language-version macro (`_MSVC_LANG` on MSVC). |
| `stringSize`, `sharedPointerSize` | Selected public C++ object-size checks. |
| `hardeningMode` | libc++ hardening mode when defined. |
| `buildOptions` | Additional declared ABI-sensitive compiler-option digest. |
| `sharedEngine`, `destroy` | Same resolved engine identity and a nonnull plugin destruction entry point. |

Mismatch reports include field labels and host/plugin values in `LoadReport::errors`.
Missing descriptor/unsupported structure returns `Incompatible` before later
fields are interpreted. After descriptor validation, `PluginInfo::requiredSdkAbi`
can add another exact restriction but cannot loosen the fingerprint checks.
A null factory result or factory/metadata/registration callback failure is an
initialization failure rather than a successful compatible load.

This is intentionally conservative. Semantic version ranges alone do not establish
C++ ABI compatibility. The fingerprint is not a complete proof for arbitrary
compiler flags or custom allocators; use a compatible SDK/toolchain and declare
additional ABI-sensitive switches consistently through `BG2E_PLUGIN_ABI_OPTIONS`.
The CMake helpers propagate the shared engine's MSVC runtime property, including
this repository's `MultiThreadedDLL` choice instead of inferring `/MDd` from Debug.

## Native-loader limits

C linkage makes bootstrap symbol names callable; it does not make Plugin,
Component, STL containers or callbacks compiler-independent. Plugins link the
application's shared engine, never an embedded static duplicate.

Opening a native module already resolves dependencies and may execute arbitrary
static initializers **before** the host can query the descriptor. Wrong architecture
or missing dependencies can therefore surface as `LibraryLoadFailure` without an
available descriptor. Static initialization must not register types or initialize
graphics. An invalid descriptor pointer, access violation, `abort`, terminating
destructor or crashing initializer cannot be contained by an in-process C++ catch.
The system is not process isolation or a sandbox for untrusted native code.

See [Plugin](Plugin.md) for generated exports and [CMake](CMake.md) for shipping
matching headers/profile/binary together.
