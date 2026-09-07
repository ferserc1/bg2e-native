#!/bin/bash
#
# update_libraries.sh
#
# Updates the bg2e headers and current-platform library in a standalone project
# previously created with create_standalone_project.sh.
#
# Usage:
#   ./scripts/update_libraries.sh <standalone_project_path>
#

set -e

die() {
    echo "ERROR: $1" >&2
    exit 1
}

info() {
    echo "  $1"
}

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <standalone_project_path>"
    exit 1
fi

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
        ;;
    Darwin*)
        CURRENT_PLATFORM="macos"
        LIBRARY_NAME="libbg2e.dylib"
        ;;
    *)
        die "This script only supports Linux and macOS. On Windows, use update_libraryes.ps1"
        ;;
esac

SOURCE_INCLUDE_DIR="${REPO_ROOT}/lib/include"
SOURCE_LIBRARY="${REPO_ROOT}/bin/${CURRENT_PLATFORM}/${LIBRARY_NAME}"
TARGET_INCLUDE_DIR="${TARGET_PATH}/include"
TARGET_LIBRARY_DIR="${TARGET_PATH}/lib/${CURRENT_PLATFORM}"

[ -f "${SOURCE_INCLUDE_DIR}/bg2e.hpp" ] || die "Cannot find bg2e.hpp at ${SOURCE_INCLUDE_DIR}/bg2e.hpp"
[ -d "${SOURCE_INCLUDE_DIR}/bg2e" ] || die "Cannot find bg2e headers at ${SOURCE_INCLUDE_DIR}/bg2e"
[ -f "${SOURCE_LIBRARY}" ] || die "Cannot find compiled library: ${SOURCE_LIBRARY}. Build bg2e-native first."

# These files and directories identify the expected standalone project layout.
[ -f "${TARGET_PATH}/CMakeLists.txt" ] || die "Target does not look like a standalone project: missing CMakeLists.txt"
[ -f "${TARGET_PATH}/cmake/standalone_utils.cmake" ] || die "Target does not look like a generated standalone project: missing cmake/standalone_utils.cmake"
[ -d "${TARGET_INCLUDE_DIR}" ] || die "Target does not contain an include directory"
[ -d "${TARGET_LIBRARY_DIR}" ] || die "Target does not contain lib/${CURRENT_PLATFORM}"

echo "bg2e Standalone Library Updater"
echo "================================"
info "Repository:  ${REPO_ROOT}"
info "Target:      ${TARGET_PATH}"
info "Platform:    ${CURRENT_PLATFORM}"
echo ""

echo "Updating engine headers..."
rm -rf "${TARGET_INCLUDE_DIR}/bg2e"
cp "${SOURCE_INCLUDE_DIR}/bg2e.hpp" "${TARGET_INCLUDE_DIR}/bg2e.hpp"
cp -R "${SOURCE_INCLUDE_DIR}/bg2e" "${TARGET_INCLUDE_DIR}/bg2e"

echo "Updating pre-compiled library..."
cp "${SOURCE_LIBRARY}" "${TARGET_LIBRARY_DIR}/${LIBRARY_NAME}"

echo ""
echo "Standalone project libraries updated successfully."
