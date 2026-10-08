# LightmapOutputWriter

**Header:** `<bg2e/db/LightmapOutputWriter.hpp>`

**Namespace:** `bg2e::db`

`LightmapOutputWriter` plans stable output names for all eligible targets before
baking, then writes one RGB lightmap image for each completed target. Optional
model copies are produced only when UV2 generation is enabled.

```cpp
#include <bg2e/db/LightmapOutputWriter.hpp>
#include <bg2e/render/LightmapBaker.hpp> // LightmapPixels

db::LightmapOutputWriter writer(outputDirectory, db::ImageFormat::PNG);

std::vector<db::LightmapOutputWriter::Target> targets;
for (const auto& target : assembly.targets) {
    targets.push_back({ target.outputIdentity, target.inputSourcePath, target.node });
}

const auto paths = writer.preflight(targets, generateUv2, protectedInputPaths);
// Bake only after preflight succeeds.
auto pixels = completedBaker->readPixels();
writer.write(pixels, targets.front().identity);
```

Each `Target` contains a stable identity, its source `.bg2` path, and its
attached node. `preflight()` returns an image path and optional `.bg2` path for
each target. Pass the context and model/prefab inputs, plus any non-target
`.bg2` assets, in `protectedInputPaths`. Target source `.bg2` paths and loaded
scene material/environment textures are also protected.

## Output and safety behavior

- Filenames are derived from sanitized target identities; colliding sanitized
  identities receive deterministic suffixes. Unresolved collisions are fatal.
- Every output and required texture sidecar is preflighted for existing paths,
  input aliases, and collisions with other batch outputs. By default existing
  files are never overwritten; calling `setOverwriteExisting(true)` before
  `preflight()` allows preexisting outputs to be replaced atomically on write.
  Context and prefab JSON files are never emitted.
- Image writes use a temporary sibling filename that retains the selected
  extension, then rename after success. `.bg2` copies and referenced material
  texture sidecars are staged with the image; a target's final files are
  committed together.
- When `writeModelCopies` is true, each copy is serialized from a Drawable
  clone. The generated image is assigned through the existing AO property on
  every submesh material, with UV set 1 and unit scale. Loaded source materials
  and input `.bg2` files are not modified.
- RGB8 pixels are written directly. RGB32F pixels are checked for finite values,
  clamped to `[0, 1]`, and rounded once to RGB8 before calling `saveImage`.

Use `writeModelCopies == generateUv2`. When it is false, only images are
written. Image-format aliases and canonical extensions are documented in
[`ImageFormat`](ImageFormat.md).
