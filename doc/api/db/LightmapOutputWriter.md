# LightmapOutputWriter

**Header:** `<bg2e/db/LightmapOutputWriter.hpp>`
**Namespace:** `bg2e::db`

Plans a complete batch's outputs before baking, then writes a completed target's
RGB image and optional `.bg2` copy. Non-copyable; move construction/assignment
are `noexcept`. Private state owns plans and sidecar-commit tracking.

## Types

| Type/field | Meaning |
|---|---|
| `Target::identity` | Nonempty unique stable target identity. |
| `Target::inputModelPath` | Backing source model path, protected against overwrite. |
| `Target::node` | Shared node with a standard Drawable, attached to the common scene root. |
| `OutputPaths::identity` | Planned target identity. |
| `OutputPaths::imagePath` | Final image path. |
| `OutputPaths::modelPath` | Optional model-copy path. |

## Members

```cpp
LightmapOutputWriter(std::filesystem::path outputDirectory, ImageFormat format);
void setOverwriteExisting(bool overwrite);
std::vector<OutputPaths> preflight(const std::vector<Target>& targets,
    bool writeModelCopies,
    const std::vector<std::filesystem::path>& protectedInputPaths = {});
void write(const render::LightmapPixels& pixels, std::string_view targetIdentity);
const std::filesystem::path& outputDirectory() const;
```

Construction normalizes the directory and requires a supported format.
Overwrite is false by default; set it before preflight (changing it afterward
throws). `preflight()` may succeed only once and requires nonempty targets.
It checks source paths, common root, texture inputs, output/sidecar collisions,
input aliases and existing outputs. Include context/prefab and other `.bg2`
assets in `protectedInputPaths`; target source models and scene textures are
protected automatically. Overwrite never bypasses input protection.

Names sanitize stable identities; collisions receive deterministic hash suffixes.
`preflight()` returns planned paths but does not write results. `write()` requires
preflight success, a known unwritten target, positive dimensions, matching variant
and exactly width×height×3 components. Missing preflight/repeated writes produce
logic errors; malformed targets/pixels or unknown identity produce argument errors;
filesystem/encoding failures produce exceptions. Moved-from state cannot be used
for preflight/write/directory access.

RGB8 is written directly. RGB32F must be finite and is clamped/rounded to RGB8
without exposure. There is no HDR image format. Model copies use a cloned
Drawable: every material gets the image via AO, UV set 1, unit scale and AO
channel zero. Source material attributes are not changed. A dedicated RGB
illumination material channel remains pending.

## Staging and overwrite limits

Image encoding uses a pending sibling filename retaining the selected extension.
Optional model and texture sidecars are staged, then committed individually.
Failure removes newly committed files for that call and staging files; completed
previous targets remain. Shared sidecars are tracked across target writes.
This is not an atomic multi-file transaction. With overwrite enabled, existing
files are removed before rename; their prior contents are not restored if a
later operation fails. New paths appearing after preflight are checked at write
and rejected when overwrite is disabled.

```cpp
bg2e::db::LightmapOutputWriter writer(outputDirectory, bg2e::db::ImageFormat::PNG);
writer.setOverwriteExisting(false);
std::vector<bg2e::db::LightmapOutputWriter::Target> targets;
for (const auto& target : assembly.targets)
    targets.push_back({target.outputIdentity, target.inputSourcePath, target.node});
auto paths = writer.preflight(targets, generateUv2, protectedInputPaths);
// After a target's bake completes:
writer.write(completedBaker->readPixels(), targets.front().identity);
```

Sources: [header](../../../lib/include/bg2e/db/LightmapOutputWriter.hpp),
[implementation](../../../lib/src/bg2e/db/LightmapOutputWriter.cpp).
See [ImageFormat](ImageFormat.md), [batch](../render/StandaloneBakeBatch.md),
and [output/material architecture](../../architecture/lightmaps/output_and_materials.md).
