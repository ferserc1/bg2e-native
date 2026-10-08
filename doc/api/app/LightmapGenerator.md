# Standalone Lightmap Generator

`lightmap_generator` is a headless command-line executable backed by the
production `bg2e::render` ray-tracing baker. It needs no window, camera, or
application render loop. The scene context, model/prefab targets, and engine
must remain available through the batch; the executable cleans the batch before
calling `Engine::cleanup()`.

## Usage

```text
lightmap_generator model --context scene.json --model chair.bg2 --output out/
    [--format png|jpg|jpeg|bmp|tga]
    [--resolution 512] [--frames 16] [--mode rtao|rtgi]
    [--generate-uv2 true|false] [--samples-per-pixel 8]
    [--gi-bounces 2] [--max-distance 50]

lightmap_generator prefab --context scene.json --prefab sofa.json --output out/
    [the same common options]
```

Options accept either `--name value` or `--name=value`. `--help` prints usage
and exits before initializing Vulkan.

| Option | Default | Rules |
|--------|---------|-------|
| `--context` | required | Context scene JSON. |
| `--model` / `--prefab` | required by subcommand | Model mode accepts one `.bg2`; prefab mode accepts one JSON subtree/scene. The other target option is contradictory. |
| `--output` | required | Output directory; created when needed. |
| `--format` | `png` | `png`, `jpg`/`jpeg`, `bmp`, or `tga`; JPEG output uses canonical `.jpg`. |
| `--resolution` | `512` | Positive square bake resolution; also used as generated UV2 atlas resolution. |
| `--frames` | `16` | Positive accumulation-frame count. |
| `--mode` | `rtgi` | `rtao` or `rtgi`. |
| `--generate-uv2` | `false` | Boolean. When true, creates UV2 and `.bg2` copies. |
| `--samples-per-pixel` | `8` | Positive samples per UV texel per update. |
| `--gi-bounces` | `2` | Positive; only valid in RTGI mode. |
| `--max-distance` | `50` metres | Positive finite RTGI ray distance; only valid in RTGI mode. RTAO uses a fixed 0.1 m radius. |

Invalid, duplicate, contradictory, missing, or mode-irrelevant options are
rejected before engine initialization. Unsupported image formats and malformed
numeric values are also rejected at this stage.

## Batch behavior

The command loads the context first, then attaches the model at the world origin
or the complete prefab subtree. It assembles targets with CPU-only loading and
hands the scene to one `StandaloneBakeBatch`. The batch validates existing UV2
or, when requested, applies `geo::GenerateUv2AtlasModifier` to CPU meshes and
validates the generated atlas. It then loads all target Drawables and BLASes,
including invalid-UV targets retained as potential occluders, before its single
`updateScene()`/TLAS build.

With `--generate-uv2=false`, targets that fail
[`geo::UvAtlasValidator`](../geo/UvAtlasValidator.md) are skipped and produce
one warning each. The usable-atlas test does not infer UV2 absence from equal
UV1/UV2 coordinates. With `--generate-uv2=true`, invalid generated UV2 is a
fatal error before the scene/TLAS update.

## Output branches

| Input | UV2 generation | Outputs |
|-------|----------------|---------|
| Model | false | One lightmap image if its existing UV2 is usable; otherwise one warning and no image. No `.bg2` copy. |
| Model | true | One lightmap image and a new `.bg2` target copy with the AO path assigned. |
| Prefab | false | One image for each target with usable UV2; one warning per skipped target. No `.bg2` copies. |
| Prefab | true | One lightmap image and one `.bg2` target copy per eligible target. |

Image extensions follow `--format`. Output names come from deterministic,
sanitized target identities. Existing outputs, unresolved name collisions, and
input aliases are fatal before any bake starts. The writer stages completed
target outputs; no partial image/model is left by a failed target write. A `.bg2`
copy associates the generated image with every submesh material's existing AO
property using UV set 1 and unit scale. The source model, source prefab, and
context JSON are never overwritten or re-emitted. Required material texture
sidecars may also be copied next to a generated `.bg2` copy.

The progress callback reports sample completion per eligible target. During the
bake batch, `SIGINT` requests cancellation; the batch stops before the next
sample, keeps outputs for fully completed targets, and returns exit code 130.
A successful batch returns 0; argument/usage errors return 2; engine, input,
bake, or output failures return 1. Skipped-target warnings and progress are
written to stderr, with baked and skipped counts summarized on stdout.

See [`StandaloneBakeBatch`](../render/StandaloneBakerContext.md#batch-workflow),
[`ImageFormat`](../db/ImageFormat.md), and
[`LightmapOutputWriter`](../db/LightmapOutputWriter.md) for reusable engine APIs.
