# Composer scene IO and independent clone checks

These checks are delegated to the project lead. No source here has been compiled
or executed by the implementing agent. No CMake registration was added.

`schema.cpp` is a standalone C++20 probe linked to bg2e using the normal engine
headers/library. Invoke `composer_schema_probe EXISTING_DISPOSABLE_DIRECTORY`.
It creates `composer-schema.json` in that directory. It needs no Engine/device or
referenced asset. It checks user-subclass factory parsing without production
creators, polymorphic base lookup, fresh clone identity and common metadata,
independent perspective/optical camera projection state, rejection of sliced
component clones, three save/load cycles without additional root nesting, FOV/
near/far persistence and ownership transfer of a multiple-root scene container.

`gpu.hpp` provides `bg2e::draw::tests::runComposerCloneProbe(engine, directory)`.
Call manually from an initialized draw app with submissions stopped and an
existing disposable directory. It checks independent CPU/GPU mesh/material/image
objects, independent geometry/material edits through complete scene save/reload,
and restoration of live source references. Also run the existing
[complete HDR/mip/UV2 resource probe](../resource_io/README.md).

## Application regression matrix

Use `draw_bg2e_composer` on applicable Vulkan and Metal backends. Reconfigure with
the normal platform generator so the app's new DocumentIo.cpp is discovered.

| Scenario | Expected result |
|---|---|
| Edit initial cube/camera, save, reopen three times | One editable root; camera/controller, names, flags, component priorities and independent resources retained; no editor floor/fill fixture exported |
| Open a production scene with perspective/optical cameras | First enabled depth-first camera is active for draw and selection; authored FOV/near/far retained; missing projection gets 50 mm film / 55 mm lens optical fallback |
| Open multi-root scene, save and reopen | Original children retained below one editable container; no dangling source Scene pointer or repeated wrappers |
| Open a scene without an enabled camera | Clear error; current graph, selection, paths and dirty state retained (an explicitly requested prior Save remains committed) |
| Import camera-less JSON prefab, BG2 with joints, OBJ/MTL or glTF | Subtree inserted below selection or root; previous nodes/path retained; scene becomes dirty |
| Call importDocument(path, scale, zUp) with translated parent | Scale and -90 degree X conversion live on a separate wrapper; authored child transforms retained; invalid/nonfinite/nonpositive scale rejected |
| Drop a model vs scene file/folder | Model inserts; JSON/legacy scene or Name/Name.json folder replaces after dirty protection |
| Save/Discard/Cancel before New/Close/Open, Save As cancellation | Save snapshots flushed edits; discard adopts replacement only after successful load; cancel or failed IO retains current document |
| Save legacy/off-layout path; cancel redirected overwrite | Canonical Name/Name.json destination inspected and confirmed; cancel retains document/path/dirty state |
| Missing texture or unsupported custom schema/resource serializer | Load/save reports failure; current scene or previous main/resources stay available; no false clean state |
| Clone a node with transforms/controller/custom components | Fresh node IDs and independent CPU/GPU state; runtime controller gestures not copied; sliced/shared/owned custom component clones fail explicitly |
| Export one selected model with children | Only its own local mesh/material/joints exported; scene placement/components and descendants excluded; document remains dirty/clean as before |
| Export multiple selected nodes | One Name/Name.bg2 per directly selected drawable; non-drawables skipped; bad/duplicate/reserved names or existing destination subfolders rejected before any model writes |
| Fail a later batch export | Failed destination reported; earlier complete model exports remain readable; no all-batch atomicity is claimed |
| Replace/delete target after queuing import/export | Weak target and document-generation checks reject stale operations |
| Close/replace with active bake or retained previews | Work retires before graph/Engine resources; old selection/material actions do not affect the replacement |
| Reopen saved bakedGI/AO model/scene | GPU-only HDR/mips, linear UV2 mode, sampler/color/inversion and indirect metadata remain correct |

Production `bg2e_composer` and `draw_model_edit` remain regression references.
M07-03 still owns the complete scene tree/component/menu/gizmo/active-controller UI
and the Composer-specific fixture/light policy. M07-04 owns async import/server
scheduling and M07-05 owns multiple bake jobs; these checks do not establish their
completion or package/backend evidence.
