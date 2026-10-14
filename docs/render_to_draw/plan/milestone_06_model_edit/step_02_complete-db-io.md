# M06-02 — Complete engine-aware model/scene resource load and save

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M06-01, M01 DB levels and M05 readback.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M07 scene IO and M08 bake model/preflight use these complete resource paths. M09 removes production overloads without breaking neutral IO.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
