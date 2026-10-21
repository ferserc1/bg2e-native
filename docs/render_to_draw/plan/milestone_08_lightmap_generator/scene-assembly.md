# M08-02 — Standalone scene and UV2 assembly delivered

[Step](step_02_scene-assembly.md) · [Context contract](standalone-context.md) · [Progress](../progress.md) · [Preservation baseline](../code-reconciliation-2026-10-21.md)

Baseline HEAD `78943d5820e69363c942d71136f479a0df87128e`, with inherited uncommitted M07 documentation, source/plan audit and M08-01 implementation preserved. The lead's continuation accepts M08-01 under the agreed convention; compilation/checks are assumed successful, with backend/scenario coverage unspecified. This delivery implements M08-02, not batch publication or the CLI.

## Public API and ownership

[StandaloneBakeSceneAssembler.hpp](../../../../lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp) and [implementation](../../../../lib/src/bg2e/bake/StandaloneBakeSceneAssembler.cpp) define `bake::StandaloneBakeSceneAssembler(draw::Engine&)`, exposed through bake/all.hpp. This is the actual class name replacing the never-implemented `SceneAssembler` forward declaration; no production alias/wrapper is introduced. The constructor requires a live windowless Engine and retains its device identity for replacement checks. Engine/Device outlive all returned assemblies and GPU owners. Stop scene/resource producers throughout assembly. Assembly itself does not require RT availability; the required effect profile remains the standalone baker's responsibility.

`assembleModel(contextJson, model, optionalUv2Options, loadTargetGpuResources=true)` accepts standard BG2/VWGLB or OBJ. BG2 input/output joints, material attributes, names/groups/visibility and identity model placement are preserved. OBJ uses the existing readObj CPU parser and attributes. No direct glTF-hierarchy model input is added; convert through existing public import/scene IO before using this API.

`assemblePrefab(contextJson, prefabJson, optionalUv2Options, loadTargetGpuResources=true)` accepts a scene document or a serialized Node. Context must be a scene document. There is no camera requirement and no Composer worker/server dependency. Each call builds a new private candidate, so a failed input/target/resource cannot alter an earlier Assembly or running context. Assembly returns:

- `scene`: complete context sources plus attached target subtree.
- `targetRoot`: the independent model/prefab subtree, suitable for target-only export later.
- `contextSourcePath`/`inputSourcePath`: normalized absolute input paths.
- `targets`: enabled authored scene-geometry nodes below targetRoot, each with its node, resolved geometry source path (or owning prefab for in-memory custom geometry), deterministic outputIdentity and UV2 validation/generated-atlas metadata.
- `sourcePaths`: unique canonical known context/input/standard geometry/material texture/environment files. Canonical protection identities are separate from lexical read paths: symlinked input documents still resolve relative references from the supplied file location.

Output identities use sanitized ASCII node-name segments and sibling positions. They remain stable for the same hierarchy across fresh runtime node IDs and avoid repeated-name/path-separator collisions. They are not pick IDs, UUIDs, descriptor addresses or a global material identity.

## Representation, custom components and helper exclusion

Context IO uses db::readSceneDocument. Single-Node prefab representation needs a small adapter using the engine JsonParser; component/hierarchy parsing always delegates to Scene::deserialize and registered graphics-free factories. The adapter validates scene/Node shape and fileType rather than implementing a separate graph parser. Unknown/untyped/nonobject component and child handling is the shared schema policy; registered parser/constructor/resource failures remain errors. There is no serialized-versus-loaded component-count equality that would turn warning/skip back into an error.

Prefab transfer uses Scene::releaseSceneRoot, then unwraps exactly one loader-generated synthetic wrapper if it has one child and no components. Multi-root containers and authored inner hierarchy remain intact; the old Scene has no retained root back-pointer. Targets attach below the new context's synthetic root. Context geometry remains a source and never enters the target list.

