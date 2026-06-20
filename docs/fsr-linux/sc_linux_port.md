# FidelityFX Shader Compiler (FidelityFX_SC) - Linux Port Report

## Overview

The FidelityFX Shader Compiler (`FidelityFX_SC.exe`) is a Windows-only tool that generates all possible shader permutation combinations from `.hlsl`/`.glsl` source files, compiling them and exporting the results as C header files (`.h`) with embedded shader binaries. These headers are a dependency for integrating FSR 3.1 and other FidelityFX effects into a graphics engine.

This document analyzes the upstream Windows source code and the Linux port located at `lib/third_party/FidelityFX_SC_Linux/`.

---

## Directory Layout

### Upstream: `lib/third_party/FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler/`

```
ffx_shader_compiler/
  CMakeLists.txt              # Windows build (MSVC, links dxc, agilitysdk, etc.)
  GenerateSolution.bat        # VS solution generator
  src/
    ffx_sc.cpp                # Main entry point (wmain), Application class
    compiler.h                # Abstract compiler interface (ICompiler, Permutation)
    hlsl_compiler.h/cpp       # HLSL/DXC/FXC compiler backend (Windows-only)
    glsl_compiler.h/cpp       # GLSL/glslang compiler backend
    utils.h/cpp               # WCharToUTF8 / UTF8ToWChar (Windows-only)
    DXBCChecksum.h/cpp        # DXBC checksum (Windows-only, FXC backend)
    pch.hpp/cpp               # Precompiled header (Windows includes)
  libs/
    MD5/                      # MD5 hash library (used by GLSL compiler)
    SPIRV-Reflect/            # SPIR-V reflection library
    tiny-process-library/     # Cross-platform process spawning
    glslangValidator/         # Prebuilt glslangValidator.exe
    agilitysdk/               # Windows Agility SDK (not present in checkout)
    dxc/                      # DirectX Shader Compiler (not present in checkout)
```

### Linux Port: `lib/third_party/FidelityFX_SC_Linux/`

```
FidelityFX_SC_Linux/
  CMakeLists.txt              # Linux-only build (UNIX AND NOT APPLE guard)
  src/
    ffx_sc.cpp                # Modified main entry (main instead of wmain)
    compiler.h                # Modified: std::string instead of std::wstring
    glsl_compiler.h           # Identical to upstream
    glsl_compiler.cpp         # Minor fixes: .exe removal, \r handling
    pch.hpp                   # Stripped Windows headers, Linux-only includes
```

---

## File-by-File Analysis

### Files Present in Linux Port (Modified from Upstream)

| File | Upstream Path | Status |
|------|--------------|--------|
| `CMakeLists.txt` | New | Linux-only build file |
| `src/ffx_sc.cpp` | `src/ffx_sc.cpp` | **Heavily modified** |
| `src/compiler.h` | `src/compiler.h` | **1-line change** (`wstring` -> `string`) |
| `src/glsl_compiler.h` | `src/glsl_compiler.h` | **Identical** |
| `src/glsl_compiler.cpp` | `src/glsl_compiler.cpp` | **Minor changes** |
| `src/pch.hpp` | `src/pch.hpp` | **Rewritten** for Linux |

### Files Removed from Upstream (Not Needed on Linux)

| File | Reason |
|------|--------|
| `hlsl_compiler.h/cpp` | HLSL/DXC/FXC depends on Windows COM APIs (`dxcapi.h`, `d3dcompiler.h`, `atlcomcli.h`) |
| `utils.h/cpp` | Only contains `WCharToUTF8()` / `UTF8ToWChar()` for `wchar_t` conversion |
| `DXBCChecksum.h/cpp` | Only needed for FXC (DirectX bytecode) backend |
| `pch.cpp` | Empty PCH compilation unit, not needed |
| `GenerateSolution.bat` | Windows-only batch script |

### Third-Party Libraries (Reused from Upstream SDK)

| Library | Source Location | Usage |
|---------|----------------|-------|
| `tiny-process-library` | `FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler/libs/tiny-process-library/` | Spawns `glslangValidator` as child process |
| `SPIRV-Reflect` | `FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler/libs/SPIRV-Reflect/` | Extracts descriptor set/binding reflection from SPIR-V |
| `MD5` | `FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler/libs/MD5/` | Hashes SPIR-V binaries for deduplication |

