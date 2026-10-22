# Draw standalone lightmap generator

`lightmap_generator` bakes camera-less model/prefab AO/GI batches through
`bg2e::draw` and `bg2e::bake`. This is the canonical
[standalone lightmap workflow](lightmap_generator/index.md); the former renderer
implementation is removed. Required native tracing/kernel/format support must exist.

## Commands

```sh
lightmap_generator --help

lightmap_generator model --context context.json --model target.obj \
    --output baked --mode rtgi --resolution 512 --frames 32 --backend vulkan

lightmap_generator prefab --context context.json --prefab target.json \
    --output baked --generate-uv2 false --write-model-copies true \
    --cpu-format rgb32f --overwrite false
```

Context is a JSON scene. Model inputs are BG2/VWGLB/OBJ; a prefab accepts a
serialized Node or complete multi-root scene array. Context geometry contributes
but is not a target. Target transforms/parents remain authored. Unknown schema
component types use shared parser warning/skip; retained registered custom classes
need explicit hooks/dependencies. The shipped app registers no custom resolver.
Invalid required geometry/UV/material targets fail the batch; they are not skipped
into a successful zero-baked result. UV overlap policy is Warn.

Every option requires a value using `--name value` or `--name=value`. Names and
subcommands are case-sensitive; documented mode/backend/boolean/format values
are normalized where implemented. Standalone `--help` initializes no GPU.
Duplicates, unknown flags, mixed model/prefab arguments, invalid/nonfinite values
and unsupported explicit combinations reject before GPU allocation.

## Options and defaults

| Option | Default / meaning |
|---|---|
| `--context`, `--output` | Required context scene and destination. |
| `--model` / `--prefab` | Required matching input for the selected command. |
| `--backend vulkan\|metal` | Metal on macOS, Vulkan elsewhere; Metal rejects on other platforms. |
| `--config path` | Typed bake SettingsCodec document, no Preferences defaults. |
| `--unknown-settings reject\|skip` | reject; requires config; skip reports validated warnings. |
| `--mode rtao\|rtgi` | rtgi. |
| `--resolution pixels` | 512, square UV job resolution, 1..16384. |
| `--frames count` | 16, completed accumulation updates, 1..1048576. |
| `--samples-per-pixel count` | 8, rays per estimate, 1..64. |
| `--gi-bounces count` | 2, 1..8; explicit GI-only control. |
| `--max-distance meters` | 50; explicit GI-only control. |
| `--ao-radius meters` | 0.1; explicit AO-only control. |
| `--ray-bias meters` | 0.0005; selected distance/radius must exceed twice bias. |
| `--cpu-format rgb8\|rgb32f` | rgb8; RGB32F uses standard Radiance RGBE `.hdr`. |
| `--format png\|jpg\|jpeg\|bmp\|tga` | PNG, RGB8 only; JPEG is lossy. |
| `--exposure EV` | 0, [-32,32], explicit RGB8 GI conversion only. |
| `--dilation pixels` | 4, 0..resolution, nearest-chart output padding. |
| `--generate-uv2 true\|false` | true; stage an independent atlas before target upload. |
| `--uv2-padding pixels` | 4; only with generation, no greater than resolution. |
| `--write-model-copies true\|false` | Follows generate-uv2; may independently request copies for existing UV2. |
| `--overwrite true\|false` | true in CLI; API default is false. Replace manifest while retaining old generations. |
| `--shader-root directory` | Deployed engine shader root; explicit root must contain draw/ and bake/. |
| `--debug true\|false` | false, native validation. |

Distance/radius/bias scalar domains are finite [1e-6,1e9]; coupled validation still
applies. Generated UV2 requires model copies to retain its atlas: explicitly
disabling copies with generation is rejected. Small resolutions need correspondingly
smaller dilation/atlas padding. Format/exposure options explicitly reject with
RGB32F; explicit AO/GI-only controls must match the final selected mode. API
settings may retain dormant values from configuration without applying them in
the other mode/format.

## Typed configuration and precedence

The [shipped example](../../apps/lightmap_generator/settings.example.json)
uses schema `bg2e.draw.settings`, version string `1`, renderer
`bg2e.bake.standalone.v1`, component ID `bake`, type
`bg2e.bake.lightmapSettings.v1`. Numeric values use typed decimal **strings**,
including enum64 (mode AO=0/GI=1, CPU RGB8=0/RGB32F=1).

```sh
lightmap_generator model --context context.json --model target.bg2 \
    --output baked --frames 64 --config settings.example.json --resolution 1024
```

Import binds the actual batch LightmapSettings object before GPU initialization.
All explicit CLI scalar overrides apply **after** import regardless of argument
order, then final coupled settings validate. There is no parallel editable
preferences model or implicit saved graphics profile.

