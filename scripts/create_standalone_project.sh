#!/bin/bash
#
# create_standalone_project.sh
#
# Generates a standalone CMake project for building bg2e applications
# outside the bg2e-native repository. Uses pre-compiled libraries.
#
# Usage:
#   ./scripts/create_standalone_project.sh <target_path> [project_name]
#
# Arguments:
#   target_path   - (required) Path where the project will be created
#   project_name  - (optional) CMake project name (default: bg2e_app)
#

set -e

# ============================================================================
# Helper functions
# ============================================================================

die() {
    echo "ERROR: $1" >&2
    exit 1
}

info() {
    echo "  $1"
}

write_linux_todo() {
    cat > "$1" << 'EOF'
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for Linux.

## Required Files

- `libbg2e.so` — the bg2e shared library

## How to Build for This Platform

1. Clone bg2e-native on a Linux machine:
   ```
   git clone --recursive <bg2e-native-repo-url>
   ```

2. Build the engine:
   ```
   cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build
   ```

3. Copy `bin/linux/libbg2e.so` to this directory.

## Notes

- The library must be compiled with a compatible C++20 compiler
- GCC 11+ or Clang 13+ is recommended
EOF
}

write_macos_todo() {
    cat > "$1" << 'EOF'
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for macOS.

## Required Files

- `libbg2e.dylib` — the bg2e shared library

## How to Build for This Platform

1. Clone bg2e-native on a macOS machine:
   ```
   git clone --recursive <bg2e-native-repo-url>
   ```

2. Build the engine:
   ```
   cmake -S . -B build -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build
   ```

3. Copy `bin/macos/libbg2e.dylib` to this directory.

## Notes

- Xcode is required for native file dialogs
- You may also need to copy MoltenVK and Vulkan validation layer dylibs
- Install the Vulkan SDK from https://vulkan.lunarg.com/sdk/home
EOF
}

write_windows_todo() {
    cat > "$1" << 'EOF'
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for Windows.

## Required Files

- `bg2e.dll` — the bg2e dynamic library
- `bg2e.lib` — the bg2e import library

## How to Build for This Platform

1. Clone bg2e-native on a Windows machine:
   ```
   git clone --recursive <bg2e-native-repo-url>
   ```

2. Build the engine:
   ```
   cmake -S . -B build -G "Visual Studio 17 2022" -DVULKAN_SDK=C:\VulkanSDK
   cmake --build build --config Release
   ```

3. Copy `bin/windows/bg2e.dll` and `bg2e.lib` to this directory.

## Notes

- Both `.dll` and `.lib` files are required
- The Vulkan SDK can be downloaded from https://vulkan.lunarg.com/sdk/home
EOF
}

# ============================================================================
# Detect script location and repository root
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ============================================================================
# Parse arguments
# ============================================================================

if [ -z "$1" ]; then
    echo "Usage: $0 <target_path> [project_name]"
    echo ""
    echo "  target_path   Path where the project will be created"
    echo "  project_name  CMake project name (default: bg2e_app)"
    exit 1
fi

TARGET_PATH="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
PROJECT_NAME="${2:-bg2e_app}"

# ============================================================================
# Detect platform
# ============================================================================

case "$(uname -s)" in
    Linux*)     CURRENT_PLATFORM="linux";;
    Darwin*)    CURRENT_PLATFORM="macos";;
    *)          die "This script only supports Linux and macOS. For Windows, use create_standalone_project.bat";;
esac

# ============================================================================
# Validate repository and binaries
# ============================================================================

echo "bg2e Standalone Project Generator"
echo "=================================="
info "Repository:  ${REPO_ROOT}"
info "Target:      ${TARGET_PATH}"
info "Project:     ${PROJECT_NAME}"
info "Platform:    ${CURRENT_PLATFORM}"
echo ""

[ -d "${REPO_ROOT}/lib/include/bg2e" ] || die "Cannot find bg2e headers at ${REPO_ROOT}/lib/include/bg2e"
[ -f "${REPO_ROOT}/lib/include/bg2e.hpp" ] || die "Cannot find bg2e.hpp at ${REPO_ROOT}/lib/include/bg2e.hpp"
[ -d "${REPO_ROOT}/assets" ] || die "Cannot find assets directory at ${REPO_ROOT}/assets"

