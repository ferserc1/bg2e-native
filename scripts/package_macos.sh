#!/usr/bin/env bash
set -euo pipefail
usage()
{
    echo "Usage: bash $0 [--identity NAME] Debug|Release DESTINATION [VULKAN_SDK]" >&2
}
identity=""
positional=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --identity)
            [[ $# -ge 2 && -n "$2" && "$2" != --* ]] || {
                echo '--identity requires a nonempty signing identity.' >&2
                usage; exit 1
            }
            identity="$2"
            shift 2
            ;;
        --) shift; positional+=("$@"); break ;;
        --*) usage; exit 1 ;;
        *) positional+=("$1"); shift ;;
    esac
done
[[ ${#positional[@]} -ge 2 && ${#positional[@]} -le 3 ]] || { usage; exit 1; }
set -- "${positional[@]}"
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
status="$(git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none)"
[[ -z "$status" ]] || { printf 'Repository has pending changes:\n%s\n' "$status" >&2; exit 1; }
submodules="$(git -C "$repo" submodule status --recursive)"
if [[ "$submodules" =~ (^|$'\n')[+U-] ]]; then
    echo 'Submodules must be initialized and match their committed revisions.' >&2; exit 1
fi
config="$1"
case "$config" in Debug|Release) ;; *) echo 'Configuration must be Debug or Release.' >&2; exit 1;; esac
[[ "$(uname -s)" == 'Darwin' ]] || { echo 'Run this script on macos.' >&2; exit 1; }
sdk="${3:-${VULKAN_SDK:-}}"
[[ -d "$sdk" ]] || { echo 'Specify a valid Vulkan SDK directory.' >&2; exit 1; }
command -v cmake >/dev/null
if [[ -n "$identity" ]]; then
    command -v codesign >/dev/null
    command -v file >/dev/null
fi
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
cmake -S "$repo" -B "$build" -G 'Xcode' \
    "-DVULKAN_SDK=$sdk" "-DPRODUCT_DIR=$build/bin/macos" "-DCMAKE_BUILD_TYPE=$config" \
    -DBG2E_BUILD_EXAMPLES=OFF -DBG2E_BUILD_TESTS=OFF -DBG2E_PACKAGE_APPS=ON
cmake --build "$build" --config "$config" --target bg2e_distribution --parallel
status="$(git -C "$repo" status --porcelain=v1 --untracked-files=all --ignore-submodules=none)"
[[ -z "$status" && "$(git -C "$repo" rev-parse HEAD)" == "$commit" ]] || { echo 'Repository changed during build; package aborted.' >&2; exit 1; }
mkdir -- "$package"
cmake --install "$build" --config "$config" --prefix "$package" --component Distribution
# Verify canonical bundle and executable names before archiving.
for app in bg2e_composer model_edit lightmap_generator; do
    [[ -x "$package/$app.app/Contents/MacOS/$app" ]] || {
        printf 'Missing packaged application: %s\n' "$app" >&2; exit 1;
    }
done
# Sign nested native code first, then seal bundles from the inside out.
# Do not use --deep for signing: every embedded library gets its own signature.
if [[ -n "$identity" ]]; then
    printf 'Signing package with identity: %s\n' "$identity"
    while IFS= read -r -d '' binary; do
        description="$(LC_ALL=C file -b -- "$binary")"
        case "$description" in
            *Mach-O*) codesign --force --sign "$identity" -- "$binary" ;;
        esac
    done < <(find "$package" -type f -print0)
    while IFS= read -r -d '' bundle; do
        codesign --force --sign "$identity" -- "$bundle"
    done < <(find "$package" -depth -type d \( -name '*.app' -o -name '*.framework' -o -name '*.xpc' -o -name '*.appex' -o -name '*.bundle' \) -print0)
    while IFS= read -r -d '' app_bundle; do
        codesign --verify --deep --strict --verbose=2 -- "$app_bundle"
    done < <(find "$package" -type d -name '*.app' -print0)
fi
printf '{"commit":"%s","configuration":"%s","platform":"macos"}\n' "$commit" "$config" > "$package/build-info.json"
(cd -- "$destination" && cmake -E tar cf "$archive" --format=zip -- "$name")
printf 'Package: %s\nArchive: %s\nBuild files retained: %s\n' "$package" "$archive" "$build"
