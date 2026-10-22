# LightmapOutputWriter (draw/bake)

**Header:** `<bg2e/bake/LightmapOutputWriter.hpp>`.
**Namespace:** `bg2e::bake`. The former DB per-target writer has been removed.
See [output ownership](../db/LightmapOutputWriter.md) for the migration boundary;
there is no legacy writer alias.

```cpp
LightmapOutputWriter(draw::Engine& engine, std::filesystem::path outputDirectory,
    LightmapSettings settings, db::ImageFormat byteFormat = db::ImageFormat::PNG);
void setOverwriteExisting(bool overwrite);
std::vector<OutputPaths> preflight(const std::vector<Target>& targets,
    bool writeModelCopies, const std::vector<std::filesystem::path>& protectedInputs = {},
    BakeDependencyResolver resolver = {});
void write(const LightmapPixels& pixels, const std::string& identity);
void validateSources() const;
void commit();
const std::filesystem::path& manifestPath() const;
const std::vector<OutputPaths>& outputs() const;
```

Declarations above are members. The writer is noncopyable and borrows live Engine/
Device lifetime for source validation and complete model snapshot. Construct with
validated frozen settings. Overwrite defaults false and must be set before
preflight. One successful preflight plans the whole nonempty batch before baking;
write stages each known identity once and commit requires every target. Neither
preflight nor an individual successful write publishes a target through the
manifest. Repeated/incomplete commit and wrong format/resolution/identity reject.

## Plans and protected inputs

Target contains identity, inputModelPath and shared node. Identities must be
nonempty/unique; all targets are attached draw Drawables under one scene root.
CPU geometry must agree with uploaded geometry. OutputPaths contains identity,
imagePath, metadataPath and optional modelPath. Names combine target index and a
sanitized identity inside a fresh unique generation, preventing path/case collisions.

Preflight protects source models/scene material images/environments/Drawable
references, original OBJ and successfully read MTL files, and caller-declared
context/prefab/config/other inputs. Include all job inputs when using the writer
directly; [StandaloneBakeBatch](StandaloneBakeBatch.md) supplies its Assembly inputs.
Disabled-node resources also remain protected. Paths retain lexical IO semantics;
canonical identities, ancestor checks and existing-file equivalence detect source
aliases including manifest hardlinks/symlinks. Overwrite never bypasses protection.

Exact builtin RTTI is recognized. Every retained unrecognized component/subclass
needs a BakeDependencyResolver returning `optional<vector<filesystem::path>>`.
nullopt or absent resolver rejects; an explicit empty vector declares no external
files. Enumerate all actual files, not directory placeholders. protectedInputs may
reserve directories as output-exclusion zones. Unknown schema types already
skipped by the parser are not loaded resources; the writer cannot infer arbitrary
external inputs from custom code.

File path/size/time checks and streamed content fingerprints, graph/component
references, geometry/material/native revisions, lights and selected environment
reject changed sources at job boundaries/commit. This is stopped-job consistency,
not a cryptographic integrity service or permission for unobserved concurrent GPU
writes. Resolvers are read-only CPU callbacks; keep producers stopped.

## Files, precision and complete model copies

RGB8 uses selected PNG/JPEG/BMP/TGA without a second exposure conversion.
Lossless formats are decoded and compared byte-for-byte; explicit JPEG is lossy
and receives a dimension check. RGB32F writes standard Radiance RGBE `.hdr`,
then verifies that it decodes with the expected dimensions and floating-point data.
RGBE quantizes color values and does not preserve alpha or exact float bits.
There is no gamma/exposure/clamp or 65504 cap in this encoder. Raw bake pass precision remains independently validated;
file encoding cannot recover already invalid/saturated input.

Per-target JSON declares identity/image/extent, linear color, top row v=0,
uvSet 1, indirectMultiplierRGB, bakedAO/bakedGI, CPU/storage format, conversion,
exposure, nearest-chart dilation and requested settings. Validity/chart coverage
is not inferred from alpha one or neutral white. See [LightmapOutput](LightmapOutput.md).

Optional model copies clone independent CPU/GPU mesh/material/native texture
owners, set the actual staged map on the AO slot with linear color/UV2/scale one/
channel zero and explicit baked AO/GI mode, then update clone materials. Complete
`db::storeDrawableBg2(path, engine, node)` copies assigned image sources and materializes GPU-only textures as PNG/HDR;
its bool-noexcept false is checked and fails before publication. Models contain
submesh placement/materials/joints, not node placement or a full hierarchy.
Exported sidecars no longer depend on staging paths. Source materials/assets are
never rewritten or temporarily adopted between targets.

## Authoritative manifest and failure limits

The output entry is `outputDirectory/lightmaps.json`. Its version 1 targets list
contains identity, image, metadata and optional model paths relative to that
directory. A simplified shape (generation/name are illustrative):

```json
{
  "version": 1,
  "targets": [{
    "identity": "target/0",
    "image": "lightmaps-unique-id/target-0-target_0.hdr",
    "metadata": "lightmaps-unique-id/target-0-target_0.lightmap.json",
    "model": "lightmaps-unique-id/target-0-target_0-model/model.bg2"
  }]
}
```

A private `.lightmaps-stage-<id>` directory holds the complete batch. Each model
has its own directory with standard image sidecars. `.lightmaps.lock` excludes cooperating
writers. Commit renames a complete unique generation, then atomically publishes
the manifest LAST through [db::publishFile](../db/FileIO.md). Existing manifests
require explicit overwrite; symlink/nonregular manifests reject. Readers resolve
the manifest, never a partially populated output directory.

Old generations are retained even on overwrite. Graceful cancel/error before
manifest publication preserves the previous complete product and removes owned
stage/lock directories during normal unwinding. Failure after generation rename
can leave an unreferenced complete generation. Crashes may leave stale lock/stage
directories. Inspect/reclaim only after confirming no writer is alive; garbage
collection/recovery are manual. This promises atomic manifest visibility for
cooperating stopped producers, not simultaneous multi-path replacement,
hostile-process filesystem security or power-loss durability.

Sources: [header](../../../lib/include/bg2e/bake/LightmapOutputWriter.hpp),
[implementation](../../../lib/src/bg2e/bake/LightmapOutputWriter.cpp),
[native complete IO](../db/DrawResourceIO.md),
[independent inspector](../../../tests/bake/product/README.md).
