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

detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        CYGWIN*|MINGW*|MSYS*)    echo "windows";;
        *)          die "Unknown platform: $(uname -s)";;
    esac
}

CURRENT_PLATFORM="$(detect_platform)"

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
[ -d "${REPO_ROOT}/assets" ] || die "Cannot find assets directory at ${REPO_ROOT}/assets"
[ -d "${REPO_ROOT}/shaders" ] || die "Cannot find shaders directory at ${REPO_ROOT}/shaders"

# Check pre-compiled library exists
LIB_DIR=""
case "${CURRENT_PLATFORM}" in
    linux)
        LIB_FILE="${REPO_ROOT}/bin/linux/libbg2e.so"
        [ -f "${LIB_FILE}" ] || die "Cannot find pre-compiled library: ${LIB_FILE}\n  Build the project first: cmake --build build"
        LIB_DIR="${REPO_ROOT}/bin/linux"
        ;;
    macos)
        LIB_FILE="${REPO_ROOT}/bin/macos/libbg2e.dylib"
        [ -f "${LIB_FILE}" ] || die "Cannot find pre-compiled library: ${LIB_FILE}\n  Build the project first: cmake --build build"
        LIB_DIR="${REPO_ROOT}/bin/macos"
        ;;
    windows)
        if [ -f "${REPO_ROOT}/bin/windows/Release/bg2e.dll" ]; then
            LIB_DIR="${REPO_ROOT}/bin/windows/Release"
        elif [ -f "${REPO_ROOT}/bin/windows/Debug/bg2e.dll" ]; then
            LIB_DIR="${REPO_ROOT}/bin/windows/Debug"
        elif [ -f "${REPO_ROOT}/bin/windows/bg2e.dll" ]; then
            LIB_DIR="${REPO_ROOT}/bin/windows"
        else
            die "Cannot find pre-compiled library bg2e.dll\n  Searched in:\n    ${REPO_ROOT}/bin/windows/Release/\n    ${REPO_ROOT}/bin/windows/Debug/\n    ${REPO_ROOT}/bin/windows/\n  Build the project first: cmake --build build"
        fi
        [ -f "${LIB_DIR}/bg2e.lib" ] || die "Cannot find import library: ${LIB_DIR}/bg2e.lib\n  Build the project first: cmake --build build"
        ;;
esac

# Check compiled shaders exist
SHADER_DIR="${REPO_ROOT}/bin/${CURRENT_PLATFORM}/shaders"
[ -d "${SHADER_DIR}" ] || die "Cannot find compiled shaders at ${SHADER_DIR}\n  Build the project first: cmake --build build"
SPV_COUNT=$(find "${SHADER_DIR}" -name "*.spv" 2>/dev/null | wc -l)
[ "${SPV_COUNT}" -gt 0 ] || die "No .spv files found in ${SHADER_DIR}\n  Build the project first: cmake --build build"

# Check if target already exists
[ -d "${TARGET_PATH}" ] && die "Target directory already exists: ${TARGET_PATH}"

echo "Using libraries from: ${LIB_DIR}"
echo ""

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
# Copy files for current platform
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

