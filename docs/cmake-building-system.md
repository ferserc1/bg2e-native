# CMake-Based Building System

This document describes in detail the build system of the bg2e-native project, based on CMake, and the utility functions provided to facilitate the creation of example applications.

## 1. Global Project Configuration

### 1.1. Root CMakeLists.txt

The main `CMakeLists.txt` file at the project root sets the global configuration:

```cmake
cmake_minimum_required(VERSION 3.18)
project(bg2e_native)

set(CMAKE_CXX_STANDARD 20)
```

**Key features:**

- **Minimum CMake version**: 3.18
- **C++ standard**: C++20 (required by the engine)
- **Automatic platform detection**: Linux, macOS, or Windows

### 1.2. VulkanSDK Configuration

The system requires the VulkanSDK:

```cmake
set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Path to VulkanSDK")
```

**Validation**: If `VULKAN_SDK` is not configured, the build fails with an informative message:

```
VulkanSDK not configured. VULKAN_SDK variable not set.
Please, set the VulkanSDK path manually:
  -DVULKAN_SDK=/path/to/VulkanSDK
```

### 1.3. Output Directories

Binaries are generated in `bin/{platform}/`:

```cmake
set(PRODUCT_DIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}")
```

| Platform | Output directory |
|----------|------------------|
| Linux    | `bin/linux/`     |
| macOS    | `bin/macos/`     |
| Windows  | `bin/windows/`   |

## 2. Build Structure

### 2.1. Build Order

The project is built in this order:

1. **Engine library** (`lib/`)
2. **Applications** (`apps/`)
3. **Examples** (`examples/`)

```cmake
add_subdirectory(lib)
add_subdirectory(apps)
add_subdirectory(examples)
```

### 2.2. Variables Exported by the Library

The `bg2e` library exports these variables for use in subdirectories:

| Variable | Description |
|----------|-------------|
| `BG2E_LIB_TARGET` | Library target name (`bg2e`) |
| `BG2E_INCLUDE_PATH` | Path to engine headers |
| `BG2E_SHADER_DIR` | Compiled shaders directory |
| `BG2E_RUNTIME_BINARY` | Path to the library binary |

## 3. Utility Functions in `cmake/utils.cmake`

### 3.1. `compile_shaders()`

Compiles GLSL shaders to SPIR-V format with proper dependencies:

```cmake
compile_shaders(
    TARGET_NAME      # Target that depends on the shaders
    VULKAN_SDK_PATH  # Path to VulkanSDK
    SRC_PATH         # Source shader directory (.glsl)
    DST_PATH         # Destination shader directory (.spv)
)
```

**How it works:**
- Finds all `.glsl` files in `SRC_PATH`
- Uses `glslang` from VulkanSDK to compile
- Creates a custom target `${TARGET_NAME}_shaders`
- Sets up dependencies to recompile only when sources change

### 3.2. `build_shaders()`

Alternative version that compiles shaders as post-build without dependencies:

```cmake
build_shaders(
    TARGET_NAME
    VULKAN_SDK_PATH
    SRC_PATH
    DST_PATH
)
```

**Difference from `compile_shaders()`:**
- Always recompiles all shaders after every build
- Does not track source file changes

### 3.3. `bundle_lib()` and `bundle_libs()`

Copies libraries to the macOS application bundle:

```cmake
# Single library
bundle_lib(
    TARGET_NAME my_app
    LIB_PATH ${SDL2_LIBRARY}
    SUBPATH "lib"  # Optional, default: "lib"
)

# Multiple libraries
bundle_libs(
    my_app
    "lib"  # Subdirectory inside the bundle
    "${VULKAN_SDK}/lib/libMoltenVK.dylib"
    "${VULKAN_SDK}/lib/libvulkan.1.dylib"
)
```

**Platform behavior:**
- **macOS**: Copies to `Contents/` inside the `.app` bundle
- **Linux/Windows**: Does nothing (can be called safely)

### 3.4. `bundle_resources()`

Copies resource directories to the bundle/application directory:

```cmake
bundle_resources(
    TARGET_NAME my_app
    SRC_PATH ${ASSETS_PATH}
    SUBPATH "assets"  # Subdirectory inside Resources/bin
)
```

**Destination by platform:**
- **macOS**: `Contents/Resources/{SUBPATH}`
- **Linux/Windows**: `{executable_directory}/{SUBPATH}`

### 3.5. `copy_vulkan_resources()`

Copies Vulkan-specific resources to the bundle (macOS only):

```cmake
copy_vulkan_resources(
    TARGET_NAME my_app
    VULKAN_SDK_PATH ${VULKAN_SDK}
)
```

**Copied content:**
- `icd.d/` and `explicit_layer.d/` (configuration)
- `libMoltenVK.dylib` and validation layers
- Symbolic link `libvulkan.1.dylib`

### 3.6. `bundle_app()` — Main Function

This is the main function for creating example applications:

```cmake
bundle_app(
    TARGET_NAME my_app          # Executable name
    SHADERS_SRC ${SHADERS_DIR}  # Optional: app-specific shaders
)
```

**Automatic actions:**

1. **Compilation**: Automatically finds `src/*.cpp` and `src/*.hpp`
2. **Linking**: Connects with the `bg2e` library and Vulkan
3. **Engine shaders**: Copies compiled engine shaders
4. **App shaders**: Compiles app-specific shaders if provided
5. **Resources**: Copies assets to the output directory
6. **Libraries**: Copies SDL2 and the bg2e library (macOS)
7. **Vulkan**: Copies Vulkan resources (macOS)

## 4. External Dependency Configuration

### 4.1. Vulkan (`cmake/FindVulkan.cmake`)

Exports platform-based variables:

| Variable | Linux | macOS | Windows |
|----------|-------|-------|---------|
| `VULKAN_INCLUDE_PATH` | `${VULKAN_SDK}/include` | `${VULKAN_SDK}/include` | `${VULKAN_SDK}/Include` |
| `VULKAN_LIB` | `libvulkan.so.1.x.x` | `libvulkan.x.x.x.dylib` | `vulkan-1.lib` |
| `VULKAN_LIB_VERSION` | `1.x.x` | `x.x.x` | `1` |

### 4.2. SDL2 (`cmake/FindSDL2.cmake`)

Searches for SDL2 in different locations depending on the platform:

- **macOS**: Inside the VulkanSDK (avoids Homebrew)
- **Linux**: Standard system paths
- **Windows**: Inside the VulkanSDK

### 4.3. Third-Party Dependencies (`lib/cmake/deps.cmake`)

Includes vendored libraries as git submodules:

| Library | Type | Usage |
|---------|------|-------|
| bg2-io | C | .bg2 format |
| bg2-scene | C++ | Scenes |
| imgui | C++ | User interface |
| nativefiledialog | C/C++ | File dialogs |
| stb_image | C | Image loading |
| tinyobj | C | OBJ models |
| cgltf | C | glTF models |

## 5. Platform-Specific Configuration

### 5.1. Linux

```cmake
# GTK3 for NFD and UI scale
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# OpenGL for some functions
find_package(OpenGL REQUIRED)

# Wayland for UI scale detection
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
```

### 5.2. macOS

```cmake
# Apple frameworks
target_link_libraries(${APP_TARGET_NAME} PRIVATE
    "-framework AppKit"
    "-framework Cocoa"
    "-framework Foundation"
    "-framework UniformTypeIdentifiers"
)

# Bundle configuration
set_target_properties(${APP_TARGET_NAME} PROPERTIES
    INSTALL_RPATH "@executable_path/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Disable automatic code signing
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
```

### 5.3. Windows

```cmake
# Runtime library
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Parallel compilation
add_compile_options(/MP)

# Link with SDL2main
list(APPEND SDL2_LIBRARIES ${SDL2MAIN_LIBRARY})
```

## 6. Workflow for Creating an Example

### 6.1. Directory Structure