---

## Key Modifications for Linux

### 1. String Types: `wchar_t` -> `char`

The largest category of changes. All `std::wstring`, `wchar_t`, and `L"..."` literals are replaced with `std::string`, `char`, and `"..."`.

- `compiler.h:122`: `std::vector<std::wstring> defines` -> `std::vector<std::string> defines`
- `ffx_sc.cpp`: Entry point changes from `int wmain(int argc, wchar_t** argv)` to `int main(int argc, char** argv)`
- All string utility functions (`Contains`, `StartsWith`, `Split`, `IsNumeric`) use `string_view`/`string` instead of `wstring_view`/`wstring`

### 2. Windows API Removal

| Windows API | Linux Replacement |
|-------------|-------------------|
| `PathAllocCanonicalize` | `std::filesystem::weakly_canonical` |
| `PathAllocCombine` | `fs::path(outputPath) / fileName` |
| `CreateDirectoryW` | `std::filesystem::create_directories` |
| `_wfopen_s` | `std::fopen` |
| `PathCchSkipRoot` | `std::filesystem::path::root_name()` / `root_directory()` |

### 3. HLSL Compiler Backends Removed

The `OpenSourceFile()` function in `ffx_sc.cpp` originally supported five compiler backends:
- `dxc` (DirectX Shader Compiler)
- `gdk.scarlett.x64` (Xbox Series X|S)
- `gdk.xboxone.x64` (Xbox One)
- `fxc` (Legacy D3DCompiler)
- `glslang` (GLSL/SPIR-V)

The Linux port **only supports `glslang`**. Attempting to use an unknown compiler throws an exception.

### 4. glslangValidator Path

| Aspect | Upstream | Linux Port |
|--------|----------|------------|
| Default executable name | `"glslangValidator.exe"` | `"glslangValidator"` |
| Path discovery | Hardcoded or prebuilt binary | `find_program()` with `VULKAN_SDK` hints |
| Compile-time injection | None | `FFX_LINUX_GLSLANG_DEFAULT` define via CMake |

### 5. Depfile Support

- Upstream supports both GCC-format and MSVC-format depfiles
- Linux port only supports GCC-format (`-deps=gcc`)
- `DumpDepfileMSVC()` function removed entirely

### 6. Robustness Fixes

- `glsl_compiler.cpp`: Added guard `if (!token.empty() && token.back() == '\r')` before `token.pop_back()` to handle `\r\n` line endings safely
- `glsl_compiler.cpp`: Added debug `fprintf(stderr, ...)` to print the exact glslang command being executed

---

## CMakeLists.txt Details

The Linux port's `CMakeLists.txt`:

```cmake
# Guard: only build on Linux
if(NOT (UNIX AND NOT APPLE))
    return()
endif()

# References upstream SDK libs (not re-vendored)
set(FFX_SC_TOOLS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler")
set(FFX_SC_LIBS_PATH  "${FFX_SC_TOOLS_PATH}/libs")

# Discovers glslangValidator from VULKAN_SDK
find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator glslang
    HINTS "${VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/bin"
)

# Target: ffx_sc_linux executable
add_executable(ffx_sc_linux
    src/ffx_sc.cpp                # Modified Linux entry point
    src/glsl_compiler.cpp         # Modified GLSL compiler
    "${FFX_SC_LIBS_PATH}/tiny-process-library/process.cpp"      # Upstream
    "${FFX_SC_LIBS_PATH}/tiny-process-library/process_unix.cpp"  # Upstream
    "${FFX_SC_LIBS_PATH}/SPIRV-Reflect/spirv_reflect.c"          # Upstream
    "${FFX_SC_LIBS_PATH}/MD5/md5.cpp"                             # Upstream
)

# Compile definition injects discovered glslang path
target_compile_definitions(ffx_sc_linux PRIVATE
    FFX_LINUX_GLSLANG_DEFAULT="${GLSLANG_VALIDATOR}"
)

# C++17, links pthread
target_link_libraries(ffx_sc_linux PRIVATE pthread)
```

