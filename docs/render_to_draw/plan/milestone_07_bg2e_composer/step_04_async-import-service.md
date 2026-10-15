# M07-04 — Guarded asynchronous import and server integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-02/M07-03 and existing MainLoop safeUpdateScene.

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