# Check pre-compiled library exists
SHADER_DIR="${REPO_ROOT}/bin/${CURRENT_PLATFORM}/shaders"

if [ "${CURRENT_PLATFORM}" = "linux" ]; then
    LIB_FILE="${REPO_ROOT}/bin/linux/libbg2e.so"
    [ -f "${LIB_FILE}" ] || die "Cannot find pre-compiled library: ${LIB_FILE}\n  Build the project first: cmake --build build"
elif [ "${CURRENT_PLATFORM}" = "macos" ]; then
    LIB_FILE="${REPO_ROOT}/bin/macos/libbg2e.dylib"
    [ -f "${LIB_FILE}" ] || die "Cannot find pre-compiled library: ${LIB_FILE}\n  Build the project first: cmake --build build"
fi

[ -d "${SHADER_DIR}" ] || die "Cannot find compiled shaders at ${SHADER_DIR}\n  Build the project first: cmake --build build"
SPV_COUNT=$(find "${SHADER_DIR}" -name "*.spv" 2>/dev/null | wc -l)
[ "${SPV_COUNT}" -gt 0 ] || die "No .spv files found in ${SHADER_DIR}\n  Build the project first: cmake --build build"

[ -d "${TARGET_PATH}" ] && die "Target directory already exists: ${TARGET_PATH}"

# ============================================================================
# Create directory structure
# ============================================================================

echo "Creating directory structure..."

mkdir -p "${TARGET_PATH}/cmake"
mkdir -p "${TARGET_PATH}/include"
mkdir -p "${TARGET_PATH}/lib/linux"
mkdir -p "${TARGET_PATH}/lib/macos"
mkdir -p "${TARGET_PATH}/lib/windows"
mkdir -p "${TARGET_PATH}/shaders"
mkdir -p "${TARGET_PATH}/assets"
mkdir -p "${TARGET_PATH}/app/src"

# ============================================================================
# Copy common files (headers, assets, shaders, cmake modules)
# ============================================================================

echo "Copying engine headers..."
cp "${REPO_ROOT}/lib/include/bg2e.hpp" "${TARGET_PATH}/include/"
cp -r "${REPO_ROOT}/lib/include/bg2e" "${TARGET_PATH}/include/"

echo "Copying assets..."
cp -r "${REPO_ROOT}/assets/"* "${TARGET_PATH}/assets/"

echo "Copying compiled shaders..."
cp "${SHADER_DIR}/"*.spv "${TARGET_PATH}/shaders/"

echo "Copying CMake modules..."
cp "${REPO_ROOT}/cmake/FindVulkan.cmake" "${TARGET_PATH}/cmake/"
cp "${REPO_ROOT}/cmake/FindSDL2.cmake" "${TARGET_PATH}/cmake/"
cp "${REPO_ROOT}/cmake/standalone_utils.cmake" "${TARGET_PATH}/cmake/"

# ============================================================================
# Platform-specific: copy libraries
# ============================================================================

if [ "${CURRENT_PLATFORM}" = "linux" ]; then
    # -----------------------------------------------------------------------
    # Linux
    # -----------------------------------------------------------------------

    echo "Copying pre-compiled library (linux)..."
    cp "${REPO_ROOT}/bin/linux/libbg2e.so" "${TARGET_PATH}/lib/linux/"
    if [ -f "${REPO_ROOT}/bin/linux/libSDL2.so" ]; then
        cp "${REPO_ROOT}/bin/linux/libSDL2.so" "${TARGET_PATH}/lib/linux/"
    fi

    echo "Creating platform placeholders..."
    write_macos_todo "${TARGET_PATH}/lib/macos/TODO.md"
    write_windows_todo "${TARGET_PATH}/lib/windows/TODO.md"

