# FSR 3.1.4 Linux Integration — Work in Progress

## Status

| Platform | CMake | Build | Shaders | Link |
|---|---|---|---|---|
| Windows | ✅ OK | ✅ OK | ✅ Generated | ✅ OK |
| Linux   | ✅ OK | ❓ Untested | ❓ Untested | ❓ Untested |

**Next session starts here: build and debug the Linux compilation.**

---

## Architecture overview

The integration is a static link chain:

```
bg2e (SHARED)
  └── ffx_fsr3_x64 (STATIC)
        └── ffx_fsr3upscaler_x64 (STATIC)
        └── ffx_frameinterpolation_x64 (STATIC)
        └── ffx_opticalflow_x64 (STATIC)
        └── ffx_backend_vk_x64 (STATIC)
              └── *_permutations.h  ← 112 generated headers with embedded SPIR-V blobs
```

Shaders are compiled at **build time** (not CMake configure time) by `ffx_sc_linux`, a Linux port
of the Windows-only `FidelityFX_SC.exe`. The generated `.h` files live in:
```
<build>/lib/ffx_sdk/src/backends/shaders/vk/
```

---

## File map

### New files created for this integration

```
lib/third_party/FidelityFX_SC_Linux/        ← Linux shader compiler port
├── CMakeLists.txt                           ← Builds ffx_sc_linux executable
└── src/
    ├── pch.hpp                              ← Portable pch (no Windows.h)
    ├── compiler.h                           ← Copy of SDK compiler.h; defines changed from
    │                                           std::vector<std::wstring> to std::vector<std::string>
    ├── glsl_compiler.h                      ← Copy of SDK glsl_compiler.h (unchanged)
    ├── glsl_compiler.cpp                    ← Copy of SDK glsl_compiler.cpp; two changes:
    │                                           1. Removed #include "utils.h"
    │                                           2. Default glslang exe: "glslangValidator" (no .exe)
    └── ffx_sc.cpp                           ← Full Linux port of ffx_sc.cpp:
                                                - wmain → main
                                                - std::wstring → std::string throughout
                                                - Windows path APIs → std::filesystem
                                                - _wfopen_s → std::fopen
                                                - HLSL/DXC/FXC branches removed
                                                - WCharToUTF8 calls eliminated
```

### Modified files

| File | Change |
|---|---|
| `CMakeLists.txt` (root) | `cmake_minimum_required` bumped from 3.18 to 3.23 (SDK requires it) |
| `lib/CMakeLists.txt` | Bumped to 3.23; added FFX include + link in `if(NOT APPLE)` block |
| `lib/cmake/deps.cmake` | Added full FFX integration block at the bottom (see below) |
| `cmake/FindVulkan.cmake` | Added `Vulkan_VERSION` parsing from `vulkan_core.h`; added `Vulkan::Vulkan` imported target |
| `lib/third_party/FidelityFX-SDK-1.1.4/sdk/src/backends/vk/FrameInterpolationSwapchain/FrameInterpolationSwapchainVK.cpp` | 3 bug fixes for MSVC strictness (see "SDK bug fixes" section) |

---

## deps.cmake integration block

Added at the bottom of `lib/cmake/deps.cmake`:

```cmake
if(NOT APPLE)
    if(UNIX AND NOT APPLE)
        add_subdirectory("${THIRD_PARTY_PATH}/FidelityFX_SC_Linux" ffx_sc_linux_build)
        set(FFX_SC_EXECUTABLE "$<TARGET_FILE:ffx_sc_linux>" CACHE STRING "" FORCE)
    endif()

    set(FFX_API_BACKEND   "VK_X64")
    set(FFX_API_VK        ON)   # CRITICAL: not set by SDK, checked first line of vk/CMakeLists.txt
    set(FFX_FSR3          ON)   # also enables FSR3Upscaler, FrameInterpolation, OpticalFlow
    set(FFX_BUILD_AS_DLL  OFF)
    set(FFX_AUTO_COMPILE_SHADERS ON CACHE BOOL "" FORCE)

    add_subdirectory("${THIRD_PARTY_PATH}/FidelityFX-SDK-1.1.4/sdk" ffx_sdk)

    if(UNIX AND NOT APPLE)
        add_dependencies(ffx_shader_permutations_vk ffx_sc_linux)
    endif()

    set(BG2E_FFX_INCLUDE_PATH "${THIRD_PARTY_PATH}/FidelityFX-SDK-1.1.4/sdk/include")
    set(BG2E_FFX_LINK_LIBRARIES ffx_fsr3_x64 ffx_backend_vk_x64)
endif()
```

Key undocumented requirement: **`FFX_API_VK ON` must be set manually.** The very first line
of `sdk/src/backends/vk/CMakeLists.txt` is `if(NOT ${FFX_API_VK}) return()`. The SDK never
sets this variable itself.

---

## Linux shader compiler (ffx_sc_linux)

### What it does
Compiles 28 GLSL shaders into 112 `*_permutations.h` C headers containing embedded SPIR-V
binary blobs. Called as a pre-build step via CMake `add_custom_command`.

