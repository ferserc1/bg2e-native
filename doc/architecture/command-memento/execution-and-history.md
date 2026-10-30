# Execution and history

## Admission to publication

1. UI/application creates a command from an explicit request.
2. `prepare` validates undoability, generation, policy and estimated memory;
   a move-only reservation owns command and reserved storage.
3. Queue admission reports acceptance, while completion reports application.
   `pump()` schedules one FIFO head using the host executor.
4. At its safe boundary the command validates weak targets and prepares before
   publication. Failure leaves state unchanged or returns `Faulted` if recovery
   itself failed.
5. Only effective successful execution advances history. A new recorded edit
   discards the redo branch.

Queue cancellation differs from command rejection. Completion is once per
request, including failed admission. Absolute-update coalescing is narrow: only
adjacent, unscheduled updates with matching interaction, generation and exact
target/property key replace each other. Ordinary updates preserve FIFO.

## Adoption and cursor

For live drag previews, transaction `begin` captures the before value once;
updates change the preview. Commit stores the final value without executing the
same edit again. Cancel restores the baseline. No-op transactions create no
entry. Prepared results use another path: candidate data is generated privately,
then a command publishes it and captures the previous state.

Undo moves the cursor only when restoration succeeds. Redo reuses captured
forward values/resources; it does not re-run generators or read mutable files.
Recovery failure stops replay and faults history.

## Saved identity and dirty state

Document state identity is separate from history count. `markSaved()` follows
successful IO and marks that identity saved. Undo can return to that identity;
evicting its command does not fabricate a new saved state. Nonhistorical edits
require explicit `Preserve`, `InvalidateRedo` or `ResetHistory` compatibility.
Document replacement increments generation and resets baseline. Selection
commands have `None` impact and do not dirty authored scene state.

## Limits and release

Default limits are 256 entries/64 MiB. Preparation and transaction updates
reserve before mutation and refresh estimates as state grows. Explicit eviction
uses `discardOldest`/`discardRedo`; there is no automatic oldest-entry deletion.
Conservative peak reservations mean large edits can be rejected while recovery
memory exceeds the limit.

The optional release sink moves destruction to a caller-defined drained point
and retains command plus accounting tickets. This protects plugin code and
native resources still referenced by queued rendering. See
[resource handling](composer-resources.md).
