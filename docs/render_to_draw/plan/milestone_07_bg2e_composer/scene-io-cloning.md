# M07-02 scene IO and clone handoff

[Step](step_02_scene-io-cloning.md) · [Progress](../progress.md) ·
[Lead checks](../../../../tests/draw/composer_io/README.md)

Starting HEAD: `bd6a95076141335550f415f47ea2a824703b119c`; checkout initially clean.
M07-01 accepted by project lead continuation instruction. Backend/package/scenario
coverage unspecified. All implementation changes are uncommitted. No configure,
compile, shader command, executable/probe/package run, staging or reset.

## Actual application operations

The Composer AppDelegate now queues Open, Save, New, Import and Export requests
through DocumentIo.cpp. prepareFrame retires a bake submission, flushes queued
edits, drains the device and processes the operation before command acquisition.
Replacement offers Save/Discard/Cancel before loading a candidate. A requested
successful save remains committed if the later open fails. Cancel or load failure
leaves the original graph, paths, dirty state and selection in place. New and
Close install a fresh Scene with an editable orbit camera. Open of a model builds
a fresh camera-bearing document; drop of a model inserts it instead.

`StageScene::prepareScene` uses graphics-free `db::loadScene(path, progress)` for
schema parsing, then explicit `Node::loadResources(drawEngine, progress)`. The
source Scene releases its complete root through `Scene::releaseSceneRoot`, which
clears the root back-pointer and cached camera/environment/light references.
Exactly one synthetic level is unwrapped for single-root files. Multi-root files
retain the detached container, preserving all nodes and avoiding dangling Scene
pointers. Complete scene opening requires an enabled camera; prefab import does
not. File dialogs accept JSON and existing scene aliases; directory open resolves
Name/Name.json using the shared path helper.

The active camera is the first enabled depth-first camera, matching draw scene
extraction. It may have its orbit controller on an ancestor; no controller is
invented for authored static cameras. Missing projections receive the production
Composer optical fallback (50 mm frame, 55 mm lens, far 1000). Built-in perspective
and optical parameters are retained in app-local RH zero-to-one projections.
Invalid built-in FOV/optical/clipping parameters fail before adoption. Camera
rotation/framing/intrinsics mark the document dirty. The initial camera now belongs
to the document and is saved; floor/fill remain editor fixtures. Full active-rig UI,
controller routing and fixture/light policy remain M07-03 work.

`prepareImport(path, unitsScale, sourceIsZUp, progress)` produces a detached wrapper
holding authored content, with positive finite uniform scale and optional -90
X rotation. BG2/VWGLB, OBJ/MTL, static glTF/GLB and JSON prefab inputs reuse draw
loaders. `importDocument` captures a weak selected parent (or editable root) plus
document generation and inserts at the stopped boundary. Scene path is retained,
editing/bake histories invalidate and the document becomes dirty. Open/import/
duplicate preflight rejects more than eight enabled authored lights; an eighth
authored light suppresses the fill fixture before renderer attachment. This is
synchronous orchestration, not the M07-04 background/HTTP request scheduler.
Detached candidates are explicitly cleaned on failed resource/tool preparation.

Save uses shared scene path inspection/confirmation and canonical Name/Name.json
layout. Native Save As confirms its final destination once; ordinary Save permits
overwrite of the current canonical document but confirms redirected legacy paths.
`StageScene::save(path, destinationConfirmed)` returns false on cancellation,
otherwise invokes complete `db::saveScene(root, drawEngine, path)` and marks clean
only after publication. GPU-only `.bg2tex` mip/layer/HDR bytes, material indirect
mode/UV/sampler/color fields, registered component reference hooks, unknown-type
failure, unique sidecars and main-file-last rollback remain the M06 contracts.
Raw read/write preserves unknown JSON; typed graph IO does not silently preserve
unregistered component types or infer custom external-resource writers.

Export Selected Model(s) snapshots only directly selected drawable nodes. Single
selection uses a BG2 save dialog; multiple selection uses a folder with one
Name/Name.bg2 output per drawable. It excludes children, node placement and other
scene components, and retains mesh/submesh/material and input/output joints.
It borrows live drawables only for synchronous complete export without changing
live source paths or document cleanliness. Portable names, case-folded duplicates
and existing subfolders are validated before model writes. Each complete model is
transactional; earlier exports survive a later batch failure, which names the
failed destination. This is not an atomic batch publisher or M08 output scheduler.

## Shared component/projection contracts

Node lookup prefers an exact component type, then searches ordered components for
a compatible base interface; user Drawable/Transform/Camera subclasses are no
longer invisible to shared engine accessors. Schema constructors must return a
detached component with the registered typeName, then use virtual graphics-free
parsing. Draw realization continues through loadResources; production creators
remain separate. Custom components must implement their schema/reference/draw
resource hooks as appropriate.

Node::clone validates nonnull, independent, ownerless components of the same RTTI
type, preserving common priority/ignoreSerialization state. A subclass inheriting
a slicing clone now fails explicitly and must override clone. Nodes keep fresh
IDs; existing Drawable clones duplicate CPU meshes, GPU meshes, live materials and
GPU images. Composer cloning publishes a prepared detached branch as a sibling,
retains the scene root and no longer copies Model Edit's five-light limit.

Projection::clone is a virtual deep-copy contract. Built-in perspective/optical
projections implement it; CameraComponent validates concrete type and independence.
Custom projections must override it rather than inherit a slicing built-in clone.
The app-local Model Edit projection supplies that dependent override too. Orbit
clones retain authored state and clear pressed keys/mouse/flight gestures.
Perspective deserialization now actually reads FOV and clipping planes; unsupported
serialized base Camera projection types fail instead of disappearing silently.
Base cameras initialize their matrix to identity.

## Verification limits and downstream work

Static source/include/API/mode/ownership/rollback, full GPL/style/delimiter and
whitespace/document-link checks were performed. The schema probe and GPU clone
probe are source fixtures for the lead; neither was compiled or executed. The
manual matrix covers round-trips, camera validation, custom subclasses, insertion,
partial batch failures, cancellation, native resources and production coexistence.
No backend performance, package or numeric result is claimed.

M07-03 consumes the actual operation/dirty/camera/selection contracts, adding full
scene editor UI, guarded component mutations, fixture/light policy and active-rig
controls. M07-04 adds request/scene generation validation across asynchronous
preparation and HTTP imports; renderer generations alone remain insufficient.
M07-05 extends bake validity to multiple targets. M08 can reuse detached prefab
preparation/root transfer and complete writers, but still owns standalone context
realization and batch scheduling. M09 must retain custom clone/schema contracts
while removing production factories only at retirement.
