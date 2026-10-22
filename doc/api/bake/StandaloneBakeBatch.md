# StandaloneBakeBatch

**Header:** `<bg2e/bake/StandaloneBakeBatch.hpp>`; also in `<bg2e/bake/all.hpp>`.
**Namespace:** `bg2e::bake`.

```cpp
static Result run(draw::Engine& engine,
    const StandaloneBakeSceneAssembler::Assembly& assembly,
    const std::filesystem::path& outputDirectory,
    const Options& options, ProgressCallback onProgress = {});
```

This member consumes a private retained Assembly on a stopped initialized
offscreen Engine. It realizes deferred targets, validates prepared identities and
Engine ownership, copies Options, preflights the complete output batch, prepares
one context and serially releases each baker before the next. No map is applied
to source materials between targets. UV2 generation belongs to the assembler.
Required malformed targets fail the batch instead of becoming skipped successes.

## Options

| Field | Default / purpose |
|---|---|
| `settings` | Actual LightmapSettings, copied/frozen by run. |
| `byteFormat` | db::ImageFormat::PNG; only the RGB8 file encoding. |
| `overwriteOutputs` | false; permits replacing the final manifest, never protected inputs. |
| `writeModelCopies` | false; independent single-model snapshots with actual map pixels. |
| `shaderRoot` | Empty selects deployed draw/bake shader root. |
| `environment` | Optional validated EnvironmentResources; null uses authored/default environment tools. |
| `protectedInputs` | Extra source files or output-exclusion directories. |
| `resolveDependencies` | Complete file declaration for retained unrecognized/custom component types. |
| `current` | Optional read-only CPU job/document-validity callback; false cancels. |

The CLI deliberately chooses different generation/overwrite/copy defaults; do not
copy those defaults into an API caller implicitly. Pass injection is available
on the [standalone context](StandaloneBakerContext.md), not in this Options struct.

## Progress, results and failures

ProgressCallback is `std::function<bool(const Progress&)>`. Progress has zero-based
targetIndex, targetCount, completedSamples and requestedSamples. It runs before
each target and after each completed sample, including the last. False cancels;
exceptions propagate. Both callbacks must be CPU-only with scene/resource producers
stopped; they cannot submit competing GPU work. Hidden mutable custom state must
remain stopped or be covered by current. Caller settings edits do not change the
frozen job, while observed scene/resource changes reject publication.

Result contains bakedCount, cancelled, manifestPath and vector of writer
OutputPaths. bakedCount is assigned only after complete commit. Cancelled results
have count zero/no new published paths; failures throw. Staged first targets are
not partial successes if a later target fails/cancels. Input/source signatures
and environment generations are rechecked at stopped boundaries and before
commit. Interrupts after publication do not undo a committed generation.

## Publication and models

[LightmapOutputWriter](LightmapOutputWriter.md) stages maps/metadata and optional
complete BG2/standard-image model copies, then publishes lightmaps.json LAST.
Resolve manifest-relative paths rather than scanning the directory. Original
assets/materials and previous committed generations remain unchanged. Model
copies carry model/submesh geometry/materials/joints, excluding node placement,
descendants and unrelated components; they are not exported prefab scenes.

The batch uses actual completed [typed output](LightmapOutput.md). RGB32F is exact
Radiance RGBE `.hdr`, not an 8-bit conversion. Required unsupported RT/kernel/
format support fails; there is no neutral no-RT bake fallback. Device/Engine must
outlive context/bakers/assembly and any externally retained GPU owner. Result
paths are CPU values and can survive normal native cleanup.

Sources: [header](../../../lib/include/bg2e/bake/StandaloneBakeBatch.hpp),
[implementation](../../../lib/src/bg2e/bake/StandaloneBakeBatch.cpp),
[CLI caller](../../../apps/lightmap_generator/src/main.cpp),
[tutorial](../../tutorials/draw/standalone_lightmaps.md).