### Build dependencies (all referenced from the SDK by path, not copied)
- `sdk/tools/ffx_shader_compiler/libs/tiny-process-library/` — `process.cpp` + `process_unix.cpp`
- `sdk/tools/ffx_shader_compiler/libs/SPIRV-Reflect/` — `spirv_reflect.c`
- `sdk/tools/ffx_shader_compiler/libs/MD5/` — `md5.cpp`

### glslangValidator detection
In `FidelityFX_SC_Linux/CMakeLists.txt`:
```cmake
find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator glslang
    HINTS "${VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/bin"
)
```
The found path is baked in as `FFX_LINUX_GLSLANG_DEFAULT` compile definition. If it cannot be
found at CMake time, the build will fail with:
```
glslangValidator not found. Ensure VULKAN_SDK is set.
```
Fix: ensure `VULKAN_SDK` env variable is set, or `-DVULKAN_SDK=/path`.

### Differences from Windows FidelityFX_SC.exe
- Only supports `glslang` compiler (HLSL/DXC/FXC branches removed)
- `std::string` everywhere (Linux is natively UTF-8)
- `std::filesystem::create_directories` + `std::filesystem::weakly_canonical` instead of
  `PathAllocCanonicalize` / `CreateDirectoryW`
- `std::fopen` instead of `_wfopen_s`
- `main()` instead of `wmain()`

---

## FindVulkan.cmake changes

The project's custom `cmake/FindVulkan.cmake` was missing two things required by the FFX SDK:

1. **`Vulkan_VERSION`** — SDK's `CheckVulkanSDKVersion()` calls
   `if(${Vulkan_VERSION} VERSION_LESS "1.3.250")`. With an empty variable this becomes
   `if(VERSION_LESS "1.3.250")` which is a CMake parse error. Fixed by parsing
   `vulkan_core.h` for `VK_HEADER_VERSION_COMPLETE` and `VK_HEADER_VERSION`.

2. **`Vulkan::Vulkan` imported target** — `ffx_backend_vk_x64` links against it via
   `target_link_libraries(... Vulkan::Vulkan)`. Fixed by adding:
   ```cmake
   add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
   set_target_properties(Vulkan::Vulkan PROPERTIES
       IMPORTED_LOCATION             "${VULKAN_LIB}"
       INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_PATH}"
   )
   ```

---

## SDK bug fixes (applied to vendor code)

File: `sdk/src/backends/vk/FrameInterpolationSwapchain/FrameInterpolationSwapchainVK.cpp`

These are real bugs in SDK 1.1.4 — rejected by MSVC in strictness mode, also potentially
problematic on GCC/Clang.

| Line | Bug | Fix |
|---|---|---|
| 338 | `waitCallback(L"FenceName", value)` — string literal passed as `wchar_t*` | `const_cast<wchar_t*>(L"FenceName")` |
| 1187 | `submit(VK_NULL_HANDLE, toWait, SubmissionSemaphores())` — rvalue bound to `SubmissionSemaphores&` | Introduce named variable `SubmissionSemaphores noSignal{}` |
| 2826 | Same as 1187 | Same fix |

---

## CMake variables reference

| Variable | Value | Purpose |
|---|---|---|
| `FFX_API_BACKEND` | `"VK_X64"` | Selects Vulkan backend; triggers `toolchain.cmake` which sets `CMAKE_GENERATOR_PLATFORM "x64"` |
| `FFX_API_VK` | `ON` | **Undocumented, critical.** First line of VK CMakeLists guards on this |
| `FFX_FSR3` | `ON` | Enables FSR3Upscaler + FrameInterpolation + OpticalFlow automatically |
| `FFX_BUILD_AS_DLL` | `OFF` | Static libraries |
| `FFX_AUTO_COMPILE_SHADERS` | `ON` | Adds pre-build shader compilation step |
| `FFX_SC_EXECUTABLE` | `$<TARGET_FILE:ffx_sc_linux>` | Path to the shader compiler (Linux only) |
| `FFX_PLATFORM_NAME` | `x64` | Set internally by SDK after `toolchain.cmake` runs; target suffix |

Static lib targets created by the SDK:
- `ffx_fsr3_x64`
- `ffx_fsr3upscaler_x64`
- `ffx_frameinterpolation_x64`
- `ffx_opticalflow_x64`
- `ffx_backend_vk_x64`

`bg2e` links only `ffx_fsr3_x64` and `ffx_backend_vk_x64`; the others are pulled in transitively.

---

## Expected build output (Linux)

After a successful build:

```
bin/linux/
├── libbg2e.so                    ← bg2e with FFX linked in
└── ...

<build>/lib/
├── ffx_sdk/
│   ├── src/backends/shaders/vk/  ← 112 *_permutations.h generated here at build time
│   └── ...
└── ffx_sc_linux_build/
    └── ffx_sc_linux              ← The shader compiler binary (built first)
```

The SPIR-V blobs end up inside `ffx_backend_vk_x64.a` → linked into `libbg2e.so`. There are
no loose `.spv` files at runtime; everything is in the shared library.

