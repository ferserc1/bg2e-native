# lightmap_generator

`lightmap_generator` bakes lightmaps with the production Vulkan render engine
without a window, camera or application render loop. It loads a JSON context
scene and attaches one `.bg2` model or a prefab's complete subtree. The context
supplies geometry, lights and environment; only model/prefab Drawables become
output targets. Ray-tracing support and the engine's runtime shaders/assets are
required. Headless operation does not provide a CPU lighting fallback.

Sources: [main.cpp](../../../apps/lightmap_generator/src/main.cpp),
[CommandLine.hpp](../../../apps/lightmap_generator/src/CommandLine.hpp),
[CommandLine.cpp](../../../apps/lightmap_generator/src/CommandLine.cpp).

## Commands

```sh
lightmap_generator model --context studio.json --model chair.bg2 --output out/
lightmap_generator prefab --context studio.json --prefab sofa.json --output out/
lightmap_generator --help
```

Both forms generate UV2 and replace existing output files by default. Source
assets remain protected. To use an existing atlas and emit only images:

```sh
lightmap_generator model --context studio.json --model chair.bg2 --output out/ \
    --generate-uv2 false --overwrite false --mode rtao --frames 32
```

To bake a prefab with colored GI:

```sh
lightmap_generator prefab --context studio.json --prefab sofa.json --output out/ \
    --resolution 1024 --frames 32 --samples-per-pixel 16 \
    --mode rtgi --gi-bounces 3 --max-distance 50 --format png
```

Options accept `--name value` or `--name=value`. Names/subcommands are case
sensitive; mode, boolean and image-format values are case-insensitive. Booleans
require an explicit true/false value. Duplicate options, unexpected positional
arguments, missing values and unsupported options are rejected. One standalone
`--help` token anywhere in arguments causes help before parsing/engine startup;
`--help=value` is invalid. Use the simple help form above.

## Option reference

| Option | Default | Meaning and validation |
|---|---|---|
| `--context` | Required | Existing JSON scene document with a `scene` array. |
| `--model` | Required in model mode | Existing `.bg2` file; not accepted in prefab mode. |
| `--prefab` | Required in prefab mode | Existing scene JSON or serialized Node; not accepted in model mode. |
| `--output` | Required | Destination directory, created when a result is written. |
| `--format` | `png` | `png`, `jpg`, `jpeg`, `bmp`, `tga`; extension aliases may have a leading dot. JPEG uses `.jpg`. |
| `--resolution` | `512` | Positive UInt32 square bake resolution, also requested UV2 packing resolution. Device limits still apply. |
| `--frames` | `16` | Positive UInt32 accumulation updates per target. |
| `--mode` | `rtgi` | `rtao` or `rtgi`. |
| `--generate-uv2` | `true` | Generate atlases and export modified `.bg2` copies. False validates existing UV2 and writes images only. |
| `--overwrite` | `true` | Allow replacement of existing outputs. False makes existing output paths an error. |
| `--samples-per-pixel` | `8` | Positive UInt32 rays per covered texel/update; baker additionally requires at most `INT_MAX`. |
| `--gi-bounces` | `2` | Positive UInt32, explicitly accepted only with RTGI. |
| `--max-distance` | `50` m | Finite positive RTGI range, explicitly accepted only with RTGI. |
| `--help` | — | Print usage without initializing Vulkan. |

RTAO uses a fixed 0.1 m radius. CLI options do not expose `giRayBias`,
`exposureEV`, `cpuFormat`, `dilationPixels` or `uv2PaddingPixels`; their library
defaults are respectively 0.0005 m, zero, RGB8, four and four. There are no
`--exposure`, `--dilation` or `--padding` switches in the current parser.
The underlying batch defaults generation/overwrite to false; these true defaults
are assigned specifically by `parseOptions()`.

Source: [parser, validation and usage](../../../apps/lightmap_generator/src/CommandLine.cpp).

## Input assembly

Model mode places the target at identity transform under the context root.
Prefab mode preserves its subtree transforms and collects its Drawable nodes.
Context Drawables are occluders/contributors, not image targets. Files referenced
by JSON resolve relative to that JSON; material textures resolve from their
owning assets. Missing textures/models, malformed JSON or failed known-component
loading report errors. Unknown/missing component types are logged and skipped.
A prefab without Drawables is invalid. A camera is optional.