Available properties are resolution, accumulationFrames, mode, samplesPerPixel,
giBounces, aoRadius, maxRayDistance, rayBias, exposureEV, cpuFormat and dilationPixels.
Unsupported display/screen-space/editor/lightPose/custom effect fields reject by
default or receive syntax-validated skip warnings. Schema/version/renderer/known
type mismatches always fail; interactive renderer profiles are not relabelled.
Backend, paths, file codec and UV2/copy/overwrite orchestration are CLI controls,
not imported graphics preferences. Config file and explicit shader roots join
protected batch inputs.

## Output, interruption and status

Success reports `baked/lightmaps.json`, whose relative paths identify one complete
unique generation. Read that manifest instead of scanning for flat filenames.
RGB8 maps are linear UNORM; RGB32F maps use standard Radiance RGBE `.hdr`,
without exposure/clamp. RGBE quantizes color and does not preserve alpha or exact
float bits; the ordinary HDR loader reads these images. Raw effect precision is
validated separately. JSON metadata records mode/UV2/color/orientation/conversion,
samples and nearest-chart padding. Optional BG2 copies contain the assigned standard
map/other image sidecars and explicit linear UV2 bakedAO/bakedGI semantics.
They retain model/submesh geometry/materials/joints, not node placement or a full
prefab hierarchy. Original assets and source materials are untouched.

The batch stages all targets before publishing the authoritative manifest LAST.
Explicit overwrite permits its atomic switch and retains previous complete
generations. Protected sources, canonical/ancestor/hardlink/symlink checks and
`.lightmaps.lock` exclude output aliases/cooperating writers. Failed/cancelled
later targets do not publish partial success. Stale lock/stage or unreferenced
generations after a crash/final publication error require manual inspection once
no writer is alive. There is no power-loss or hostile filesystem race guarantee.

SIGINT/SIGTERM handlers only store a request, checked before work, around completed
samples and before commit. Cancellation returns no newly committed product;
interrupt after commit does not undo complete output. Progress is one line per
target. All scene/baker/native owners release before Engine cleanup; success is
reported only after cleanup succeeds. A cleanup error can return failure after a
complete manifest has already been published.

| Exit | Meaning |
|---|---|
| 0 | Help or complete committed batch and successful cleanup. |
| 2 | Invalid arguments, paths or typed configuration. |
| 1 | Initialization/backend, assembly, required capability, bake/output or cleanup failure. |
| 130 / 143 | Cooperative SIGINT / SIGTERM cancellation. |

## Build and resources

Target `lightmap_generator` is registered under apps/lightmap_generator and uses bundle_app.
The repository still requires VULKAN_SDK for its shared build, including a Metal
selection. Use Ninja on Linux and Visual Studio 17 2022 on Windows; macOS Xcode
is the native-dialog/distribution path (Ninja development builds retain the
existing dialog limitation). No new app-specific CMake variables are required.
macOS output is lightmap_generator.app; run its executable under
Contents/MacOS. Other platforms use the normal product directory. The typed
OffscreenApplication path creates a depthless 1×1 context and records no camera
frames; standalone UV surfaces use the requested resolution.

Reuse the engine's paired draw/bake shader aggregate, shader-only bundle refresh
and RuntimeShaders install. Check the **executed** root contains
bake/uv_surface/surface.vert.metallib for Metal or its .spv peer for Vulkan, plus
all required draw effects. Recursive Distribution discovery includes the new app;
source registration is not proof of a native installed/relocated run. Follow
[packaging instructions](../../scripts/PACKAGING.md) for clean-checkout,
external-destination and platform-generator requirements. Precompiled standalone
projects must retain full headers, native libraries and both shader trees; the
generated default main uses a deferred draw scene, not this CLI. Replace it with
an explicit draw/bake caller when creating a headless product. The current
creator/updater shader preflight is documented in
[build/distribution](../architecture/build_and_distribution.md#standalone-shader-preflight).

## Verification

[Independent artifact inspection](../../tests/bake/product/README.md) checks bounded
PNG/native maps, metadata and BG2 material sections against optional fixture
expectations. It does not reload geometry, establish lighting/chart correctness
or prove native/package/performance behavior. Ten synthetic inspector tests passed;
backend/scenario details of lead continuation acceptance remain unspecified.
Use separate native model reload, controlled scene/edge comparisons, source hashes,
process/failure and relocated-package checks. No agent build/runtime claim is
made by this documentation.

See [C++ tutorial](../tutorials/draw/standalone_lightmaps.md),
[batch API](../api/bake/StandaloneBakeBatch.md),
[publication contract](../api/bake/LightmapOutputWriter.md),
[application sources](../../apps/lightmap_generator/README.md).
