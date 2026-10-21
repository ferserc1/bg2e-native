# lightmap_generator

Headless UV2 AO/GI baking on `bg2e::draw` / `bg2e::bake`.

The CMake target is `lightmap_generator`. It uses the existing `bundle_app`
resource rules and automatic distribution discovery. On macOS run the executable
inside `lightmap_generator.app/Contents/MacOS/`; other platforms place it in
the normal product directory. Select Vulkan or Metal explicitly when needed;
defaults are Metal on macOS and Vulkan elsewhere. Required bake/tracing support
must exist on the selected device; an unsupported job fails.

```sh
lightmap_generator --help

lightmap_generator model --context context.json --model target.obj \
    --output baked --mode rtgi --resolution 512 --frames 32 --backend vulkan

lightmap_generator prefab --context context.json --prefab prefab.json \
    --output baked --config settings.example.json --frames 64

lightmap_generator model --context context.json --model target.bg2 \
    --output baked --generate-uv2 false --write-model-copies true \
    --cpu-format rgb32f --overwrite false
```

Every option takes a value, either `--name value` or `--name=value`. Contexts are
scene JSON; prefabs may be scene arrays or single serialized nodes. Model inputs
are BG2, VWGLB or OBJ. No camera is required. Invalid required targets fail the
whole batch; there is no silent skipped-target success.

Useful previous CLI defaults are retained: RTGI, resolution 512, 16 completed UV
samples, 8 rays per pixel, 2 GI bounces, distance 50, PNG, generated UV2 and explicit
replacement enabled by default. `--frames` counts completed UV accumulation
samples, independently of rays per pixel. The offscreen context surface is 1×1;
UV surfaces use the requested bake resolution. No camera frame is recorded.

Generating UV2 also requires writing model copies to retain that atlas.
`--write-model-copies` defaults to the `--generate-uv2` value and may explicitly
request copies for existing UV2. `--generate-uv2 true --write-model-copies false`
is rejected. `--uv2-padding` controls generated atlas padding; `--dilation`
controls nearest-chart output padding. Both default to 4. For small resolutions,
reduce them explicitly. Existing UV2 is validated with the assembler's overlap
warning policy; malformed required geometry still fails.

`--cpu-format rgb8` uses `--format png|jpg|jpeg|bmp|tga`. JPEG is lossy. RGB8 GI
applies `--exposure` before linear UNORM clamp/round in the bake composition step.
`--cpu-format rgb32f` preserves HDR with no exposure/clamp and uses native
RGBA32F `.bg2tex` files (alpha one). The draw native loader is required for those
files; they are not general interchange HDR images. Explicit `--format` or
`--exposure` with RGB32F is rejected. AO uses `--ao-radius`; GI uses
`--gi-bounces` / `--max-distance`; explicit options for the wrong mode reject.
See standalone `--help` for all ranges/options.

## Settings configuration

`--config` reads the engine's strict SettingsCodec format **before GPU allocation**
and applies it to the actual batch LightmapSettings object. Explicit CLI scalar
settings override imported values regardless of argument order; final coupled
settings are validated after overrides. No saved Preferences values are loaded.

[settings.example.json](settings.example.json) is a complete small example. Its
schema is `bg2e.draw.settings`, version string `1`, renderer identity
`bg2e.bake.standalone.v1`, component ID `bake` and component type
`bg2e.bake.lightmapSettings.v1`. Numeric values are typed **decimal strings**,
including enums (mode AO=0/GI=1, CPU RGB8=0/RGB32F=1). Available properties are
resolution, accumulationFrames, mode, samplesPerPixel, giBounces, aoRadius,
maxRayDistance, rayBias, exposureEV, cpuFormat and dilationPixels.

Configuration covers the effects/settings exposed by the public bake contract.
It does not activate camera rendering, screen-space effects, denoise controls,
editor document/lightPose targets or an interactive renderer profile. The codec
rejects unknown components/properties by default. `--unknown-settings skip` keeps
syntax/type validation and reports skipped fields as warnings. Schema/version,
renderer identity and known component type mismatches always fail; renderer
profiles are not relabelled or silently translated. Mode/format-specific dormant
configuration values may be retained but are only used by their applicable mode.

Backend, input/output paths, image file encoding, overwrite and UV2/model-copy
controls belong to CLI job orchestration, not persisted graphics settings. The
configuration file itself is added to protected batch inputs. An explicit
`--shader-root` must name a deployed engine shader root containing both `draw/`
and `bake/`; it is also protected against output writes.

## Publication, cancellation and status

Success reports the authoritative `output/lightmaps.json` path. Resolve its
relative artifact paths to a complete unique generation; do not discover outputs
by scanning the directory. RGB maps, per-target metadata and optional complete
BG2/native-sidecar models are staged together. Copies retain model geometry,
submesh placement/materials and chain joints; they do not export node placement,
descendants or unrelated components. Original materials/assets are untouched.

`--overwrite true` permits replacing the manifest and retains previous complete
generations. `false` refuses an existing manifest. Protected-source checks,
cooperative `.lightmaps.lock` and required custom dependency declarations come
from the batch writer. This executable registers no custom components/resolvers:
undeclared custom external inputs fail closed rather than being guessed.

SIGINT/SIGTERM handlers only store the request; the stopped batch checks it before
work, around completed samples and before commit. A cancelled job publishes no
new manifest. An interrupt after publication does not undo an already committed
complete generation. Existing output survives failures/cancellation. A crash or
final rename failure may leave a stale lock/stage or unreferenced generation.
Inspect those only after confirming no writer is alive; cleanup of previous
generations is manual. Atomic manifest visibility does not promise power-loss
durability or protection against hostile concurrent filesystem changes.

| Exit | Meaning |
|---|---|
| 0 | Help or complete committed batch after successful cleanup |
| 2 | Invalid arguments, paths or settings configuration |
| 1 | Engine/backend, assembly, required capability, bake, output or cleanup failure |
| 130 | SIGINT cooperative cancellation |
| 143 | SIGTERM cooperative cancellation |

Native initialization/cleanup is owned by OffscreenApplication. All scene/baker
owners die before Engine cleanup. Success is printed after cleanup; a cleanup
failure may report code 1 even if a complete manifest was already committed.

## Validation status

Agent checks are static only; no configure, build, shader compilation, executable
or probe was run. [CLI probes](../../tests/bake/cli/README.md) and
[batch probes](../../tests/bake/batch/README.md) define lead-run checks, including
both native backends, installed resources, error paths and model reload. Full
product validation belongs to M08-05.