The CLI uses CPU-only target assembly in both UV2 branches. Generation remaps
vertices before GPU loading; existing-UV2 mode validates the supplied channel.
The validator accepts UV2 copied from UV1 if it is already a usable nonoverlapping
atlas. If at least one target is eligible, all target geometry is GPU-loaded,
including skipped targets, before one shared scene TLAS is built. There is no
TLAS rebuild for every target or sample.

Sources: [assembler](../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp),
[batch](../../../lib/src/bg2e/render/StandaloneBakeBatch.cpp).

## Output and skipped targets

| Input | `--generate-uv2` | Completed target outputs |
|---|---|---|
| Model | false | One image, no model copy. |
| Model | true | One image and one `.bg2` copy. |
| Prefab | false | One image per eligible Drawable, no model copies. |
| Prefab | true | One image and `.bg2` copy per eligible Drawable. |

With generation disabled, invalid existing UV2 is skipped with one warning.
With generation enabled, atlas generation or generated-atlas validation failure
also skips only the affected target with a warning. No eligible targets yields
a successful zero-baked summary. Structural input, preflight, GPU, scene-update
or write failures abort execution with an error.

Names derive from node-name hierarchy segments and sibling indices, then filename
sanitization; they are not simply guaranteed to equal the source model basename.
The writer preflights outputs before atlas generation/baking, rejects input
aliases and unresolved collisions, and handles permitted existing outputs.
Generated `.bg2` copies use UV2 and unit scale on each material's existing AO
texture property; required texture sidecars may also be copied. Source context,
prefab and `.bg2` files are not re-emitted or overwritten.

Outputs are 8-bit RGB files. RTGI values above one clip in RGB8 export. A dedicated
RGB illumination channel in materials is pending: the current carrier is AO,
whose ordinary material path is scalar 8-bit, although deferred shaders already
include an RGB path reusing its texture. Do not infer a dedicated material-channel
API from successful RGB image generation. See [material integration](../../architecture/lightmaps/output_and_materials.md).

The writer stages files and cleans newly committed artifacts on failure. It
commits files individually and removes old outputs before replacement; there is
no atomic image/model transaction or restoration of overwritten outputs.
See [LightmapOutputWriter](../../api/db/LightmapOutputWriter.md).

## Progress, interruption and exit status

Progress is printed on stdout as `Baking target X/Y: N/M samples`, with a carriage
return between updates. Warnings/errors go to stderr. Final baked/skipped counts
are printed on stdout. The displayed target index is one-based; the batch callback
index is zero-based and counts only eligible targets.

The SIGINT handler sets a flag checked by the progress callback. Cancellation
occurs at sample boundaries, not in the middle of GPU execution or during input
assembly/preflight. A target whose final sample just completed is still written;
a partially accumulated target is not. Previously completed outputs remain.

| Exit | Meaning |
|---|---|
| `0` | Help or completed batch, including batches with skipped/all-skipped targets. |
| `1` | Engine startup, assembly, bake, output or cleanup failure. |
| `2` | Argument parsing or preliminary path validation failure. |
| `130` | Batch cancelled through SIGINT/progress callback. |

Engine initialization and runtime failures are reported separately from parser
errors. `runBatch()` releases assembly/context/bakers before `Engine::cleanup()`.

## Implementation map

`lightmap_generator::Command` selects Model/Prefab. `Options` stores input/output
paths, image format, batch settings and flags tracking explicitly supplied
GI-only options. `helpRequested()`, `parseOptions()`, `validateOptions()` and
`printUsage()` implement the CLI boundary. Private `runBatch()` assembles targets,
installs SIGINT handling and calls `StandaloneBakeBatch::run()` with progress and
warning callbacks. `main()` maps failures to exit status and owns engine cleanup.
No `MainLoop` or `OffscreenApplication` is constructed.

Continue with the [architecture guide](../../architecture/lightmaps/index.md),
[batch API](../../api/render/StandaloneBakeBatch.md),
[assembler API](../../api/render/StandaloneBakeSceneAssembler.md), and
[settings reference](../../api/render/LightmapSettings.md).
