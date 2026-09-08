#!/usr/bin/env bash
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
status="$(git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none)"
[[ -z "$status" ]] || { printf 'Repository has pending changes:\n%s\n' "$status" >&2; exit 1; }
submodules="$(git -C "$repo" submodule status --recursive)"
if [[ "$submodules" =~ (^|$'\n')[+U-] ]]; then
    echo 'Submodules must be initialized and match their committed revisions.' >&2; exit 1
fi
[[ $# -ge 2 && $# -le 3 ]] || { echo "Usage: bash $0 Debug|Release DESTINATION [VULKAN_SDK]" >&2; exit 1; }
config="$1"
case "$config" in Debug|Release) ;; *) echo 'Configuration must be Debug or Release.' >&2; exit 1;; esac
[[ "$(uname -s)" == 'Linux' ]] || { echo 'Run this script on linux.' >&2; exit 1; }
sdk="${3:-${VULKAN_SDK:-}}"
[[ -d "$sdk" ]] || { echo 'Specify a valid Vulkan SDK directory.' >&2; exit 1; }
command -v cmake >/dev/null
commit="$(git -C "$repo" rev-parse HEAD)"
name="bg2engine_$(git -C "$repo" rev-parse --short=12 HEAD)"
# Require an existing destination so its canonical path can be checked first.
[[ -d "$2" ]] || { echo 'Create the destination parent directory first.' >&2; exit 1; }
destination="$(cd -- "$2" && pwd -P)"
case "$destination/" in "$repo/"*) echo 'Destination must be outside the repository.' >&2; exit 1;; esac
package="$destination/$name"
archive="$package.zip"
[[ ! -e "$package" && ! -e "$archive" ]] || { echo 'Package or ZIP already exists.' >&2; exit 1; }
build="$(mktemp -d "${TMPDIR:-/tmp}/bg2engine-build.XXXXXXXX")"
printf 'Building %s (%s) in %s\n' "$commit" "$config" "$build"
cmake -S "$repo" -B "$build" -G 'Ninja' \
    "-DVULKAN_SDK=$sdk" "-DPRODUCT_DIR=$build/bin/linux" "-DCMAKE_BUILD_TYPE=$config" \
    -DBG2E_BUILD_EXAMPLES=OFF -DBG2E_BUILD_TESTS=OFF -DBG2E_PACKAGE_APPS=ON
cmake --build "$build" --config "$config" --target bg2e_distribution --parallel
status="$(git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none)"
[[ -z "$status" && "$(git -C "$repo" rev-parse HEAD)" == "$commit" ]] || { echo 'Repository changed during build; package aborted.' >&2; exit 1; }
mkdir -- "$package"
cmake --install "$build" --config "$config" --prefix "$package" --component Distribution
printf '{"commit":"%s","configuration":"%s","platform":"linux"}\n' "$commit" "$config" > "$package/build-info.json"
(cd -- "$destination" && cmake -E tar cf "$archive" --format=zip -- "$name")
printf 'Package: %s\nArchive: %s\nBuild files retained: %s\n' "$package" "$archive" "$build"
