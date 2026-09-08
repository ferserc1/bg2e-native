#!/usr/bin/env bash

# Build and package bg2engine and its applications from a clean Git checkout.
#
# The resulting directory contains the applications, engine library, shaders,
# and assets installed by the CMake Distribution component. Linux system and
# runtime libraries are deliberately not bundled; the applications use the
# libraries provided by the target system. A compressed tar archive containing
# the same directory is created beside it.
#
# Usage:
#   ./scripts/package_linux.sh [--allow-dirty] [--overwrite] Debug|Release DESTINATION [VULKAN_SDK]
#
# Example:
#   ./scripts/package_linux.sh Release "$HOME/packages" "$VULKAN_SDK"
#   ./scripts/package_linux.sh --allow-dirty --overwrite Debug "$HOME/packages" "$VULKAN_SDK"

set -euo pipefail

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly repo="$(cd -- "$script_dir/.." && pwd -P)"

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

usage() {
    printf \
        'Usage: %s [--allow-dirty] [--overwrite] Debug|Release DESTINATION [VULKAN_SDK]\n' \
        "$0" >&2
}

allow_dirty=false
overwrite=false

while (( $# > 0 )); do
    case "$1" in
        --allow-dirty) allow_dirty=true ;;
        --overwrite) overwrite=true ;;
        --) shift; break ;;
        -*) die "Unknown option: $1" ;;
        *) break ;;
    esac
    shift
done

if (( $# < 2 || $# > 3 )); then
    usage
    exit 2
fi

readonly configuration="$1"
readonly destination_argument="$2"
readonly vulkan_sdk="${3:-${VULKAN_SDK:-}}"

case "$configuration" in
    Debug|Release) ;;
    *) die 'Configuration must be Debug or Release.' ;;
esac

[[ "$(uname -s)" == 'Linux' ]] || die 'Run this script on Linux.'

require_command git
require_command cmake
require_command ninja

[[ -d "$vulkan_sdk" ]] || die 'Specify a valid Vulkan SDK directory or set VULKAN_SDK.'
[[ -d "$destination_argument" ]] || die 'Create the destination directory first.'

# Resolve the destination before creating anything, then ensure packaging cannot
# write into the source checkout and make it dirty during the build.
readonly destination="$(cd -- "$destination_argument" && pwd -P)"
case "$destination/" in
    "$repo/"*) die 'Destination must be outside the repository.' ;;
esac

# Packaging normally requires a reproducible, clean checkout. --allow-dirty is
# useful for local test packages that intentionally include uncommitted changes.
repository_status="$(
    git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none
)"
if [[ "$allow_dirty" == false && -n "$repository_status" ]]; then
    printf 'Repository has pending changes:\n%s\n' "$repository_status" >&2
    printf 'Pass --allow-dirty to package these changes anyway.\n' >&2
    exit 1
fi

submodule_status="$(git -C "$repo" submodule status --recursive)"
if [[ "$submodule_status" =~ (^|$'\n')[+U-] ]]; then
    die 'Submodules must be initialized and match their committed revisions.'
fi

readonly commit="$(git -C "$repo" rev-parse HEAD)"
readonly build_id="$(git -C "$repo" rev-parse --short=12 HEAD)"
readonly package_name="bg2engine_$build_id"
readonly package="$destination/$package_name"
readonly archive="$destination/$package_name.tar.gz"

if [[ -e "$package" || -L "$package" || -e "$archive" || -L "$archive" ]]; then
    if [[ "$overwrite" == false ]]; then
        die "Output already exists: $package or $archive. Pass --overwrite to replace it."
    fi

    # Both paths are derived from the validated destination and the hexadecimal
    # Git build ID, so removal cannot escape the intended output directory.
    rm -rf -- "$package"
    rm -f -- "$archive"
fi

# A unique build tree prevents stale binaries, caches, and shaders from entering
# the distribution. It is retained to make a failed build easier to diagnose.
readonly build="$(mktemp -d "${TMPDIR:-/tmp}/bg2engine-build.XXXXXXXX")"
readonly products="$build/bin/linux"
readonly staging_root="$build/package"
readonly staged_package="$staging_root/$package_name"
readonly staged_archive="$build/$package_name.tar.gz"

printf 'Building %s (%s) in %s\n' "$commit" "$configuration" "$build"

cmake -S "$repo" -B "$build" -G 'Ninja' \
    "-DVULKAN_SDK=$vulkan_sdk" \
    "-DPRODUCT_DIR=$products" \
    "-DCMAKE_BUILD_TYPE=$configuration" \
    -DCMAKE_JOB_POOLS=bg2e_package_link=1 \
    -DCMAKE_JOB_POOL_LINK=bg2e_package_link \
    -DBG2E_BUILD_EXAMPLES=OFF \
    -DBG2E_BUILD_TESTS=OFF \
    -DBG2E_PACKAGE_APPS=ON

# Each application currently copies resources into the shared product directory
# in a post-build step. The one-slot linker pool configured above serializes
# those post-build copies while compilation and shader generation stay parallel.
cmake --build "$build" \
    --config "$configuration" \
    --target bg2e_distribution \
    --parallel

# A clean build must remain clean. For an explicitly dirty build, at least guard
# against switching commits while binaries are being produced.
repository_status="$(
    git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none
)"
current_commit="$(git -C "$repo" rev-parse HEAD)"
if [[ "$current_commit" != "$commit" || ("$allow_dirty" == false && -n "$repository_status") ]]; then
    die 'Repository changed during the build; package aborted.'
fi

# Install and archive in the temporary tree so incomplete output is not exposed
# in the requested destination when installation or compression fails.
mkdir -p -- "$staged_package"
cmake --install "$build" \
    --config "$configuration" \
    --prefix "$staged_package" \
    --component Distribution

# CMake's runtime dependency scan copies parts of the host graphics, desktop,
# and C++ runtime stacks. Those copies can be incompatible with GPU drivers,
# Vulkan ICDs, plugins, and other components loaded from the target system. Keep
# only the project-owned engine library and rely on the target Linux system for
# all external shared libraries.
while IFS= read -r -d '' library; do
    library_name="${library##*/}"
    case "$library_name" in
        libbg2e.so|libbg2e.so.*) ;;
        *) rm -- "$library" ;;
    esac
done < <(
    find "$staged_package" -maxdepth 1 \
        \( -type f -o -type l \) \
        -name '*.so*' \
        -print0
)

printf \
    '{"commit":"%s","configuration":"%s","platform":"linux","builtAtUtc":"%s"}\n' \
    "$commit" "$configuration" "$(date -u +'%Y-%m-%dT%H:%M:%SZ')" \
    > "$staged_package/build-info.json"

(
    cd -- "$staging_root"
    cmake -E tar czf "$staged_archive" -- "$package_name"
)

mv -- "$staged_package" "$package"
mv -- "$staged_archive" "$archive"

printf 'Package: %s\nArchive: %s\nBuild files retained: %s\n' \
    "$package" "$archive" "$build"
