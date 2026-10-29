# Step 3 — Composer dispatch and document lifecycle

Status: complete (source integration; uncompiled). See [progress.md](progress.md) and the step 3 checkpoint in [mutation-inventory.md](mutation-inventory.md).

Dependency: step 2.

## Work

- Introduce the Composer command context/dispatcher and application command subdirectory. Move reusable scheduling mechanics into `bg2e::app`; keep graphics synchronization adapters and Composer invalidation policy outside the generic core.
- Submit command instances through CPU-value, retained-resource or conservative renderer-update boundaries as required. Preserve generation guards, owner lifetime checks and renderer-generation cancellation.
- Capture original state only when execution reaches the safe boundary, after validating the document and target. Freeze intended targets at dispatch/interaction start; never rediscover them from the current selection during redo.
- Add initial simple commands such as node rename/enabled state. Replace their opaque lambdas while leaving a clearly tracked migration path for remaining actions.
- Centralize effects reporting: authored changes, tools/selection refresh, lightmap-source invalidation, render-history invalidation and frame requests. A selection-only command must not trigger document invalidation by default.
- Update `Document` and save flow to use stable saved-state identity plus explicit nonhistorical dirty revisions. Pending preview changes count as modified. Saving resolves valid transactions and queues behind prior edits before establishing the saved snapshot's identity.
- On successful document replacement create/reset history only at publication; failed/cancelled open preserves the current document/history. Preserve source/save paths outside mementos. Cancel stale queued requests and transactions deterministically.
- Retire histories, retained nodes/resources and callbacks before engine/plugin teardown. Renderer recreation must not leave commands holding invalid GPU realizations; define compatible re-realization or clear invalid history explicitly.
- Keep open/save/export and parsing outside commands. Do not move `EditorActions.cpp` wholesale into the motor's generic application framework.

## Completion

Initial scene commands use safe queued execution, document/history lifecycle is connected, and remaining lambda-based mutation paths are listed in the inventory. Source review only; no Composer tests or build.
