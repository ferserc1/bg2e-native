# StandaloneBakeSceneAssembler

**Header:** `<bg2e/render/StandaloneBakeSceneAssembler.hpp>`
**Namespace:** `bg2e::render`

Loads context geometry, environment and lights and attaches model/prefab targets
to one scene. It does not bake or write output files.

## Data types

| Type/field | Meaning |
|---|---|
| `Target::node` | Shared node directly containing the target Drawable. |
| `Target::inputSourcePath` | Absolute normalized backing `.bg2` path. |
| `Target::outputIdentity` | Deterministic hierarchy name with sibling indices; used for output naming. |
| `Assembly::scene` | Shared assembled scene containing context and targets. |
| `Assembly::contextSourcePath` | Absolute normalized context JSON path. |
| `Assembly::inputSourcePath` | Absolute normalized model `.bg2` or prefab JSON path. |
| `Assembly::targets` | Targets in traversal order; context Drawables are excluded. |

## Members

`explicit StandaloneBakeSceneAssembler(Engine*)` requires a non-null initialized
engine with ray-tracing support. All methods below are `const` and return `Assembly`.

```cpp
Assembly assembleModel(const std::filesystem::path& contextJson,
                       const std::filesystem::path& modelBg2,
                       std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt) const;
Assembly assembleModel(const std::filesystem::path& contextJson,
                       const std::filesystem::path& modelBg2,
                       std::optional<geo::Uv2AtlasOptions> uv2Options,
                       bool loadTargetGpuResources) const;
// assemblePrefab has the same overloads with prefabJson instead of modelBg2.
```

Three-argument overloads GPU-load targets. Passing `false` defers only target
GPU loading; context geometry is still loaded. `assembleModel` accepts `.bg2`
and attaches exactly one target with identity transform. `assemblePrefab`
accepts a scene document (`scene` array) or a serialized `type: Node` object,
attaches the complete subtree and preserves its transforms. Context input must
be a scene document. Asset paths resolve relative to their owning files.

Optional atlas options generate UV2 immediately; already loaded targets are
reloaded after a device wait. Atlas-generation failures here throw. For batch
UV2 generation use `std::nullopt, false` and let the batch apply its per-target
skip policy instead.

```cpp
bg2e::render::StandaloneBakeSceneAssembler assembler(engine);
auto assembly = assembler.assemblePrefab(
    "studio.json", "sofa.json", std::nullopt, false);
// Pass assembly to StandaloneBakeBatch::run().
```

Missing/malformed input and dependencies, unusable `.bg2` geometry/material
counts, failed known-component loading, attachment errors and duplicate node
identities are reported as exceptions. Unknown or missing component types are
logged and skipped. Prefabs must contain target Drawables. Output identities
are hierarchy-based, not random runtime identifiers.

Sources: [header](../../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp),
[implementation](../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp).
See [scene assembly architecture](../../architecture/lightmaps/standalone_scene_and_batch.md).
