# StandaloneBakeSceneAssembler

**Header:** `<bg2e/bake/StandaloneBakeSceneAssembler.hpp>`.
**Namespace:** `bg2e::bake`.

`StandaloneBakeSceneAssembler(draw::Engine&)` borrows an initialized offscreen
Engine and builds a private context/target candidate. It does not mutate another
application scene or publish product files. Assembly itself does not require RT;
creating a baker checks required native bake support.

```cpp
Assembly assembleModel(const std::filesystem::path& contextJson,
    const std::filesystem::path& model,
    std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt,
    bool loadTargetGpuResources = true) const;
Assembly assemblePrefab(const std::filesystem::path& contextJson,
    const std::filesystem::path& prefabJson,
    std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt,
    bool loadTargetGpuResources = true) const;
```

These declarations are members of StandaloneBakeSceneAssembler. There is no
`bake::SceneAssembler` alias. Model input supports BG2/VWGLB/OBJ through shared CPU
parsers; model placement starts at identity. Context requires scene JSON. A prefab
accepts a scene array or single serialized Node, preserving multiple roots; no
camera is needed. Resources resolve from the owning lexical input path, while
canonical paths identify protected sources. Unknown schema component types follow
the shared parser's warning/skip policy; retained registered custom components
still need their proper resource hooks and batch dependency declarations.

## Returned owners and identities

| Assembly member | Meaning |
|---|---|
| `scene` | Retained Scene containing context contributors and attached target tree. |
| `targetRoot` | Complete target subtree/container; context geometry is never selected as a target. |
| `contextSourcePath`, `inputSourcePath` | Normalized input references. |
| `targets` | Enabled authored target geometry below targetRoot. |
| `sourcePaths` | Canonical known standard dependencies, not a complete custom-resource protection proof. |

Each Target contains node, resolved inputSourcePath, stable outputIdentity,
UvAtlasValidation and optional generatedUv2 atlas metadata. Output identity derives
from hierarchy/name/sibling placement, not runtime node/pick/material-table IDs.
Retain the Scene alongside root/targets while using scene selection/caches.

## UV2 and realization

With atlas options, all target meshes are independently staged and validated
before adoption or initial GPU upload. Position/normal/tangent/UV1/submesh ownership
follows the shared geo atlas contract. Without options, existing UV2 is validated.
Overlap policy is Warn: overlap warnings alone do not skip a target. Invalid
required geometry/UVs/normals/affine transforms/material eligibility reject the
candidate. Non-scene helper Drawables are excluded; context resources remain
contributors, and target parents/transforms/chain data remain authored scene data.

`loadTargetGpuResources=false` leaves standard targets CPU-prepared and context
resources loaded. Later `targetRoot->loadResources(engine)` invokes normal/custom
resource hooks; it must retain prepared target drawable/mesh identities. Batch
performs deferred loading when needed, checks Engine ownership and CPU/uploaded
geometry agreement, and avoids repeating hooks on an already realized assembly.
Custom CPU preparation must supply a usable CPU mesh; it is not replaced by a
legacy realization path.

[StandaloneBakeBatch](StandaloneBakeBatch.md) supplements dependency inventory
with original OBJ/read MTL files and complete custom declarations, then protects
sources before output. Assembly alone neither claims that proof nor writes UV2
back to original assets. Optional output model copies retain generated UV2.

Sources: [header](../../../lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp),
[implementation](../../../lib/src/bg2e/bake/StandaloneBakeSceneAssembler.cpp),
[shared scene contracts](../scene/index.md),
[opt-in assembly probe](../../../tests/bake/assembly/README.md).
