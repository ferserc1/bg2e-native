# Standalone scene assembly and batch execution

## StandaloneBakeSceneAssembler

The assembler separates input validation and scene construction from baking.
`Assembly` holds the assembled shared Scene, context/input paths and `Target`
records; each target holds a node, backing `.bg2` path and stable output identity.
Context geometry participates in tracing but is excluded from target enumeration.

`assembleModel()` loads a context scene, validates a `.bg2` mesh and material
assets, loads the model and sets its root transform to identity before attaching
it. Exactly one Drawable target must result. `assemblePrefab()` accepts a scene
array or serialized Node, validates dependencies and attaches the complete loaded
subtree to the context. Its hierarchy and transforms remain intact; each directly
attached Drawable below that subtree becomes a target.

Input paths become absolute and lexically normalized. Drawable asset names
resolve relative to the owning JSON and acquire `.bg2` extension. Referenced
filesystem material textures and environment images must exist. Known component
factories are checked; unknown/missing types are logged and skipped. Loaded
Drawable/component counts are checked against the validated expected counts.
This prevents an input dependency failure being mistaken for a smaller valid scene.

CPU-only assembly uses a dedicated prefab deserializer: it constructs ordinary
nodes/components but creates Drawables from `loadMeshBg2()` without loading their
GPU meshes. It preserves material names, groups and visibility. Context assets
are still GPU-loaded. Traversal verifies parent attachments and unique/nonempty
runtime node identities. Output identities are instead built from sanitized node
names and sibling indices, so filenames remain stable for a fixed hierarchy.

Sources: [assembler header](../../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp),
[implementation and private parsing records](../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp).

## Choosing the UV2 preparation boundary

Three-argument assembly overloads load targets on the GPU. Optional atlas
options passed to the assembler generate UV2 there and reload loaded targets,
with device waits; a failure throws for the assembly operation. The batch has a
different per-target policy, so its generation branch requires CPU-only assembly:

```cpp
// Same assembly shape used by lightmap_generator::runBatch().
bg2e::render::StandaloneBakeSceneAssembler assembler(&engine);
auto assembly = assembler.assemblePrefab(
    "studio.json", "sofa.json", std::nullopt, false);
bg2e::render::StandaloneBakeBatch::Options options;
options.generateUv2 = true;
auto result = bg2e::render::StandaloneBakeBatch::run(
    &engine, assembly, "out", bg2e::db::ImageFormat::PNG, options);
```

Adapted from [CLI main](../../../apps/lightmap_generator/src/main.cpp).
This ensures vertex remapping precedes GPU vertex/index upload and BLAS creation;
otherwise the UV surface and ray scene could reference different geometry arrays.

## StandaloneBakeBatch

`Options` groups lightmap settings, UV2 generation/padding and overwrite policy.
`Progress` describes the current eligible target and accumulation count.
`Result` records successful writes, skipped targets and cancellation.
`ProgressCallback` returns continuation/cancellation; `WarningCallback` receives
one diagnostic for each skipped target. Private `ValidTarget` associates assembly
metadata with a retained Drawable during execution.

The implementation has explicit boundaries:

1. Validate engine, scene, format and bake settings. Verify direct target Drawables
   and their root membership. Reject GPU-loaded targets when generation is requested.
2. Without generation, validate existing UV2 and exclude invalid targets with one
   warning each. With generation, initially consider all targets eligible.
3. Preflight output names, collisions and protected inputs for the eligible set.
4. With generation, run the CPU modifier and public validator per target. Failures
   are skipped with warnings, and remaining targets continue.
5. If any eligible target remains, load all assembly target Drawables, including
   skipped ones, and wait for GPU resources.
6. Initialize one standalone context, call `updateScene(0.0f)` once, then create a
   baker for each remaining eligible target.
7. Accumulate targets sequentially and export only those reaching the full count.

No eligible targets means an early result without bake preparation or image
writes. Output preflight intentionally precedes generated-atlas failures, so a
collision in a target that would later fail generation can still abort the batch.
The batch creates all eligible bakers before the sequential update loop;
per-target image memory is therefore allocated together, not streamed one baker
at a time. The shared TLAS contains context and every loaded target. Repeated
sampling and moving to the next target do not rebuild it.

Source: [batch declarations](../../../lib/include/bg2e/render/StandaloneBakeBatch.hpp),
[run implementation](../../../lib/src/bg2e/render/StandaloneBakeBatch.cpp).

## Cancellation and write boundary

Progress runs before a target's first sample and after each synchronous sample.
Returning false prevents the next sample. If the current sample reaches the
requested count, the implementation still writes that completed target before
returning cancelled. Interrupted partial targets receive no output, while earlier
completed targets remain. Warnings/atlas-generation failures are recoverable skips;
GPU load, scene update, output preflight and output writes report exceptions.
The CLI maps cancellation to exit 130 and ordinary runtime failures to exit 1.

The writer plans `.bg2` copies only for the UV2 generation branch. Existing-atlas
baking exports images without updating input assets. Generation exports modified
model copies and assigns the image via AO metadata; it never serializes a new
context or prefab. See [output architecture](output_and_materials.md) and the
[application guide](../../apps/lightmap_generator/index.md).
