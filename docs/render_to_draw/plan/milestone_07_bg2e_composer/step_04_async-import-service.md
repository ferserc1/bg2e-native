# M07-04 — Guarded asynchronous import and server integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M07-02/M07-03 and existing MainLoop safeUpdateScene.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08 batch assembly can reuse parsers but not composer-specific networking/scheduling. M09 final cleanup audits server/import code for hidden legacy calls.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
