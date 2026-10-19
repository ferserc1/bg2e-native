# M06-02 — Complete engine-aware model/scene resource load and save

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M06-01, M01 DB levels and M05 readback.

## Scope and deliverable

- `lib/include/bg2e/db/scene.hpp`
- `lib/include/bg2e/db/scene_gltf.hpp`
- `lib/include/bg2e/db/mesh_bg2.hpp`
- `lib/include/bg2e/db/mesh_obj.hpp`
- `lib/include/bg2e/scene/ComponentFactoryRegistry.hpp`
- `apps/draw/model_edit/src/StageScene.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/db/scene.hpp](../../../../lib/include/bg2e/db/scene.hpp), [lib/include/bg2e/db/scene_gltf.hpp](../../../../lib/include/bg2e/db/scene_gltf.hpp), [lib/include/bg2e/db/mesh_bg2.hpp](../../../../lib/include/bg2e/db/mesh_bg2.hpp), [lib/include/bg2e/db/mesh_obj.hpp](../../../../lib/include/bg2e/db/mesh_obj.hpp), [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp).

## Implementation work

1. Complete draw overloads for required .bg2/OBJ/glTF and scene/resource loading with progress, per-texture settings, submesh metadata and component factory path selection.
2. Keep low-level schema/file/reference IO engine-free and incomplete by design; high-level helpers accept engine/context for full realization and safe publication.
3. Implement complete save paths for edited/generated textures, including GPU-only images: obtain host pixels through GPU before DB encoding, then write resource references and models with correct relative paths/metadata.
4. Preserve expected scene/model names, joints/material semantics and error/unsaved-document behavior. Keep production overloads operational and avoid duplicating import parsers merely to swap engine types.

## Verification and exit conditions

- Load/edit/save/reload required formats and graph metadata; schema-only round-trip needs no graphics context.
- A texture created solely on GPU is exported and reloaded with correct color/UV/lightmap settings, not dropped or represented by an unresolved nonexistent file.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07 scene IO and M08 bake model/preflight use these complete resource paths. M09 removes production overloads without breaking neutral IO.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-07 actual DB baseline

Build on [db-levels.md](../milestone_01_deferred_renderer/db-levels.md): readSceneDocument/writeSceneDocument preserve raw JSON without devices; loadScene(path) creates a typed graph with unresolved references; saveSceneReferences explicitly writes references only. Engine-aware draw scene/BG2/OBJ overloads use draw::Engine& and Node/Component::loadResources. OBJ has no MTL import yet. Complete draw save/export is not implemented and existing production save rejects draw resources. Implement real GPU readback/resource encoding and publication here rather than relabeling reference IO as complete save. New factory schema/draw paths use DefaultCreator plus virtual two-argument deserialize/serializeReferences and optional loadResources; unsupported custom components throw instead of invoking production creators. Preserve custom subclass fields explicitly.

## M05-05 completed-image readback baseline

M05 now supplies synchronous `gpu::Image::readPixels` for one initialized color
mip/layer with `TransferSrc`; returned bytes are tightly packed in the image's
native pixel format. `readPixelsRGBA8` only accepts `R8G8B8A8_UNORM`. Vulkan uses
its transfer staging path and Metal strips row padding. The offscreen example
selects that RGBA8 format, verifies a temporary PNG round-trip, and publishes one
non-overwriting `scene.png` after GPU completion. This is a camera-image example,
not a reusable resource-save API: it does not serialize GPU texture mip/layers,
material references, scene graph/model files, or HDR formats. M06 must retain the
actual resource format and metadata, perform required conversion explicitly, and
write the complete scene/model resources and references with correct relative
paths. It may reuse the completed raw readback and DB codec contracts; do not claim
full resource export from the M05 example's PNG writer.

## M01-16 reviewed baseline

Existing readSceneDocument/writeSceneDocument preserve raw JSON; loadScene schema/reference IO does not realize resources. Node/Component::loadResources, draw BG2/OBJ overloads and strict mode/context rejection exist. Complete high-level draw save currently rejects resource export; OBJ maps in the example are explicitly assigned, not MTL import. Implement the actual missing glTF/MTL, edited/GPU-only resource save, joint/custom-component round trips using M05 readback and preserve production overloads. See [M01 review](../milestone_01_deferred_renderer/review.md).


## M04-07 reviewed implementation requirement

Materialize GPU-authored indirect maps before writing material references. M04 stores explicit IndirectTextureMode but its GPU-backed Texture settings can have an empty source path; serializeMaterial alone cannot export the image. Save/reload actual RGB content, linear interpretation, ao/bakedAO/bakedGI mode, UV2 and sampler/scale without converting HDR to UNORM. Cover clone-after-bake and stored-definition/live-realization agreement.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M06-01 concrete application baseline

The registered application is apps/draw/model_edit (`draw_model_edit`), with
AppDelegate/StageScene/EditorUi and a procedural startup fixture. There is no new
Document or complete file loader/saver yet. Implement the Document state and IO
publication in this application, replacing the fixture only after successful
stopped adoption. Preserve current document on failed/cancelled load/save and add
file/drop/dialog flows from the [inventory](application-inventory.md). App identity
is org.bg2e.apps.draw.model-edit; production model_edit remains intact. A new app
startup is not evidence that glTF/MTL/full resource export already works.


## M06-02 delivered implementation

See [resource IO contracts and review handoff](resource-io.md), the maintained
[API reference](../../../../doc/api/db/DrawResourceIO.md) and
[manual verification matrix](../../../../tests/draw/resource_io/README.md).
The complete save overloads, native GPU archive, shared MTL/glTF import and editor
Document/IO are now implemented; runtime acceptance remains lead-owned. Import
subsets and custom external-resource export contracts are explicit, not inferred
from raw/schema preservation. Next continuation selects M06-03.
