#!/bin/bash
#
# update_libraries.sh
#
# Builds the bg2e library with CMake and updates the bg2e headers and
# current-platform library in a standalone project previously created with
# create_standalone_project.sh.
#
# The build uses a dedicated directory (build-update-libraries/) so it does
# not interfere with the CLion build directory nor with the temporary build
# trees used by the package_*.sh scripts.
#
# Usage:
#   ./scripts/update_libraries.sh <standalone_project_path> Debug|Release
#

set -e

die() {
    echo "ERROR: $1" >&2
    exit 1
}

info() {
    echo "  $1"
}

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <standalone_project_path> Debug|Release"
    exit 1
fi

BUILD_CONFIG="$2"
case "${BUILD_CONFIG}" in
    Debug|Release) ;;
    *) die "Configuration must be Debug or Release." ;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

[ -d "$1" ] || die "Standalone project directory does not exist: $1"
TARGET_PATH="$(cd "$1" && pwd)"

case "${TARGET_PATH}" in
    /|"${REPO_ROOT}")
        die "Refusing unsafe target path: ${TARGET_PATH}"
        ;;
esac

case "$(uname -s)" in
    Linux*)
        CURRENT_PLATFORM="linux"
        LIBRARY_NAME="libbg2e.so"
        CMAKE_GENERATOR="Ninja"
        ;;
    Darwin*)
        CURRENT_PLATFORM="macos"
        LIBRARY_NAME="libbg2e.dylib"
        CMAKE_GENERATOR="Xcode"
        ;;
    *)
        die "This script only supports Linux and macOS. On Windows, use update_libraries.ps1"
        ;;
esac

VULKAN_SDK="${VULKAN_SDK:-}"
[ -d "${VULKAN_SDK}" ] || die "Set the VULKAN_SDK environment variable to a valid Vulkan SDK directory."

command -v cmake >/dev/null 2>&1 || die "Required command not found: cmake"

BUILD_DIR="${REPO_ROOT}/build-update-libraries"
PRODUCT_DIR="${BUILD_DIR}/bin/${CURRENT_PLATFORM}"

# Multi-config generators (Xcode) append the configuration name to the
# runtime output directory; single-config generators (Ninja) do not.
if [ "${CMAKE_GENERATOR}" = "Xcode" ]; then
    SOURCE_LIBRARY="${PRODUCT_DIR}/${BUILD_CONFIG}/${LIBRARY_NAME}"
else
    SOURCE_LIBRARY="${PRODUCT_DIR}/${LIBRARY_NAME}"
fi

SOURCE_INCLUDE_DIR="${REPO_ROOT}/lib/include"
TARGET_INCLUDE_DIR="${TARGET_PATH}/include"
TARGET_LIBRARY_DIR="${TARGET_PATH}/lib/${CURRENT_PLATFORM}"

[ -f "${SOURCE_INCLUDE_DIR}/bg2e.hpp" ] || die "Cannot find bg2e.hpp at ${SOURCE_INCLUDE_DIR}/bg2e.hpp"
[ -d "${SOURCE_INCLUDE_DIR}/bg2e" ] || die "Cannot find bg2e headers at ${SOURCE_INCLUDE_DIR}/bg2e"

# These files and directories identify the expected standalone project layout.
[ -f "${TARGET_PATH}/CMakeLists.txt" ] || die "Target does not look like a standalone project: missing CMakeLists.txt"
[ -f "${TARGET_PATH}/cmake/standalone_utils.cmake" ] || die "Target does not look like a generated standalone project: missing cmake/standalone_utils.cmake"
[ -d "${TARGET_INCLUDE_DIR}" ] || die "Target does not contain an include directory"
[ -d "${TARGET_LIBRARY_DIR}" ] || die "Target does not contain lib/${CURRENT_PLATFORM}"

echo "bg2e Standalone Library Updater"
echo "================================"
info "Repository:    ${REPO_ROOT}"
info "Target:        ${TARGET_PATH}"
info "Platform:      ${CURRENT_PLATFORM}"
info "Configuration: ${BUILD_CONFIG}"
info "Build dir:     ${BUILD_DIR}"
echo ""

echo "Configuring and building bg2e (${BUILD_CONFIG})..."
cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" \
    -DVULKAN_SDK="${VULKAN_SDK}" \
    -DPRODUCT_DIR="${PRODUCT_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_CONFIG}" \
    -DBG2E_BUILD_EXAMPLES=OFF \
    -DBG2E_BUILD_TESTS=OFF

cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}" --target bg2e --parallel

[ -f "${SOURCE_LIBRARY}" ] || die "Build finished but the library was not found at ${SOURCE_LIBRARY}"

echo "Updating engine headers..."
rm -rf "${TARGET_INCLUDE_DIR}/bg2e"
cp "${SOURCE_INCLUDE_DIR}/bg2e.hpp" "${TARGET_INCLUDE_DIR}/bg2e.hpp"
cp -R "${SOURCE_INCLUDE_DIR}/bg2e" "${TARGET_INCLUDE_DIR}/bg2e"

echo "Updating pre-compiled library..."
cp "${SOURCE_LIBRARY}" "${TARGET_LIBRARY_DIR}/${LIBRARY_NAME}"

echo ""
echo "Standalone project libraries updated successfully."