echo "Copying pre-compiled library (${CURRENT_PLATFORM})..."
case "${CURRENT_PLATFORM}" in
    linux)
        cp "${REPO_ROOT}/bin/linux/libbg2e.so" "${TARGET_PATH}/lib/linux/"
        # Copy SDL2 library if available
        if [ -f "${REPO_ROOT}/bin/linux/libSDL2.so" ]; then
            cp "${REPO_ROOT}/bin/linux/libSDL2.so" "${TARGET_PATH}/lib/linux/"
        fi
        ;;
    macos)
        cp "${REPO_ROOT}/bin/macos/libbg2e.dylib" "${TARGET_PATH}/lib/macos/"
        # Copy SDL2 library if available
        if [ -f "${REPO_ROOT}/bin/macos/libSDL2.dylib" ]; then
            cp "${REPO_ROOT}/bin/macos/libSDL2.dylib" "${TARGET_PATH}/lib/macos/"
        fi
        # Copy Vulkan resources for macOS bundle
        if [ -n "${VULKAN_SDK}" ] && [ -d "${VULKAN_SDK}" ]; then
            echo "Copying Vulkan resources..."
            mkdir -p "${TARGET_PATH}/vulkan/icd.d"
            mkdir -p "${TARGET_PATH}/vulkan/explicit_layer.d"
            cp -r "${VULKAN_SDK}/share/vulkan/icd.d/"* "${TARGET_PATH}/vulkan/icd.d/" 2>/dev/null || true
            cp -r "${VULKAN_SDK}/share/vulkan/explicit_layer.d/"* "${TARGET_PATH}/vulkan/explicit_layer.d/" 2>/dev/null || true
            # Copy MoltenVK and validation layers
            for dylib in libMoltenVK.dylib libvulkan.*.dylib; do
                src="${VULKAN_SDK}/lib/${dylib}"
                if [ -f "${src}" ]; then
                    cp "${src}" "${TARGET_PATH}/lib/macos/"
                fi
            done
            # Copy validation layer dylibs
            for layer in libVkLayer_khronos_validation.dylib libVkLayer_khronos_synchronization2.dylib libVkLayer_khronos_shader_object.dylib; do
                src="${VULKAN_SDK}/lib/${layer}"
                if [ -f "${src}" ]; then
                    cp "${src}" "${TARGET_PATH}/lib/macos/"
                fi
            done
        fi
        ;;
    windows)
        cp "${LIB_DIR}/bg2e.dll" "${TARGET_PATH}/lib/windows/"
        cp "${LIB_DIR}/bg2e.lib" "${TARGET_PATH}/lib/windows/"
        # Copy SDL2 library if available
        if [ -f "${LIB_DIR}/SDL2.dll" ]; then
            cp "${LIB_DIR}/SDL2.dll" "${TARGET_PATH}/lib/windows/"
        fi
        ;;
esac

# ============================================================================
# Create TODO.md for other platforms
# ============================================================================

echo "Creating platform placeholders..."

generate_todo() {
    local platform_dir="$1"
    local platform_name="$2"

    cat > "${platform_dir}/TODO.md" << 'TODOMD'
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for this platform.

## Required Files

### Linux (`lib/linux/`)
- `libbg2e.so` — the bg2e shared library

### macOS (`lib/macos/`)
- `libbg2e.dylib` — the bg2e shared library

### Windows (`lib/windows/`)
- `bg2e.dll` — the bg2e dynamic library
- `bg2e.lib` — the bg2e import library

## How to Build for This Platform

1. Clone bg2e-native on the target platform:
   ```
   git clone --recursive <bg2e-native-repo-url>
   ```

2. Build the engine:
   ```
   cmake -S . -B build -G <generator> -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build
   ```

3. Copy the library files from `bin/<platform>/` to this directory.

## Notes

- The library must be compiled with a compatible C++20 compiler
- On Windows, both `.dll` and `.lib` files are required
- On macOS, you may also need to copy MoltenVK and Vulkan validation layer dylibs
TODOMD

    # Customize the TODO.md for the specific platform
    sed -i "s/This directory should contain the pre-compiled bg2e library for this platform./This directory should contain the pre-compiled bg2e library for ${platform_name}./" "${platform_dir}/TODO.md"
}

if [ "${CURRENT_PLATFORM}" != "linux" ]; then
    generate_todo "${TARGET_PATH}/lib/linux" "Linux"
fi

if [ "${CURRENT_PLATFORM}" != "macos" ]; then
    generate_todo "${TARGET_PATH}/lib/macos" "macOS"
fi

if [ "${CURRENT_PLATFORM}" != "windows" ]; then
    generate_todo "${TARGET_PATH}/lib/windows" "Windows"
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
echo "  ├── include/bg2e/"
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
