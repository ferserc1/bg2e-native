# Step 2 — Reusable infrastructure in bg2e::app

Status: complete. Public APIs are in `lib/include/bg2e/app/{Command,CommandHistory,CommandQueue,CommandTransaction,CompositeCommand}.hpp`; focused sources are in `tests/app/command_memento/`. Tests have not been compiled or run.

Dependency: step 1.

## Work

- Add generic public headers and implementations directly under `bg2e/app`. Proposed types: `Command`, `CommandQueue`, `CommandHistory`, `CommandTransaction`, `CompositeCommand`, result and policy/context types. Follow existing export and ownership conventions.
- Keep the core independent of scene, UI, draw/render, GPU, reflection and Composer. Execution is owner-thread serialized; asynchronous producers must marshal requests rather than race the history.
- Commands expose name/category, undo support, history policy and document impact. Inject application preference resolution; the engine does not know Composer categories or preference storage.
- Queue owning command instances in order. Distinguish queue acceptance from successful execution. Provide completion/cancellation and an executor hook so Composer can select an existing safe graphics boundary.
- Implement execute/undo/redo and already-applied adoption. No-op, rejection or recovered failure must not add history, advance the cursor or discard redo. A successful new recorded edit discards redo.
- Reserve history storage/ownership needed for recording before publishing a mutation. Avoid a successful edit becoming untracked because insertion/allocation failed afterward.
- Implement composite atomicity with reverse-order undo and rollback of partial execution. Define explicit fault handling when recovery fails; prohibit further unsafe history operations in a faulted state.
- Implement a single active transaction per history initially. Capture before once at the safe boundary, apply updates, commit one already-applied entry, cancel by restoration, and discard an unchanged final result. Serialize commit/cancel relative to queued updates.
- Support absolute-update coalescing only for compatible transaction updates. Do not coalesce arbitrary incremental operations or cross target/property/origin boundaries.
- Track stable document-state/save identifiers independently of selection entries and cursor indexes. Define eviction behavior when the saved state becomes unreachable.
- Bound retained history memory/entries, including detached redo entries and oversized commands. Do not silently turn an undoable edit into a nonundoable mutation when its budget is exceeded. Prefer a documented rejection or explicit application policy. Shared resources need an accounting strategy that avoids obvious duplicate counting.
- Release discarded commands through caller-safe ownership boundaries; generic core must not issue GPU waits or destroy engine resources after their owner is gone.

## Permitted focused tests

Add source-level automated tests under `tests/` for the generic core only, following available repository conventions without adding a test framework dependency or editing CMake. Use minimal fake commands and an in-memory model.

Cover queue ordering/completion, execute/undo/redo branching, no-ops, failure/cursor atomicity, composite rollback/faults, transaction commit/cancel/already-applied behavior, policy snapshots, selection-only versus document state, saved-state identity after branching/eviction, and retained-object release. These tests must exercise public behavior rather than duplicate implementation logic.

Do not add Composer tests. Do not compile or run compilation-backed tests unless explicitly requested; record unexecuted test sources honestly.

## Completion

The core contracts and focused test sources exist, dependencies remain generic, and `progress.md` records actual API names and source-review results.
