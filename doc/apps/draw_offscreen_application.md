# Draw offscreen scene CLI

Target: `draw_offscreen_application`. Source:
[examples/draw/13_offscreen_application](../../examples/draw/13_offscreen_application/README.md).
The CLI uses draw over Vulkan/Metal and renders the deferred sample scene with
textured/transparent models, floor, four lights and an HDR environment. Camera/light
animation is frozen and editor UI, selection, gizmos and overlays are absent.
The former rendering framework has been removed; this is the canonical camera
export example, separate from UV bake batch output.

## Run and options

From the deployed executable directory (use the executable inside the macOS app
bundle when applicable):

```sh
./draw_offscreen_application --output-dir renders/frame --width 1280 --height 720 --scale 0.5 --frames 8 --backend vulkan
```

On macOS, choose `--backend metal` for native Metal. The platform default is Metal
on macOS and Vulkan elsewhere. No automatic backend switching occurs.

| Option | Default / validation |
|---|---|
| `--output-dir DIR` | `offscreen_output`; nonempty; directory created before GPU initialization |
| `--width N --height N` | 1920×1080; positive integers supplied together |
| `--internal-width N --internal-height N` | Optional paired positive fixed internal size |
| `--scale N` | Optional 0.25..1; incompatible with fixed internal dimensions |
| `--frames N` | 1; exact completed scene samples, 1..1048576 |
| `--backend vulkan\|metal` | Platform default |
| `--graphics-config FILE` | Optional strict exported draw-settings JSON |
| `--skip-unknown-settings` | Opt-in skip with diagnostics; strict rejection by default |
| `--help` / `-h` | Print usage and return zero without GPU initialization |

Repeated options (apart from help), unknown options, missing values and conflicting
resolution forms reject. The CLI's output dimensions/backend/N are job values.
Explicit internal-size/scale arguments override imported settings; absent arguments
retain renderer/imported values. The example opts out of persistence. Settings
exports containing per-run `light/<id>`, environment IDs or example-only
`lightPose/<id>` need matching catalog IDs or the explicit skip policy; there is
no name remap. Skipped properties are printed. A valid requested optional effect
value may still fall back if RT/FSR is unavailable. Reduced internal resolution
remains valid without FSR.

## Output and errors

Success publishes exactly `<output-dir>/scene.png` and prints the completed sample
count and absolute output path. Output dimensions remain full size even with reduced
internal resolution. Every sample contributes its reconstructed linear HDR scene
signal; final tone/color and optional SMAA run once on the mean. The implementation
uses synchronous readback and CPU double sums, then an RGBA16F upload. No hidden
warm-up frames are discarded.

The final surface is R8G8B8A8_UNORM. The writer preserves top-to-bottom rows and
RGBA bytes without another color conversion or row flip. It writes
`scene.pending.png`, decodes it and checks exact dimensions/bytes, then publishes
`scene.png` using a same-directory hard link that refuses an existing destination.
The destination filesystem must support hard links. Existing final or pending files
are refused; failure attempts to remove a newly written pending file. Success
attempts to remove the pending link after publication. Permissions/filesystem
cleanup errors can leave that link, so inspect the directory after interrupted or
failed jobs. The exact decode check proves codec preservation, not visual scene
correctness. There is no CLI option for HDR/JPEG/BGRA output or overwrite.

Errors print a diagnostic on stderr and return 1. GPU work is drained and delegate
resources are cleaned before Engine destruction. No partially accumulated image
is advertised as a completed result.

For comparison with the interactive deferred example, match renderer/camera/scene
settings, freeze animation and disable overlays, then compare N=1. N>1 can differ
from an individual interactive frame because it averages history-filtered scene
samples. Independent orientation/content and Vulkan/Metal checks belong to the
project lead; no backend-specific image evidence is asserted by this guide.

## Build and resources

Use the repository's configured build and platform generator: Ninja on Linux,
Xcode on macOS for working native dialogs, Visual Studio 17 2022 on Windows.
VULKAN_SDK is the required CMake setting; Metal shaders also need Apple's compiler
toolchain. Build target `draw_offscreen_application`; it is registered in
`examples/CMakeLists.txt` and uses `bundle_app_sdl`. Its resources follow the
standard `bin/<platform>` or macOS bundle layout. Keep engine native draw shaders,
assets and required runtime libraries together; copying the executable alone is
insufficient. The scene uses `two_submeshes.obj`, its material JPGs and
`HDR_multi_nebulae_1.hdr` from the bundled assets.

See [shader deployment](../api/gpu/ShaderLibraries.md),
[packaging/standalone/library updates](../../scripts/PACKAGING.md) and
[the tutorial](../tutorials/draw/13_offscreen_application.md). These instructions
are source-checked; no build or packaging run was performed for this documentation.