---

## Potential Linux-specific issues to watch for

### 1. `ffx_sc_linux` fails to find glslangValidator at runtime
**Symptom:** Pre-build shader step fails with "command not found" or similar.  
**Check:** `which glslangValidator` on the Linux machine. If missing, install from the Vulkan SDK:
the validator is at `$VULKAN_SDK/bin/glslangValidator`.  
**Verify CMake baked the right path:** in the build dir, run:
```sh
grep -r "FFX_LINUX_GLSLANG_DEFAULT" CMakeCache.txt
```

### 2. `ffx_sc_linux` builds but crashes during shader compilation
**Symptom:** Build step exits non-zero; `*_permutations.h` not generated.  
**Debug:** Run the compiler manually:
```sh
<build>/lib/ffx_sc_linux_build/ffx_sc_linux -compiler=glslang \
  -output=<build>/lib/ffx_sdk/src/backends/shaders/vk/ \
  <sdk>/sdk/src/backends/shaders/vk/ffx_fsr3upscaler_accumulate_pass.glsl \
  -DFFX_GLSL=1 --target-env vulkan1.2 -S comp -Os -e CS
```

### 3. CMake cannot find `ffx_shader_permutations_vk` target for `add_dependencies`
**Symptom:** CMake error about unknown target.  
**Cause:** The target only exists when `FFX_AUTO_COMPILE_SHADERS=ON` and `FFX_API_VK=ON`.  
**Check:** Ensure both are set before `add_subdirectory(...sdk...)`.

### 4. Linker errors: undefined symbols from FFX libs
**Symptom:** `libbg2e.so` links but missing `ffx_*` symbols.  
**Cause:** Static libs need to come before the objects that reference them on Linux linker.  
**Fix in `lib/CMakeLists.txt`:** wrap FFX link with `-Wl,--whole-archive` if needed:
```cmake
target_link_libraries(bg2e PRIVATE
    -Wl,--whole-archive ${BG2E_FFX_LINK_LIBRARIES} -Wl,--no-whole-archive
)
```
Only do this if you get undefined symbol errors at link time.

### 5. SDK's `toolchain.cmake` sets `CMAKE_SYSTEM_NAME WINDOWS` inside subdirectory
**Impact:** This is harmless — setting `CMAKE_SYSTEM_NAME` after `project()` is called is a
no-op. The build system remains Linux.

### 6. Shader compilation produces empty or corrupt headers
**Symptom:** `*_permutations.h` exist but are empty or `bg2e` crashes at runtime.  
**Check:** Look at the first few bytes of a generated header — it should start with
`// ffx_fsr3upscaler_*.h` and contain `static const unsigned char g_ffx_...`.

### 7. `process_unix.cpp` compilation errors
**Symptom:** Compilation error in tiny-process-library's Unix implementation.  
**Cause:** Missing POSIX headers.  
**Fix (in `FidelityFX_SC_Linux/CMakeLists.txt`):** add `target_compile_definitions(ffx_sc_linux PRIVATE _GNU_SOURCE)` if needed.

---

## Linux build commands

```sh
# From project root
cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/VulkanSDK/x.y.z/x86_64
cmake --build build

# Or just build the shader compiler first to test it in isolation
cmake --build build --target ffx_sc_linux

# Then trigger shader compilation
cmake --build build --target ffx_shader_permutations_vk
```

---

## SDK directory cleanup (pending decision)

~155 MB can be removed from the SDK before committing to the repo. Safe to delete:

```
lib/third_party/FidelityFX-SDK-1.1.4/
├── ffx-api/                                         # Legacy API layer, not used
├── sdk/src/backends/dx12/                           # DX12 backend
├── sdk/include/FidelityFX/host/backends/dx12/       # DX12 headers
├── sdk/tools/ffx_shader_compiler/libs/agilitysdk/   # DX12 Agility SDK (~64 MB)
├── sdk/tools/ffx_shader_compiler/libs/dxc/          # HLSL compiler (~44 MB)
├── sdk/libs/pix/                                    # DX12 GPU profiler
├── sdk/libs/antilag2/                               # DX12-only feature
├── sdk/tools/media_delivery/MediaDelivery.exe
├── sdk/tools/binary_store/d3dconfig.exe
├── sdk/tools/binary_store/d3d12SDKLayers.dll
├── sdk/tools/binary_store/D3D12Core.dll
├── sdk/tools/binary_store/dxcompiler.dll
└── sdk/tools/binary_store/dxil.dll
```

**Do NOT delete:**
- `sdk/tools/binary_store/FidelityFX_SC.exe` — Windows shader compiler
- `sdk/tools/ffx_shader_compiler/libs/{MD5,SPIRV-Reflect,tiny-process-library}/` — used by Linux port
- `sdk/tools/ffx_shader_compiler/libs/glslangValidator/` — uncertain, may be used by SC.exe on Windows

Verify `binary_store/glslangValidator.exe` exists before deleting `libs/glslangValidator/`.