Helper DrawableComponent subclasses with isSceneGeometry=false are removed from the private candidates before realization/collection and again after resource hooks. No manipulation namespace dependency or hard-coded gizmo class is introduced. Other registered components, local transforms, flags and schema/resource/clone hooks remain on their original concrete instances. Source identities/ownership and enabled-light limits are checked across the combined graph.

Standard target geometry uses db::loadMeshBg2/readObj, not another file parser. Registered custom resource formats may use their virtual loadResources hook in immediate mode after attachment to the new context. CPU mode needs custom graphics-free deserialization to provide a CPU draw mesh; unsupported custom CPU preparation fails explicitly. Final resource hooks may not replace the prepared target Drawable/mesh and silently discard generated UV2. Custom external resources still need explicit source-discovery and export contracts.

## UV2 and realization boundaries

Enabled targets are prepared before a bake scene snapshot exists. With UV2 options, all CPU candidate meshes are copied/generated using geo::GenerateUv2AtlasModifier and draw clone semantics; all replacements validate before candidate component adoption. If a custom component already realizes its target, the existing generateUv2AndReload helper makes the independent replacement. Standard initial assembly generates before first upload, avoiding a needless initial raster/BLAS build and reload. There is no mutation of input files, context geometry or editable resources in another assembly.

UvAtlasValidator uses Warn: genuine overlap retains valid=true and diagnostics rather than skipping a target. buildUvSurfaceMesh supplies existing structural/finite-coordinate/positive-area/normal/chart checks. Invalid geometry, singular/nonfinite/nonaffine placement and transparent baked-map receivers fail with target identity/source diagnostics. Source material alpha defaults/classification, GPU table policies and object independence are unchanged.

With loadTargetGpuResources=true, the whole target tree's shared resource hooks complete and a draw queue validates the combined loaded scene, including lights/materials. With false, standard targets retain CPU mesh/material references; target resource hooks are deferred while context sources are still loaded. Call targetRoot->loadResources(engine) explicitly before StandaloneBakerContext::prepareScene. If this later operation fails, discard the private incomplete candidate; it is not a transactional live-scene replacement API.

Disabled branches remain in the authored graph and known dependency inventory but are not selected as targets; disabled-only/empty target inputs fail. Assembly does not animate/update arbitrary components. The caller performs any desired UpdateVisitor work, final target/resource validation and source invalidation before freezing M08-01's scene generation. Do not adopt material maps between serial target jobs without advancing sources; defer adoption until every target finishes to retain one frozen baseline.

## Validation, limits and downstream owners

[Manual fixture](../../../../tests/bake/assembly/README.md) covers camera-less/context/model/single-Node/multi-root assembly, BG2 joints, CPU/deferred/GPU target loading, unchanged source bytes, unknown/null entries, helpers, stable path IDs across fresh node IDs, independent loaded clones, registered parse failure/candidate isolation and nonfatal overlap. Additional lead checks cover native backend/texture/environment/custom hook/source alias/light/transform/cancellation contexts. This is a source-only fixture, with no target/framework/CMake change.

Static checks: public signatures/includes/schema adapters; root/parent/Scene ownership; target/source/resource/UV metadata; helper/custom hook behavior; staged replacements and no source mutation; GPL/control-flow/brace/function/method grouping; local Markdown paths and whitespace. No configure/build, shader compiler, executable/probe, GPU/app, output batch or package execution was performed. Root skills.md is absent as previously recorded; existing source and operative instructions were used.

Known dependencies are not a complete protected batch preflight. OBJ's existing ObjMesh does not expose the original MTL file list, and generic custom components do not enumerate external resources. M08-03 must supplement discovery/protection (or explicitly reject unverifiable jobs) before allowing output paths that could touch these dependencies; do not silently treat sourcePaths alone as complete. It also owns collisions, overwrite, output identity uniqueness across jobs, map encoding and actual complete resource publication with bool-noexcept failure checks.

M08-02 awaits project lead validation. M08-03 is the next step. Existing production render assembler, shared draw/gpu optimizations, default material/import rules, shaders and CMake are unchanged.