```
examples/{example_name}/
├── CMakeLists.txt
├── shaders/
│   └── src/          # GLSL shaders (optional)
└── src/
    └── main.cpp      # Main source code
```

### 6.2. Minimal CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME my_example)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})
```

### 6.3. CMakeLists.txt with Shaders

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME my_example)
set(APP_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/src")
bundle_app(TARGET_NAME ${APP_TARGET_NAME} SHADERS_SRC ${APP_SHADERS})
```

### 6.4. Registering the Example

Add to `examples/CMakeLists.txt`:

```cmake
add_subdirectory(my_example)
```

### 6.5. Basic Source Code

```cpp
#include <bg2e.hpp>

class MyDelegate : public bg2e::render::RenderLoopDelegate,
                  public bg2e::app::InputDelegate,
                  public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine* vulkan) override {
        RenderLoopDelegate::init(vulkan);
    }

    void initScene() override {
        // Scene initialization
    }

    VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* colorImage,
        const bg2e::render::vulkan::Image* depthImage,
        const bg2e::render::vulkan::Image* msaaDepthImage,
        bg2e::render::vulkan::FrameResources& frameResources
    ) override {
        // Rendering code
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
        // UI setup
    }

    void drawUI() override {
        // UI rendering
    }
};

class MyApp : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override {
        auto delegate = std::shared_ptr<MyDelegate>(new MyDelegate());
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);
    }
};

int main(int argc, char** argv) {
    bg2e::app::MainLoop mainLoop("org.bg2engine.examples.my_example");
    MyApp app;
    app.init(argc, argv);
    return mainLoop.run(&app);
}
```

## 7. Build Commands

### 7.1. Initial Configuration

```bash
cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
```

### 7.2. Build

```bash
cmake --build build
```

### 7.3. Build with Specific Generator

**Linux (Ninja):**
```bash
cmake -S . -B build -G Ninja -DVULKAN_SDK=$VULKAN_SDK
cmake --build build
```

**macOS (Xcode - for native file dialogs):**
```bash
cmake -S . -B build -G Xcode -DVULKAN_SDK=$VULKAN_SDK
cmake --build build
```

**Windows (Visual Studio):**
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -DVULKAN_SDK=C:\VulkanSDK
cmake --build build --config Release
```

## 8. Common Troubleshooting

### 8.1. VulkanSDK Not Found

```
VulkanSDK not configured. VULKAN_SDK variable not set.
```

**Solution**: Set the `VULKAN_SDK` environment variable or pass `-DVULKAN_SDK=/path` to CMake.

### 8.2. Shaders Not Compiling

**Check:**
1. `glslang` exists in `${VULKAN_SDK}/bin/`
2. Shaders are in the `shaders/src/` directory
3. Shaders have the `.glsl` extension

### 8.3. Libraries Not Found on macOS

**Solution**: Verify that libraries are inside the VulkanSDK and not from Homebrew.

### 8.4. Code Signing Error on macOS

**Solution**: The project automatically disables code signing. If the error persists, clean the `build/` folder and reconfigure.

## 9. Quick Function Reference

| Function | Purpose | Main Use |
|----------|---------|----------|
| `compile_shaders()` | Compiles GLSL→SPIR-V with dependencies | Engine library |
| `build_shaders()` | Always compiles GLSL→SPIR-V | Dependency-free alternative |
| `bundle_lib()` | Copies a library to the bundle | SDL2, bg2e on macOS |
| `bundle_libs()` | Copies multiple libraries | Vulkan dylibs on macOS |
| `bundle_resources()` | Copies resource directories | Shaders, assets |
| `copy_vulkan_resources()` | Copies Vulkan resources | macOS only |
| `bundle_app()` | **Creates a complete application** | Examples and apps |

## 10. Additional Notes

- **No test framework**: Examples are the primary verification surface.
- **Incremental builds**: CMake only recompiles what is necessary thanks to configured dependencies.
- **Portability**: The system works on Linux, macOS, and Windows with platform-specific configurations.
- **Extensions**: New utility functions can be added in `cmake/utils.cmake` following the same pattern.