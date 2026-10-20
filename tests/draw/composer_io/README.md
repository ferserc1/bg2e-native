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
| Open a scene with enabled Environment and valid HDR, JPEG or PNG | Loaded component selected; renderer and inspector agree; no hidden auxiliary Environment supersedes it. HDR remains linear; SDR is converted from sRGB to linear before environment generation. Check environmentError |
| Open with disabled Environment plus enabled sibling/child | Disabled source is ignored; first enabled scene Environment becomes active consistently in renderer/queue/settings |
| Model Edit removes/re-adds an Environment and calls updateEnvironment | Legacy automatic lookup re-resolves and preserves Model Edit behavior; default sky when no component remains |
| Change selected HDR/JPEG/PNG, click Clear, edit horizon/zenith/ground RGB | Clear sets an empty path (Browse-cancel does not); procedural RGB becomes visible and persists after safe updates/save/reopen |
| Reopen a changed scene with the same HDR path as the prior document | Source-cache reset rereads/regenerates the file; renderer displays new contents rather than reusing the previous snapshot |
| Add another Environment earlier in the hierarchy while one is active | New component stays in the graph but is inactive; it cannot steal selection or trigger generation |
| Remove active Environment while an ignored component remains | Default sky returns; ignored components are not promoted. Add a fresh Environment: it becomes active |
| Disable/detach selected branch, rollback import, Open/New | No dangling source; disabled branches are ignored and failed imports restore prior selection; fresh documents choose their own first eligible component or defaults |
| Select invalid HDR/JPEG/PNG image, then correct or click Clear | Picker filters supported environment formats; Clear sets empty path, cancelled Browse preserves it and procedural RGB returns. Values remain editable; last good snapshot plus visible error, then recovery |
| Open multi-root scene, save and reopen | Original children retained below one editable container; no dangling source Scene pointer or repeated wrappers |
| Open a scene without an enabled camera | Clear error; current graph, selection, paths and dirty state retained (an explicitly requested prior Save remains committed) |
| Import camera-less JSON prefab, BG2 with joints, OBJ/MTL or glTF | Subtree inserted below selection or root; previous nodes/path retained; scene becomes dirty |
| Import Blender glTF/GLB with emissive RGB, AO strength or material extensions | Import succeeds like production; supported core material data is retained and unsupported factors/extensions are logged as warnings, not fatal conversion errors |
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
| Open JSON/legacy scene or scene folder while moving/resizing the window | Raw document IO/parse runs on the guarded worker with Loader; GPU component realization advances between Loader frames on the main thread; no scene rendering or editor mutations while loading |
| Repeat Open/drop/Save/New/Close during scene load | No overlapping replacement or save; Close is deferred/rejected until the pending load finishes; no stale worker publication |
| Invalid JSON, missing resource, or shutdown/exception while reading | Error reported with old document retained before adoption; CPU worker joined before UI/Engine teardown; queued completion of the exiting run is cancelled |
| Open primitives_scene with full tracing temporarily disabled | Null entries warn/skip, camera and all eight drawables load; per-component resource progress appears; separately track the known tracing capacity overflow rather than attributing it to JSON IO |
| Reopen saved bakedGI/AO model/scene | GPU-only HDR/mips, linear UV2 mode, sampler/color/inversion and indirect metadata remain correct |

Production `bg2e_composer` and `draw_model_edit` remain regression references.

One resource load is still synchronous: a very large model/image can stall its
individual Loader frame. Environment/renderer initialization after adoption also
remains synchronous. Fully streaming CPU model/image decode and bounded GPU
uploads are not claimed by this correction. Re-enable tracing only to reproduce
the separate capacity failure described in the M07 investigation.
M07-03 still owns the complete scene tree/component/menu/gizmo/active-controller UI
and the Composer-specific fixture/light policy. M07-04 owns async import/server
scheduling and M07-05 owns multiple bake jobs; these checks do not establish their
completion or package/backend evidence.