Key design decisions:
- **No re-vendoring**: Third-party libs are compiled directly from the upstream SDK path
- **Platform guard**: `if(NOT (UNIX AND NOT APPLE))` prevents accidental builds on other platforms
- **VULKAN_SDK dependency**: Uses `find_program()` to locate `glslangValidator` with `VULKAN_SDK` as a hint path
- **No HLSL dependencies**: `dxguid`, `agilitysdk`, `dxc` are not linked

---

## Architecture: How the Shader Compiler Works

```
                         ┌─────────────────────┐
                         │   Command Line Args  │
                         │  -D<Name>={<V1>,<V2>}│
                         │  -output <path>      │
                         │  -compiler glslang   │
                         └─────────┬───────────┘
                                   │
                         ┌─────────▼───────────┐
                         │  Permutation Engine  │
                         │  (Cartesian Product  │
                         │   of all -D options) │
                         └─────────┬───────────┘
                                   │
                    ┌──────────────┼──────────────┐
                    │              │              │
              ┌─────▼─────┐ ┌─────▼─────┐ ┌─────▼─────┐
              │ Perm #0   │ │ Perm #1   │ │ Perm #N   │
              │ -DVAR=0   │ │ -DVAR=1   │ │ ...       │
              └─────┬─────┘ └─────┬─────┘ └─────┬─────┘
                    │              │              │
              ┌─────▼──────────────▼──────────────▼─────┐
              │       glslangValidator (child process)   │
              │       GLSL -> SPIR-V compilation         │
              └─────────────────┬───────────────────────┘
                                │
              ┌─────────────────▼───────────────────────┐
              │          SPIRV-Reflect                    │
              │  Extract descriptor set/binding info      │
              └─────────────────┬───────────────────────┘
                                │
              ┌─────────────────▼───────────────────────┐
              │          MD5 Hash (deduplication)         │
              │  Identical SPIR-V binaries share a       │
              │  single entry via hash matching           │
              └─────────────────┬───────────────────────┘
                                │
              ┌─────────────────▼───────────────────────┐
              │          Header File Generation           │
              │                                          │
              │  Per-permutation: <hash>.h                │
              │    static const unsigned char             │
              │      g_<name>_<hash>_data[] = {...};      │
              │                                          │
              │  Master: <shader>_permutations.h          │
              │    - #include all permutation headers     │
              │    - PermutationKey union (bitfields)     │
              │    - PermutationInfo struct array          │
              │    - Indirection table (key -> index)     │
              └──────────────────────────────────────────┘
```

---

## Build Instructions (Linux)

```bash
# Prerequisites: VULKAN_SDK environment variable must be set
# glslangValidator must be available in $VULKAN_SDK/bin/

# From the project root (bg2e-native):
cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
cmake --build build

# The ffx_sc_linux binary will be in bin/linux/
```

---

## Integration with bg2e-native Build System

The FidelityFX_SC_Linux CMakeLists.txt is designed to be included as a subdirectory of the bg2e-native project. The `ffx_sc_linux` target can be used as a dependency for custom shader compilation commands that generate the FSR 3.1 shader permutation headers.

To use it in the engine's build, add a custom command like:

```cmake
add_custom_command(
    OUTPUT ${OUTPUT_DIR}/fsr3_permutations.h
    COMMAND ffx_sc_linux
        -source ${FSR3_SHADER_DIR}/ffx_fsr3.glsl
        -output ${OUTPUT_DIR}
        -name fsr3
        -compiler glslang
        -reflection
        -DFFX_FSR3_OPTION_UPSAMPLE={0,1,2}
        -DFFX_FSR3_OPTION_ACCUMULATE={0,1}
    DEPENDS ffx_sc_linux ${FSR3_SHADER_DIR}/ffx_fsr3.glsl
)
```

---

## Limitations of the Linux Port

1. **GLSL/SPIR-V only**: No HLSL/DXC/FXC compilation support. Only Vulkan GLSL shaders can be processed.
2. **No Xbox/GDK backends**: `gdk.scarlett.x64` and `gdk.xboxone.x64` compilers are removed.
3. **No MSVC depfiles**: Only GCC-format depfiles are supported (`-deps=gcc`).
4. **macOS not supported**: The CMake guard explicitly excludes Apple platforms (`UNIX AND NOT APPLE`).
5. **Debug output**: A `fprintf(stderr, ...)` debug line in `glsl_compiler.cpp` may produce verbose output.
