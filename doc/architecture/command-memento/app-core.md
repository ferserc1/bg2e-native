# Application core

See the [public app reference](../../api/app/CommandHistory.md) for declarations.

`CommandInfo` carries name/category, `CommandHistoryPolicy`,
`CommandDocumentImpact` and undoability. `CommandResult` statuses are `Applied`,
`NoOp`, `Rejected`, `Cancelled`, `Failed`, and `Faulted`; only Applied and NoOp
are accepted. Diagnostic `string_view` text must have static lifetime.
`CommandContext` freezes document generation, origin and compatibility.

| Policy or impact | Behavior |
|---|---|
| `Never` | Apply without adding an entry. |
| `Always` | Require an undoable compatible command and record effective work. |
| `UserPreference` | Resolve category preference at admission; decision is a snapshot. |
| Impact `None` | Does not alter document saved-state identity (optional selection). |
| Impact `Document` | Successful recorded edits change the current state identity. |

`CommandMemory.localBytes` is per-entry. `CommandResource` represents one
immutable resource version; shared identity is accounted once, with equal byte
declarations required. Estimates must include preparation, redo and recovery
peak. A mismatch or over-limit growth rejects the operation.

`Command` owns operation semantics and recovery. `CommandHistory` owns
reservations and applied entries. Calls, callbacks and destruction belong to
its constructing thread; callbacks must not re-enter it. `CommandQueue` borrows
history and owns pending tasks, so history must outlive it. `CommandTransaction`
uses the same contract for preview/adoption. `CompositeCommand` owns immutable
children. `CommandReleaseSink` may delay destruction, retaining tickets and
engine/module owners until native work is safe to release.

The core defaults to 256 entries and 1 GiB. Successful edits automatically evict oldest entries when retention limits are exceeded. Failed
recovery faults history; `reportExternalFault` covers host-side publication
that could not be restored. `resetBaseline` requires a greater generation after
producers stop.

Legacy document notifications return a recoverable rejection while history is
busy. Composer keeps already-applied legacy changes dirty and retries their
coalesced compatibility notification at a stable boundary, including before
save. Cancelling a preview cannot clear that pending dirty state. Saving with
unsettled commands returns failure without clearing unsaved state. Dispatcher
admission failures and throwing completion notifications do not escape into the
editor loop. Queue cancellation during model execution and payload-limit
reductions below pending bytes return `Rejected` without changing queued work.

Composite children must be undoable and share parent category/policy and impact.
Execution tracks effective children. If a later child fails, earlier children
are undone; failed compensation faults history. This is used for grouped scene
graphics settings, not as a general transaction manager.
