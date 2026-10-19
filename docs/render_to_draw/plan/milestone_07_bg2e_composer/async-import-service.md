# M07-04 asynchronous import and HTTP service handoff

M07-03 and its bool-reference correction are accepted by project lead
continuation. Starting HEAD is `f7c2498cb4c29e27921c91809043dcf5fe4d963c`;
its uncommitted M07-03 work is preserved. No CMake, configure/build, shader,
executable, package, commit, staging or reset operation was performed.

## Owners and thread boundaries

The app-local SceneImporter owns a joinable CPU worker, one active Job and up to
16 waiting jobs. Each job has its own serial/cancellation flag, captured document
generation, renderer identity/binding generation, weak destination and optional
weak reimport target. The worker owns CPU data and thread-safe Loader status only.
It never reads the live graph, invokes factories/UI widgets, uses an Engine or
uploads/submits resources. Parser completion is release/acquire synchronized;
the main thread joins the completed worker before reading its result. Completion
wakes MainLoop::requestFrame, including when background rendering is throttled.
Shutdown cancels jobs, wakes/stops/joins HTTP handlers and joins the CPU reader
before renderer/scene/Engine teardown. No detached MainLoop::asyncLoad task is used.

New reusable DB entry points keep parser data out of app services:

- db::readGltf(path) returns GltfDocument, an opaque CPU parser/buffer snapshot;
  db::loadGltf(document, drawEngine, progress) realizes it when stopped. The opaque
  storage hides cgltf's concrete parser representation from public headers; it
  does not conceal renderer/pass configuration. Existing path/production overloads
  retain the shared parser. Vertex/material conversion and image decoding still
  occur in the realization phase.
- db::readObj(path) returns ObjMesh: CPU geometry, MTL attributes and submesh names/
  groups. db::loadDrawableObj(model, drawEngine, callback) copies editable geometry/
  attributes and realizes resources. Existing draw path loading delegates to these
  operations; the production OBJ overload is retained.
- db::loadMeshBg2(path) remains the existing CPU reader. The new
  db::loadSceneBg2(path, const Bg2Mesh&, drawEngine, progress) realizes its retained
  CPU result, including chain joints and independent geometry/texture descriptions.
- db::readSceneDocument parses/validates a prefab's JSON on the worker. Scene
  factories, projection preparation and referenced resources run on the main thread.

Files referenced by texture paths and prefab resource paths are not an immutable
filesystem snapshot. Changing/deleting those files before realization can fail
that job; the previous import is retained. Cancellation is cooperative at parser
boundaries: a running third-party parser finishes before its worker can be joined.
Cancel restores scene/UI promptly and prevents publication; another job waits for
the occupied reader to finish. GPU realization is synchronous at a stopped main-
thread boundary and cannot be interrupted mid-call. Its Loader progress updates
do not promise intermediate presented frames while that call blocks.

## Queue, publication and reimport

Local menu imports and model drops enqueue jobs, preserving selected parent,
conversion scale/YUp-ZUp wrapper and existing document/save paths. Import Settings
selects local source units (m/cm/mm/in/ft) and Z up. Loader/queue UI retains the
paused scene image, shows progress/status, supports Cancel and Queue another
import, and can stop the HTTP service. Viewport controller input is suppressed
while valid jobs are loading. Loader entry clears selection/controller drag state
without changing authored camera settings, so consumed mouse releases cannot leave
stuck gestures after cancel/failure. Cancellation resumes the scene without changing
user continuous-rendering preferences. Terminal status has a separate status-bar
item, including when the document is dirty.

Main-thread processing first flushes pending authored edits, then validates the
job token/cancellation, document generation, destination membership and reimport
identity/parent. Renderer replacements reject prepared jobs. Ordinary reattachment/
resize of the same renderer can refresh its binding generation: parsing captured
no renderer/catalog/GPU targets to retain across that change. The current binding
is reacquired with withRenderer before realization. One job's successful scene
rebind therefore does not invalidate the rest of the same-document import queue.

Remote imports are tracked by canonical source path and document generation.
Reimport replaces the previous imported wrapper at its current parent; local
manual imports append independent content. Expired/detached entries are swept.
A removed or moved reimport target during parsing rejects the prepared replacement.
New/Open replacement cancels admitted/pending jobs and clears the table only after
successful adoption. HTTP admission captures a separately synchronized document
generation so a pending request cannot silently migrate to a newer document.

StageScene::publishImport preflights enabled cameras/eight lights while excluding
the old import, allowing replacement at the light limit. It reserves retirement
and table storage before graph publication, keeps the previous resources alive,
and rebinds extraction/settings before retiring them. Failure restores parent,
sibling order, camera/fill/environment state and dirty state, then rebinds the old
graph before disposing the detached candidate. Selection/clipboard are cleared,
source bake work is retired, histories invalidated and catalogs rebound through
the M07-03 notifyEdited contract. If restoring renderer bindings also fails, the
candidate is retained for renderer-first teardown and the fatal error propagates;
the app does not keep rendering with failed extraction. Cancellation/ordinary
parse, resource and validation failures preserve the original scene content.

## HTTP/configuration behavior

ImportServer/ImportRequest/ImportSettings are app-local copies of the production
protocol in namespace bg2e_composer; production files are unchanged. The app uses
the existing vendored cpp-httplib through a necessary private relative include;
no build-system edit or new dependency is required.

The service binds 127.0.0.1. Settings default to enabled on port 8643, accept
1024-49151 and use the independent draw.bg2e_composer.import preferences scope.
Window > Import Settings starts/stops and persists the service, validates the
port and reports bind failures. The legacy and draw apps cannot simultaneously
own the same port; the second app reports failure and allows a different port.
Stopping the service cancels its admitted jobs without canceling local jobs.

GET /status returns running/port/busy. POST /import requires application/json and
filePath, accepts optional fileName, units (m/cm/mm/in/ft) and coordinateSystem
(y_up/z_up), and accepts only glTF/GLB. There is one admitted HTTP slot; another
request returns 503. A successful response is 200 with status/message; malformed
requests, unsupported media, missing files and failed jobs preserve the original
400/415/404/500 behavior. The handler waits until main-thread realization,
publication and renderer binding restoration complete. Service stop wakes any
waiting handler with failure. No network handler accesses live scene/GPU objects.
The engine's current static glTF subset and unsupported-feature errors remain.

## Validation and remaining owners

Agent evidence is static API/include/GPL/style/ownership/thread/rollback/protocol/
resource-path/Markdown/whitespace inspection only. No configuration, compilation,
HTTP request, parser fixture, application, GPU, shader or package was executed.
Lead continuation accepts M07-03; backend/scenario coverage remains unspecified.

The [lead validation matrix](../../../../tests/draw/composer_import/README.md)
covers queue success/failure/cancel, service status/busy/errors/reimport, scene/
renderer/parent replacement, shutdown and backend/fallback/production regression.
M07-05 consumes the exact source invalidation/retirement hooks for multi-target
baking. M08 can reuse CPU parsers, not app scheduling/networking; M09 audits the
retained legacy import/server path before retirement. M07-04 awaits lead validation;
M07-05 is the next linked step.
