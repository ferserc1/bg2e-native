# M07-04 — Guarded asynchronous import and server integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-02/M07-03 and existing MainLoop safeUpdateScene.

M02 DrawMaterialEditor validates renderer generation, binding revision and weak
selected-resource identity, but that does not validate a document or an import
request. Give each scene/document and import job an independent generation/token;
on publication recheck both plus source/target membership, then invalidate stale
selection/editor/bake/render bindings before replacing nodes/resources. Preserve
the existing scene on cancellation or failed publication. See [M02 review](../milestone_02_manipulation/review.md).

## Scope and deliverable

- `apps/bg2e_composer/src/SceneImporter.cpp`
- `apps/bg2e_composer/src/ImportServer.cpp`
- `apps/bg2e_composer/src/ImportSettings.cpp`
- `apps/bg2e_composer/src/ImportRequest.hpp`
- `apps/draw/bg2e_composer/src/SceneImporter.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/SceneImporter.cpp](../../../../apps/bg2e_composer/src/SceneImporter.cpp), [apps/bg2e_composer/src/ImportServer.cpp](../../../../apps/bg2e_composer/src/ImportServer.cpp), [apps/bg2e_composer/src/ImportSettings.cpp](../../../../apps/bg2e_composer/src/ImportSettings.cpp), [apps/bg2e_composer/src/ImportRequest.hpp](../../../../apps/bg2e_composer/src/ImportRequest.hpp).

## Implementation work

1. Port import request/config/status orchestration without altering intended service behavior or making draw depend on server/application objects.
2. Use low-level parse/IO on appropriate workers and high-level GPU realization/publication under the initialized draw mode and safe-update/loading policy.
3. Preserve pause/loader/progress, cancellation/token validity, multiple jobs and main-thread adoption. Coordinate producers and waitIdle for resource replacement where needed.
4. Prevent a late request from publishing into a replaced/deleted scene or restoring stale settings/bake targets; preserve documented failures and completion notifications inside the app.

## Verification and exit conditions

- Queued/canceled/failed/successful imports leave correct progress and scene state; late completion after scene replacement is ignored safely.
- Scene UI remains responsive according to current loader policy and no GPU objects are used from unsupported threads or another engine mode.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08 batch assembly can reuse parsers but not composer-specific networking/scheduling. M09 final cleanup audits server/import code for hidden legacy calls.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-16 reviewed baseline

Use actual delegate rendererGeneration and catalog generation with weak target references; add scene/job generation rather than treating those as the same identity. Direct Texture reload/updateSampler invalidates captured image/sampler pointers; rebuild extraction/bindings and reset effect/bake state before publication. Worker schema parsing must not implicitly invoke windowed UI or native upload. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M03-10 reviewed implementation requirement

Keep scene/import job validity separate from renderer/catalog generations. After stopped adoption reload geometry identities, replace retained tracing/material snapshots, invalidate effect/SDK histories and rebind live settings/selection. Do not let asynchronous jobs retain a borrowed catalog target from a removed scene.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Stop or retire an active bake before main-thread scene/texture/geometry adoption. Imported replacement nodes must not be published into an existing frozen bake baseline. Explicitly invalidate sources/reset targets, reacquire environment snapshots and rebind UV/result previews; a worker token alone does not establish GPU completion or valid target generations.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## M07-02 preparation/adoption handoff

Reuse StageScene::prepareImport(path, unitsScale, sourceIsZUp, progress) and
insert(candidate, parent), documented in [scene-io-cloning.md](scene-io-cloning.md).
They currently run synchronously at a stopped Engine boundary. Preserve their
separate conversion wrapper, typed resource realization and document path/dirty
semantics. Background parsing must not move GPU realization to an unsafe worker.
Capture request and document generations plus a weak current parent; revalidate
after asynchronous completion and before insertion/reimport removal. Retire or
discard detached candidates before releasing their borrowed context.


## M07-03 editor mutation handoff

Consume [scene-editing.md](scene-editing.md): SceneTree callbacks carry intents,
AppDelegate queues weak document/member targets, and notifyEdited retires bake
sources, marks dirty, invalidates histories and reattaches extraction/catalogs.
Tree selection is explicitly node-only; submesh material intent remains separate.
Rebuild tools and prune selection before retaining removed resources. Respect
StageScene::validateEditingGraph camera/eight-enabled-light constraints and
DrawableComponent::isSceneGeometry helper exclusion when adopting job output.
M07-03 does not implement async services or multi-target bake scheduling.