elif [ "${CURRENT_PLATFORM}" = "macos" ]; then
    # -----------------------------------------------------------------------
    # macOS
    # -----------------------------------------------------------------------

    echo "Copying pre-compiled library (macos)..."
    cp "${REPO_ROOT}/bin/macos/libbg2e.dylib" "${TARGET_PATH}/lib/macos/"
    if [ -f "${REPO_ROOT}/bin/macos/libSDL2.dylib" ]; then
        cp "${REPO_ROOT}/bin/macos/libSDL2.dylib" "${TARGET_PATH}/lib/macos/"
    fi

    if [ -n "${VULKAN_SDK}" ] && [ -d "${VULKAN_SDK}" ]; then
        echo "Copying Vulkan resources..."
        mkdir -p "${TARGET_PATH}/vulkan/icd.d"
        mkdir -p "${TARGET_PATH}/vulkan/explicit_layer.d"
        cp -r "${VULKAN_SDK}/share/vulkan/icd.d/"* "${TARGET_PATH}/vulkan/icd.d/" 2>/dev/null || true
        cp -r "${VULKAN_SDK}/share/vulkan/explicit_layer.d/"* "${TARGET_PATH}/vulkan/explicit_layer.d/" 2>/dev/null || true
        for dylib in libMoltenVK.dylib libvulkan.*.dylib; do
            src="${VULKAN_SDK}/lib/${dylib}"
            if [ -f "${src}" ]; then
                cp "${src}" "${TARGET_PATH}/lib/macos/"
            fi
        done
        for layer in libVkLayer_khronos_validation.dylib libVkLayer_khronos_synchronization2.dylib libVkLayer_khronos_shader_object.dylib; do
            src="${VULKAN_SDK}/lib/${layer}"
            if [ -f "${src}" ]; then
                cp "${src}" "${TARGET_PATH}/lib/macos/"
            fi
        done
    fi

    echo "Creating platform placeholders..."
    write_linux_todo "${TARGET_PATH}/lib/linux/TODO.md"
    write_windows_todo "${TARGET_PATH}/lib/windows/TODO.md"
fi

# ============================================================================
# Generate CMakeLists.txt files from templates
# ============================================================================

echo "Generating CMakeLists.txt..."

TEMPLATE_DIR="${SCRIPT_DIR}/templates"
[ -f "${TEMPLATE_DIR}/CMakeLists.txt.in" ] || die "Cannot find template: ${TEMPLATE_DIR}/CMakeLists.txt.in"
[ -f "${TEMPLATE_DIR}/app_CMakeLists.txt.in" ] || die "Cannot find template: ${TEMPLATE_DIR}/app_CMakeLists.txt.in"

sed "s/\${PROJECT_NAME}/${PROJECT_NAME}/g" "${TEMPLATE_DIR}/CMakeLists.txt.in" > "${TARGET_PATH}/CMakeLists.txt"
sed "s/\${PROJECT_NAME}/${PROJECT_NAME}/g" "${TEMPLATE_DIR}/app_CMakeLists.txt.in" > "${TARGET_PATH}/app/CMakeLists.txt"

# ============================================================================
# Copy app/main.cpp from template
# ============================================================================

echo "Copying app/src/main.cpp from template..."

[ -f "${TEMPLATE_DIR}/main.cpp" ] || die "Cannot find template: ${TEMPLATE_DIR}/main.cpp"
cp "${TEMPLATE_DIR}/main.cpp" "${TARGET_PATH}/app/src/main.cpp"

# ============================================================================
# Print summary
# ============================================================================

echo ""
echo "Project created successfully!"
echo ""
echo "Project structure:"
echo "  ${TARGET_PATH}/"
echo "  ├── CMakeLists.txt"
echo "  ├── cmake/"
echo "  │   ├── standalone_utils.cmake"
echo "  │   ├── FindVulkan.cmake"
echo "  │   └── FindSDL2.cmake"
echo "  ├── include/"
echo "  │   ├── bg2e.hpp"
echo "  │   └── bg2e/"
echo "  ├── lib/"
echo "  │   ├── linux/"
echo "  │   ├── macos/"
echo "  │   └── windows/"
echo "  ├── shaders/"
echo "  ├── assets/"
echo "  └── app/"
echo "      ├── CMakeLists.txt"
echo "      └── src/"
echo "          └── main.cpp"
echo ""
echo "To build:"
echo "  cd ${TARGET_PATH}"
echo "  cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk"
echo "  cmake --build build"
echo ""
